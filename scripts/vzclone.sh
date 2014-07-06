# 
# 
# Filename: vzclone
# Author: Nipun Arora
# Created: Sat Jul  5 23:05:45 2014 (-0400)
# URL: http://www.nipunarora.net 
# 
# Description: 
# Live Cloning of OpenVZ
#
#!/bin/sh

echo " Sandbox Testing, VZ Cloning \n"

ACT_SCRIPTS_SFX="start stop mount umount premount postumount"
# blowfish is a fast block cipher, much faster then 3des
SSH_OPTIONS="-c blowfish -o BatchMode=yes"
SCP_OPTIONS=$SSH_OPTIONS
RSYNC_OPTIONS="-aHAX --delete --numeric-ids"
VZCTL=vzctl

online=0
verbose=0
remove_area=1
keep_dst=0
debug=0
times=0
compact=0
snapshot=0
check_only=0
confdir="/etc/vz/conf"
vzconf="/etc/vz/vz.conf"
tmpdir="/var/tmp"
act_scripts=
PLOOP=no
RSYNC1_OPTIONS=
PLOOP_DEV=
TOP_DELTA=

# Errors:
MIG_ERR_USAGE=1
MIG_ERR_VPS_IS_STOPPED=2
MIG_ERR_CANT_CONNECT=4
MIG_ERR_COPY=6
MIG_ERR_START_VPS=7
MIG_ERR_STOP_SOURCE=8
MIG_ERR_EXISTS=9
MIG_ERR_NOEXIST=10
MIG_ERR_IP_IN_USE=12
MIG_ERR_QUOTA=13
MIG_ERR_CHECKPOINT=$MIG_ERR_STOP_SOURCE
MIG_ERR_MOUNT_VPS=$MIG_ERR_START_VPS
MIG_ERR_RESTORE_VPS=$MIG_ERR_START_VPS
MIG_ERR_OVZ_NOT_RUNNING=14
MIG_ERR_APPLY_CONFIG=15
MIG_ERR_UNSUP_PLOOP=16
MIG_ERR_UNSUP_CPT_VER=17
MIG_ERR_UNSUP_CPU=18

# For local vzctl to work, make sure /usr/sbin is in $PATH
if ! echo ":${PATH}:" | fgrep -q ':/usr/sbin:'; then
	PATH="/usr/sbin:$PATH"
fi

usage() {
	cat >&2 <<EOF
This program is used for container migration to another node.
Usage:
vzmigrate [-r yes|no] [--ssh=<options>] [--keep-dst] [--live] [-v]
	destination_address <CTID>
Options:
-r, --remove-area yes|no
	Whether to remove container on source host after successful migration.
--ssh=<ssh options>
	Additional options that will be passed to ssh while establishing
	connection to destination host. Please be careful with options
	passed, DO NOT pass destination hostname.
--rsync=<rsync options>
	Additional options that will be passed to rsync.
--keep-dst
	Do not clean synced destination container private area in case of some
	error. It makes sense to use this option on big container migration to
	avoid re-syncing container private area in case some error
	(on container stop for example) occurs during first migration attempt.
-c, --compact
	Compact a container image before migration. Works for ploop only.
-s, --snapshot
	Create a container snapshot before migration. Works for ploop only.
--live
	Perform live migration: instead of restarting a container, checkpoint
	and restore are used, so there is no container downtime or service
	interruption. Additional steps are performed to minimize the time
	when a container is in suspended state.
--check-only, --dry-run
	Do not perform actual migration, stop after preliminary checks.
	This is used to check if a CT can possibly be migrated. Combine
	with --live to enable more checks for live migration case.
-v
	Verbose mode. Causes vzmigrate to print debugging messages about
	its progress (including some time statistics). Multiple -v options
	increase the verbosity. The maximum is 4.
-t, --times
	At the end of live migration, output various timings for migration
	stages that affect total suspended CT time.

Examples:
	Online migration of CT #101 to foo.com:
		vzmigrate --live foo.com 101
	Migration of CT #102 to foo.com with downtime:
		vzmigrate foo.com 102
Notes:
	This program uses ssh as a transport layer. You need to put ssh
	public key to destination node and be able to connect without
	entering a password.
EOF
	exit $MIG_ERR_USAGE
}

# Logs message
# There are 3 types of messages:
# 0 - error messages (print to stderr)
# 1 - normal messages (print to stdout)
# 2 - debug messages (print to stdout if in verbose mode)
log () {
	if [ $1 -eq 0 ]; then
		shift
		echo "Error: $*" >&2
	elif [ $1 -eq 1 ]; then
		shift
		echo "$*"
	elif [ $verbose -gt 0 ]; then
		shift
		echo "   $@"
	fi
}

# Executes command and returns result of execution
# There are 2 types of execution:
# 1 - normal execution (all output will be printed)
# 2 - debug execution (output will be printed if verbose mode is set,
#     in other case stdout and stderr redirected to /dev/null)
logexec () {
	if [ $1 -eq 1 -o $verbose -gt 0 ]; then
		shift
		$@
	else
		shift
		$@ >/dev/null 2>&1
	fi
}

undo_conf () {
	$SSH "root@$host" "$VZCTL set $VEID --name '' --save > /dev/null"
	$SSH "root@$host" "rm -f $vpsconf"
}

undo_act_scripts () {
	if [ -n "$act_scripts" ] ; then
		$SSH "root@$host" "rm -f $act_scripts"
	fi
	undo_conf
}

undo_private () {
	if [ $keep_dst -eq 0 ]; then
		$SSH "root@$host" "rm -rf $VE_PRIVATE"
	fi
	undo_act_scripts
}

undo_root () {
	$SSH "root@$host" "rm -rf $VE_ROOT"
	undo_private
}

undo_quota_init () {
	[ "${DISK_QUOTA}" = 'no' ] || $SSH "root@$host" "vzquota drop $VEID"
	undo_root
}

undo_quota_on () {
	[ "${DISK_QUOTA}" = 'no' ] || $SSH "root@$host" "vzquota off $VEID"
	undo_quota_init
}

undo_sync () {
	# Root will be destroyed in undo_root
	undo_quota_on
}

undo_suspend () {
	logexec 2 $VZCTL chkpnt $VEID --resume
	undo_sync
}

undo_dump () {
	if [ $debug -eq 0 ]; then
		rm -f "$VE_DUMPFILE"
	fi
	undo_suspend
}

undo_copy_dump () {
	$SSH "root@$host" "rm -f $VE_DUMPFILE"
	undo_suspend
}

undo_stop () {
	if [ "$state" = "running" ]; then
		$VZCTL start $VEID
	elif [ "$mounted" = "mounted" ]; then
		$VZCTL mount $VEID
	fi
	undo_sync
}

undo_source_stage() {
	if [ $online -eq 1 ]; then
		undo_copy_dump
	else
		undo_stop
	fi
}

undo_quota_dump () {
	rm -f "$VE_QUOTADUMP"
	undo_source_stage
}

undo_copy_quota () {
	$SSH "root@$host" "rm -f $VE_QUOTADUMP"
	undo_quota_dump
}

undo_undump () {
	logexec 2 $SSH root@$host $VZCTL restore $VEID --kill
	undo_copy_quota
}

get_status() {
	exist=$3
	mounted=$4
	state=$5
}

get_time () {
	awk -v t2=$2 -v t1=$1 'BEGIN{print t2-t1}'
}

get_ploop_info() {
	local dev top img root private

	root=$(readlink -f "$VE_ROOT")
	private=$(readlink -f "$VE_PRIVATE")
	dev=$(awk '$2=="'$root'" {print $1}' /proc/mounts | \
		sed -e 's|^/dev/||' -e 's|p1$||') || return 1
	top=$(cat /sys/block/${dev}/pstate/top) || return 1
	img=$(cat /sys/block/${dev}/pdelta/${top}/image) || return 1
	TOP_DELTA=$(echo $img | sed "s|^${private}/||") || return 1
	PLOOP_DEV="${dev}"
}

# Copy top delta with write tracker and CT stop/suspend
ploop_copy() {
	local cmd dev delta cat size

	cmd=$*
	dev=/dev/$PLOOP_DEV
	delta=$VE_PRIVATE/$TOP_DELTA
	# Sanity checks
	test -b $dev || return 1
	test -f $delta || return 1

	cat=cat
	# Use nice progress bar with pv if we can
	if test $verbose -gt 0 && pv -V >/dev/null 2>&1; then
		size=$(du -b $delta | awk '{print $1}')
		test -n "$size" && cat="pv -s $size"
	fi
	ploop copy -s $dev -F "$cmd 1>&2" | $cat | \
		$SSH "root@$host" ploop copy -d $delta
}

print_times() {
	local fmt='  %20s: %6.2f\n'
	local dt_susp_dump dt_pcopy

	[ $times -eq 0 ] && return

	echo
	dt_susp_dump=$(get_time $time_suspend $time_copy_dump)
	if [ "$PLOOP" = "yes" ]; then
		dt_pcopy=$(get_time $time_suspend $time_pcopy)
		dt_susp_dump=$(get_time $dt_pcopy $dt_susp_dump)
	fi
	printf "$fmt" "Suspend + Dump" $dt_susp_dump
	if [ "$PLOOP" = "yes" ]; then
		printf "$fmt" "Pcopy after suspend" $dt_pcopy
	fi
	printf "$fmt" "Copy dump file" $(get_time $time_copy_dump $time_rsync2)
	if [ "$PLOOP" != "yes" ]; then
		printf "$fmt" "Second rsync" $(get_time $time_rsync2 $time_quota)
		printf "$fmt" "2nd level quota" $(get_time $time_quota $time_undump)
	fi
	printf "$fmt" "Undump + Resume" $(get_time $time_undump $time_finish)
	printf "  %20s  ------\n" " "
	printf "$fmt" "Total suspended time" $(get_time $time_suspend $time_finish)
	echo
}

check_cpt_props() {
	local version caps

	log 2 "Checking for CPT version compatibility"
	version=$(vzcptcheck version)
	if [ $? -ne 0 -o "$version" = "" ]; then
		log 1 "Warning: can't get local CPT version, skipping check"
	elif ! logexec 1 $SSH root@$host vzcptcheck version $version; then
		log 0 "Error: CPT version check failed on destination node!"
		log 0 "Destination node kernel is too old, please upgrade"
		log 0 "Can't continue live migration"
		exit $MIG_ERR_UNSUP_CPT_VER
	fi

	log 2 "Checking for CPU flags compatibility"
	caps=$($SSH root@$host vzcptcheck caps)
	if [ $? -ne 0 -o "$caps" = "" ]; then
		log 1 "Warning: can't get remote CPU caps, skipping check"
	elif ! logexec 1 vzcptcheck caps $VEID $caps; then
		log 0 "Error: CPU capabilities check failed!"
		log 0 "Destination node CPU is not compatible"
		log 0 "Can't continue live migration"
		exit $MIG_ERR_UNSUP_CPU
	fi
}

if [ $# -lt 2 ]; then
	usage
fi

while [ ! -z "$1" ]; do
	case "$1" in
	--live|--online)
		online=1
		;;
	-v)
		verbose=$((verbose+1)) # can just be 'let verbose++' in bash
		;;
	-vv)
		verbose=$((verbose+2))
		;;
	-vvv)
		verbose=$((verbose+3))
		;;
	-vvvv)
		verbose=$((verbose+4))
		;;
	--remove-area|-r)
		shift
		if [ "$1" = "yes" ]; then
			remove_area=1
		elif [ "$1" = "no" ]; then
			remove_area=0
		else
			usage
		fi
		;;
	--keep-dst)
		keep_dst=1
		;;
	-c|--compact)
		compact=1
		;;
	-s|--snapshot)
		snapshot=1
		;;
	--check-only|--dry-run)
		check_only=1
		;;
	--ssh=*)
		SSH_OPTIONS="$SSH_OPTIONS $(echo $1 | cut -c7-)"
		SCP_OPTIONS="`echo $SSH_OPTIONS | sed 's/-p/-P/1'`"
		;;
	--rsync=*)
		RSYNC_OPTIONS="$RSYNC_OPTIONS $(echo $1 | cut -c9-)"
		;;
	--times|-t)
		times=1
		;;
	*)
		break
		;;
	esac
	shift
done

if [ $verbose -gt 0 ]; then
	times=1
fi
if [ $verbose -gt 1 ]; then

	RSYNC_OPTIONS="$RSYNC_OPTIONS -v"
	VZCTL="$VZCTL --verbose"
fi
if [ $verbose -gt 2 ]; then
	RSYNC_OPTIONS="$RSYNC_OPTIONS -v"
	VZCTL="$VZCTL --verbose"
fi
if [ $verbose -gt 3 ]; then
	RSYNC_OPTIONS="$RSYNC_OPTIONS -v"
	VZCTL="$VZCTL --verbose"
	SSH_OPTIONS="$SSH_OPTIONS -v"
	SCP_OPTIONS="$SCP_OPTIONS -v"
fi

RSYNC="rsync $RSYNC_OPTIONS"
SSH="ssh $SSH_OPTIONS"
SCP="scp $SCP_OPTIONS"
export RSYNC_RSH="$SSH"

host=$1
shift
VEID=$1
shift

if [ -z "$host" -o -z "$VEID" -o $# -ne 0 ]; then
	usage
fi

# Support CT names as well
if echo $VEID | egrep -qv '^[[:digit:]]+$'; then
	VEID=$(vzlist -o ctid -H $VEID | tr -d ' ')
	if [ -z "$VEID" ]; then
		# Error message is printed by vzlist to stderr
		exit $MIG_ERR_NOEXIST
	fi
fi

vpsconf="$confdir/$VEID.conf"

if [ ! -r "$vzconf" ]; then
	log 0 "Can't read global config file $vzconf"
	exit $MIG_ERR_NOEXIST
fi

get_status $($VZCTL status $VEID)
if [ "$exist" = "deleted" ]; then
	log 0 "CT #$VEID doesn't exist"
	exit $MIG_ERR_NOEXIST
fi

S="Starting"
M="migration"
[ $check_only -eq 1 ] && S="Checking"
[ $online -eq 1 ] && M="live migration"
log 1 "$S $M of CT $VEID to $host"
unset S M

# Try to connect to destination
if ! logexec 2 $SSH root@$host /bin/true; then
	log 0 "Can't connect to destination address using public key"
	log 0 "Please put your public key to destination node"
	exit $MIG_ERR_CANT_CONNECT
fi

# Check if OpenVZ is running
if ! logexec 2 $SSH root@$host /etc/init.d/vz status ; then
	log 0 "OpenVZ is not running on the target machine"
	log 0 "Can't continue migration"
	exit $MIG_ERR_OVZ_NOT_RUNNING
fi

# Check if CPT modules are loaded for live migration
if [ $online -eq 1 ]; then
	if [ ! -f /proc/cpt ]; then
		log 0 "vzcpt module is not loaded on the source node"
		log 0 "Can't continue live migration"
		exit $MIG_ERR_OVZ_NOT_RUNNING
	fi
	if ! logexec 2 $SSH root@$host "test -f /proc/rst";
	then
		log 0 "vzrst module is not loaded on the destination node"
		log 0 "Can't continue live migration"
		exit $MIG_ERR_OVZ_NOT_RUNNING
	fi
fi

dst_exist=$($SSH "root@$host" "$VZCTL status $VEID" | awk '{print $3}')
if [ "$dst_exist" = "exist" ]; then
	log 0 "CT #$VEID already exists on destination node"
	exit $MIG_ERR_EXISTS
fi

if [ $online -eq 1 -a "$state" != "running" ]; then
	log 0 "Can't perform live migration of a stopped container"
	exit $MIG_ERR_VPS_IS_STOPPED
fi

if [ $online -eq 1 ]; then
	if ! which vzcptcheck >/dev/null; then
		log 1 "Warning: no vzcptcheck binary on local node,"
		log 1 "skipping CPT version and CPU caps checks"
	elif ! logexec 1 $SSH root@$host which vzcptcheck >/dev/null; then
		log 1 "Warning: no vzcptcheck binary on destination node,"
		log 1 "skipping CPT version and CPU caps checks"
	else
		check_cpt_props
	fi
fi

log 2 "Loading $vzconf and $vpsconf files"

. "$vzconf"
. "$vpsconf"
VE_DUMPFILE="$tmpdir/dump.$VEID"
VE_QUOTADUMP="$tmpdir/quotadump.$VEID"

DDXML=$VE_PRIVATE/root.hdd/DiskDescriptor.xml
if [ -f $DDXML ]; then
	PLOOP=yes
	# Disable vzquota operations for ploop CT
	DISK_QUOTA=no
fi

if [ "$PLOOP" = "yes" ]; then
	log 2 "Checking if ploop is supported on destination node"
	if ! logexec 2 $SSH "root@$host" ploop getdev ; then
		log 0 "Destination node does not support ploop, can't migrate"
		exit $MIG_ERR_UNSUP_PLOOP
	fi

	if [ "$state" = "running" ]; then
		# Online ploop migration: exclude top delta
		if ! get_ploop_info; then
			log 0 "Can't get ploop information"
			exit $MIG_ERR_COPY
		fi
		RSYNC1_OPTIONS="--exclude $TOP_DELTA --delete-excluded"
	fi
else
	# non-ploop case: can use sparse rsync, can't do compact/snapshot
	RSYNC1_OPTIONS="--sparse"
	compact=0
	snapshot=0
fi

# Check that IP_ADDRESSes are not in use on dest
# remove extra spaces and netmasks
IP_X=$(echo $IP_ADDRESS " " | sed 's@/[0-9][0-9]*[[:space:]]@ @g')
log 2 "Checking IPs on destination node: $IP_X"
# remove extra spaces, replace spaces with |
IP_X=$(echo $IP_X | sed 's/ /|/g')
# do check
if [ $($SSH "root@$host" "grep -cwE \"$IP_X\" /proc/vz/veip") -gt 0 ]; then
	log 0 "IP address(es) already in use on destination node"
	exit $MIG_ERR_IP_IN_USE
fi

if [ $check_only -eq 1 ]; then
	exit 0
fi

log 1 "Preparing remote node"

log 2 "Copying config file"
if ! logexec 2 $SCP $vpsconf root@$host:$vpsconf ; then
	log 0 "Failed to copy config file"
	exit $MIG_ERR_COPY
fi

logexec 2 $SSH root@$host $VZCTL set $VEID --applyconfig_map name --save
RET=$?
# vzctl return code 20 or 21 in case of unrecognized option
if [ $RET != 20 ] && [ $RET != 21 ] && [ $RET != 0 ]; then
	log 0 "Failed to apply config on destination node"
	undo_conf
	exit $MIG_ERR_APPLY_CONFIG
fi

for sfx in $ACT_SCRIPTS_SFX; do
	file="$confdir/$VEID.$sfx"
	if [ -f "$file" ]; then
		act_scripts="$act_scripts $file"
	fi
done
if [ -n "$act_scripts" ]; then
	log 2 "Copying action scripts"
	if ! logexec 2 $SCP $act_scripts root@$host:$confdir ; then
		log 0 "Failed to copy action scripts"
		undo_conf
		exit $MIG_ERR_COPY
	fi
fi

log 2 "Creating remote container root dir"
if ! $SSH "root@$host" "mkdir -p $VE_ROOT"; then
	log 0 "Failed to make container root directory"
	undo_act_scripts
	exit $MIG_ERR_COPY
fi

log 2 "Creating remote container private dir"
if ! $SSH "root@$host" "mkdir -p $VE_PRIVATE"; then
	log 0 "Failed to make container private area directory"
	undo_private
	exit $MIG_ERR_COPY
fi

if [ "${DISK_QUOTA}" != "no" ]; then
	log 1 "Initializing remote quota"

	log 2 "Quota init"
	if ! $SSH "root@$host" "$VZCTL quotainit $VEID"; then
		log 0 "Failed to initialize quota"
		undo_root
		exit $MIG_ERR_QUOTA
	fi

	log 2 "Turning remote quota on"
	if ! $SSH "root@$host" "$VZCTL quotaon $VEID"; then
		log 0 "Failed to turn quota on"
		undo_quota_init
		exit $MIG_ERR_QUOTA
	fi
fi

if [ "$compact" -eq 1 ]; then
	log 1 "Compacting container image"
	if ! logexec 2 vzctl compact $VEID ; then
		log 0 "Failed to compact container image"
		undo_root
	fi
fi

if [ "$snapshot" -eq 1 ]; then
	log 1 "Creating a container snapshot"
	if ! logexec 2 $VZCTL snapshot $VEID ; then
		log 0 "Failed to snapshot a container"
		undo_root
	fi
fi

log 1 "Syncing private"
$RSYNC $RSYNC1_OPTIONS "$VE_PRIVATE" "root@$host:${VE_PRIVATE%/*}"
RET=$?
# Ignore rsync error 24 "Partial transfer due to vanished source files"
if [ $RET != 24 ] && [ $RET != 0 ]; then
	log 0 "Failed to sync container private areas"
	undo_quota_on
	exit $MIG_ERR_COPY
fi

if [ $online -eq 1 ]; then
	log 1 "Live migrating container..."

	if [ "$PLOOP" != "yes" ]; then
		time_suspend=$(date +%s.%N)
		log 2 "Suspending container"
		if ! logexec 2 $VZCTL chkpnt $VEID --suspend ; then
			log 0 "Failed to suspend container"
			undo_sync
			exit $MIG_ERR_CHECKPOINT
		fi
	else
		log 2 "Copying top ploop delta with CT suspend"
		TMPF=$(mktemp)
		if ! ploop_copy "date '+%s.%N' > $TMPF; $VZCTL chkpnt $VEID --suspend"; then
			log 0 "Failed to copy top ploop delta"
			$VZCTL chkpnt $VEID --resume 2>/dev/null
			rm -f $TMPF
			undo_sync
			exit $MIG_ERR_COPY
		fi
		time_pcopy=$(date +%s.%N)
		time_suspend=$(cat $TMPF)
		rm $TMPF
	fi

	log 2 "Dumping container"
	if ! logexec 2 $VZCTL chkpnt $VEID --dump --dumpfile $VE_DUMPFILE ; then
		log 0 "Failed to dump container"
		undo_suspend
		exit $MIG_ERR_CHECKPOINT
	fi

	log 2 "Copying dumpfile"
	time_copy_dump=$(date +%s.%N)
	if ! logexec 2 $SCP $VE_DUMPFILE root@$host:$VE_DUMPFILE ; then
		log 0 "Failed to copy dump"
		undo_dump
		exit $MIG_ERR_COPY
	fi
else
	if [ "$state" = "running" ]; then
		if [ "$PLOOP" != "yes" ]; then
			log 1 "Stopping container"
			if ! logexec 2 $VZCTL stop $VEID ; then
				log 0 "Failed to stop container"
				undo_sync
				exit $MIG_ERR_STOP_SOURCE
			fi
		else
			log 1 "Copying top ploop delta with CT stop"
			if ! ploop_copy "$VZCTL stop $VEID --skip-umount"; then
				log 0 "Failed to copy top ploop delta"
				$VZCTL start $VEID 2>/dev/null
				undo_sync
				exit $MIG_ERR_COPY
			fi
			log 1 "Unmounting container"
			if ! logexec 2 $VZCTL umount $VEID ; then
				log 0 "Failed to umount container"
				$VZCTL start $VEID 2>/dev/null
				undo_sync
				exit $MIG_ERR_STOP_SOURCE
			fi
		fi
	elif [ "$mounted" = "mounted" ]; then
		log 1 "Unmounting container"
		if ! logexec 2 $VZCTL umount $VEID ; then
			log 0 "Failed to umount container"
			undo_sync
			exit $MIG_ERR_STOP_SOURCE
		fi
	fi
fi

time_rsync2=$(date +%s.%N)
if [ "$state" = "running" -a "$PLOOP" != "yes" ]; then
	log 2 "Syncing private (2nd pass)"
	if ! $RSYNC "$VE_PRIVATE" "root@$host:${VE_PRIVATE%/*}"; then
		log 0 "Failed to sync container private areas"
		undo_source_stage
		exit $MIG_ERR_COPY
	fi
fi

time_quota=$(date +%s.%N)
if [ "${DISK_QUOTA}" != "no" ]; then
	log 1 "Syncing 2nd level quota"

	log 2 "Dumping 2nd level quota"
	if ! vzdqdump $VEID -U -G -T -F > "$VE_QUOTADUMP"; then
		log 0 "Failed to dump 2nd level quota"
		undo_quota_dump
		exit $MIG_ERR_QUOTA
	fi

	log 2 "Copying 2nd level quota"
	if ! logexec 2 $SCP $VE_QUOTADUMP root@$host:$VE_QUOTADUMP ; then
		log 0 "Failed to copy 2nd level quota dump"
		undo_quota_dump
		exit $MIG_ERR_COPY
	fi

	log 2 "Loading 2nd level quota"
	if ! $SSH "root@$host" "(vzdqload $VEID -U -G -T -F < $VE_QUOTADUMP &&
			vzquota reload2 $VEID)"; then
		log 0 "Failed to load 2nd level quota"
		undo_copy_quota
		exit $MIG_ERR_QUOTA
	fi
fi

if [ $online -eq 1 ]; then
	log 2 "Undumping container"
	time_undump=$(date +%s.%N)
	if ! logexec 2 $SSH root@$host $VZCTL restore $VEID --undump \
			--dumpfile $VE_DUMPFILE --skip_arpdetect ; then
		log 0 "Failed to undump container"
		undo_copy_quota
		exit $MIG_ERR_RESTORE_VPS
	fi

	log 2 "Resuming container"
	if ! logexec 2 $SSH root@$host $VZCTL restore $VEID --resume ; then
		log 0 "Failed to resume container"
		undo_undump
		exit $MIG_ERR_RESTORE_VPS
	fi
	time_finish=$(date +%s.%N)
	print_times
	log 1 "Cleaning up"

	log 2 "Killing container"
	logexec 2 $VZCTL chkpnt $VEID --kill
	logexec 2 $VZCTL umount $VEID

	log 2 "Removing dumpfiles"
	rm -f "$VE_DUMPFILE"
	$SSH "root@$host" "rm -f $VE_DUMPFILE"
else
	if [ "$state" = "running" ]; then
		log 1 "Starting container"
		if ! logexec 2 $SSH root@$host $VZCTL start $VEID ; then
			log 0 "Failed to start container"
			undo_copy_quota
			exit $MIG_ERR_START_VPS
		fi
	elif [ "$mounted" = "mounted" ]; then
		log 1 "Mounting container"
		if ! logexec 2 $SSH root@$host $VZCTL mount $VEID ; then
			log 0 "Failed to mount container"
			undo_copy_quota
			exit $MIG_ERR_MOUNT_VPS
		fi
	elif [ "${DISK_QUOTA}" != "no" ]; then
		log 1 "Turning quota off"
		if ! logexec 2 $SSH root@$host vzquota off $VEID ; then
			log 0 "failed to turn quota off"
			undo_copy_quota
			exit $MIG_ERR_QUOTA
		fi
	fi

	log 1 "Cleaning up"
fi

if [ $remove_area -eq 1 ]; then
	log 2 "Destroying container"
	logexec 2 $VZCTL destroy $VEID
else
	# Move config as veid.migrated to allow backward migration
	mv -f $vpsconf $vpsconf.migrated
fi
