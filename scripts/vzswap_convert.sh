# !/bin/bash
# 
# Filename: vzswap_convert.sh
# Author: Nipun Arora
# Created: Fri Nov 28 11:31:43 2014 (-0500)
# URL: http://www.nipunarora.net 
# 
# Description: 
#
# This script converts a typical OpenVZ container to a VSwap Container
# Usage 
#

CTID= $1
RAM= $2
SWAP= $3
CFG=/etc/vz/conf/${CTID}.conf
cp $CFG $CFG.pre-vswap
grep -Ev '^(KMEMSIZE|LOCKEDPAGES|PRIVVMPAGES|SHMPAGES|NUMPROC|PHYSPAGES|VMGUARPAGES|OOMGUARPAGES|NUMTCPSOCK|NUMFLOCK|NUMPTY|NUMSIGINFO|TCPSNDBUF|TCPRCVBUF|OTHERSOCKBUF|DGRAMRCVBUF|NUMOTHERSOCK|DCACHESIZE|NUMFILE|AVNUMPROC|NUMIPTENT|ORIGIN_SAMPLE|SWAPPAGES)=' > $CFG <  $CFG.pre-vswap
vzctl set $CTID --ram $RAM --swap $SWAP --save
vzctl set $CTID --reset_ub
