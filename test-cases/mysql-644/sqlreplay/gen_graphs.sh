#!/bin/bash

TMP=`mktemp /tmp/gen_graphs.XXXXXX`
GNUPLOT=/usr/bin/gnuplot
#GNUPLOT=cat
FORMAT=png
if [ "x$FORMAT" = "xeps" ]; then
    FORMATGP="postscript eps"
else
    FORMATGP=$FORMAT
fi

OPWD=`pwd`

ODIR=$1
shift

if [ ! -d "$ODIR" ]; then
    echo "wrong param"
    exit 1
fi

HTMLDIR=$1
shift

if [ "x$HTMLDIR" = "xnogen" ]; then
    DONTGEN=$HTMLDIR
    HTMLDIR=
fi

if [ "x$HTMLDIR" = "x" ]; then
    HTMLDIR=`basename $ODIR`
fi


cd $ODIR

rm -rf ~/public_html/$HTMLDIR
mkdir -p ~/public_html/$HTMLDIR
cat <<EOF > ~/public_html/$HTMLDIR/index.html
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
        "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html>
<head>
<title>$HTMLDIR</title>
</head>
<body>
EOF

NRTHREAD=$(( `grep nr_threads params.log | perl -aple '$_=$F[-1]'` + 1 ))

RAMUP=`grep rampuptime params.log | perl -aple '$_=$F[-1]'`
RUNUP=$(( `grep runtime params.log | perl -aple '$_=$F[-1]'` + $RAMUP ))
RAMDN=$(( `grep rampdowntime params.log | perl -aple '$_=$F[-1]'` + $RUNUP ))

if [ "x$DONTGEN" = "x" ]; then
    rm -rf gnuplottmp
    mkdir gnuplottmp
fi
mkdir images >/dev/null 2>&1

if [ "x$DONTGEN" = "x" ]; then
    perl -l <<'EOF'
open (Q, "queries") || die $!;
my @TIMINGSQ;
my @TIMINGSB;
my @TIMINGST;
my $line =0;
my $maxlat = -1;
my $minlat = 999999;
my $avglat = 0;
my $avgcnt = 0;
my $bmaxlat = -1;
my $bminlat = 999999;
my $bavglat = 0;
my $bavgcnt = 0;
my $sumlat2 = 0;
while (<Q>) {
  chomp;
  next unless $_;
  print $line if ++$line%25000==0;
  /^(\d+) (\d) ([^ ]+) (\d+.\d+) "([^"]*)"$/;
  warn "$1,$2,$3,$4,$5 bad line $_" unless $3;
  my ($client, $type, $host, $response, $query) = ($1,$2,$3,$4,$5);
  #$response *= 1000; #convert to millisec
  if ($type==3){
    $TIMINGSQ[$response]++;
    $avgcnt++;
    $avglat += $response;
    $sumlat2 += $response*$response;
    $maxlat = $response if $response > $maxlat;
    $minlat = $response if $response < $minlat;
  }
  else {
    $TIMINGSB[$response*10]++;
    $bavgcnt++;
    $bavglat += $response;
    $bmaxlat = $response if $response > $bmaxlat;
    $bminlat = $response if $response < $bminlat;
  }
  $TIMINGST[$response]++;
}
close Q;

open (TIMINGSQ, ">gnuplottmp/timings.select") || die $!;
if ($avgcnt) {
  print TIMINGSQ "$_ ".(100*($TIMINGSQ[$_]||0)/$avgcnt) for (0 .. @TIMINGSQ);
}
close TIMINGSQ;

open (TIMINGSQ, ">gnuplottmp/timings.max") || die $!;
print TIMINGSQ $maxlat if $maxlat != -1;;
close TIMINGSQ;

open (TIMINGSQ, ">gnuplottmp/timings.min") || die $!;
print TIMINGSQ $minlat if $minlat != 999999;
close TIMINGSQ;

open (TIMINGSQ, ">gnuplottmp/timings.avg") || die $!;
print TIMINGSQ $avglat/$avgcnt if $avgcnt;
close TIMINGSQ;

open (TIMINGSQ, ">gnuplottmp/timings.std") || die $!;
if ($avgcnt) {
  my $SAK = $sumlat2 - ($avglat*$avglat)/$avgcnt;
  print TIMINGSQ $SAK;
  print TIMINGSQ $avgcnt;
  my $s2 = $SAK/($avgcnt-1);
  print TIMINGSQ $s2;
  my $s = sqrt($s2);
  print TIMINGSQ $s;
  my $se = $s/sqrt($avgcnt);
  print TIMINGSQ $se;
}
close TIMINGSQ;


open (TIMINGSB, ">gnuplottmp/timings.basics") || die $!;
if ($avgcnt) {
  print TIMINGSB "$_ ".(100*($TIMINGSB[$_]||0)/$avgcnt) for (0 .. @TIMINGSB);
}
close TIMINGSB;

open (TIMINGSB, ">gnuplottmp/timings.bmax") || die $!;
print TIMINGSB $bmaxlat if $bmaxlat != -1;;
close TIMINGSB;

open (TIMINGSB, ">gnuplottmp/timings.bmin") || die $!;
print TIMINGSB $bminlat if $bminlat != 999999;
close TIMINGSB;

open (TIMINGSB, ">gnuplottmp/timings.bavg") || die $!;
print TIMINGSB $bavglat/$bavgcnt if $bavgcnt;
close TIMINGSB;



open (TIMINGST, ">gnuplottmp/timings.total") || die $!;
if ($avgcnt) {
  print TIMINGST "$_ ".(100*($TIMINGST[$_]||0)/$avgcnt) for (0 .. @TIMINGST);
}
close TIMINGST;

EOF
fi

cat <<EOF >> ~/public_html/$HTMLDIR/index.html
<table>
<tr><td>Hostname</td><td>Selects</td><td>Min latency</td><td>Max latency</td><td>Avg latency</td><td>Std varians</td></tr>
EOF

if [ "x$DONTGEN" = "x" ]; then
    for server in `grep 'monitor\b' params.log | perl -aple '$_=$F[-1]' | xargs` ; do
        $OPWD/gen_plots.pl $server $RAMDN
    done
fi

MAXLATENCY=$(tail -n1 gnuplottmp/*.timings.select | sort -n | tail -n1 | perl -anle 'print $F[0]')
CEILLAT="*"
#CEILLAT=$(perl -ne 'print if 2 .. eof' gnuplottmp/timings.select | sort -n -k 2 -r | head -n1 | perl -anle 'print $F[-1]')

for server in `grep 'monitor\b' params.log | perl -aple '$_=$F[-1]' | xargs` ; do
    #truncate
    cat <<EOF >$TMP
EOF
    
    #echo "Active mysql queries graph"
    if [ -s "gnuplottmp/$server.mysql.dat" -a "x$(( $(uniq gnuplottmp/$server.mysql.dat | wc -l) ))" != "x1" ]; then
        cat <<EOF >>$TMP
reset
set terminal $FORMATGP
set output "images/$server.mysql.$FORMAT"
set title "Active mysql queries"
set yrange [0:$NRTHREAD]
set xlabel "Time in seconds"
set ylabel "Active mysql queries"
set arrow from $RAMUP, graph 0 to $RAMUP, graph 1 nohead lt 7
set arrow from $RUNUP, graph 0 to $RUNUP, graph 1 nohead lt 7
plot "gnuplottmp/$server.mysql.dat"  title "$server" with lines
EOF
    fi

    #echo "Generating $server CPU total/user/system/nice time graph"
    if [ "$( sort -n gnuplottmp/$server.cpu.busy.dat | perl -nle 'print "ok" if eof && $_ > 3' )" = "ok" ]; then
    cat <<EOF >>$TMP
reset
set terminal $FORMATGP
set output "images/$server.cpu_user_kernel.$FORMAT"
set title "Total/User/Kernel/Nice processor usage"
set xlabel "Time in seconds"
set ylabel "Processor usage in %"
set yrange [0:100]
set arrow from $RAMUP, graph 0 to $RAMUP, graph 1 nohead lt 7
set arrow from $RUNUP, graph 0 to $RUNUP, graph 1 nohead lt 7
plot "gnuplottmp/$server.cpu.busy.dat"  title "$server total" with lines, "gnuplottmp/$server.cpu.user.dat"  title "$server user" with lines, "gnuplottmp/$server.cpu.system.dat"  title "$server kernel" with lines, "gnuplottmp/$server.cpu.nice.dat"  title "$server nice" with lines
EOF
    fi

  # Plot Processes/second
    #echo "Generating $server Processes/second graph"
    if [ "$( sort -n gnuplottmp/$server.proc.dat | perl -nle 'print "ok" if eof && $_ > 10' )" = "ok" ]; then
    cat <<EOF >>$TMP
reset
set terminal $FORMATGP
set output "images/$server.procs.$FORMAT"
set title "Processes created"
set xlabel "Time in seconds"
set ylabel "Processes created per second"
set yrange [0:*]
set arrow from $RAMUP, graph 0 to $RAMUP, graph 1 nohead lt 7
set arrow from $RUNUP, graph 0 to $RUNUP, graph 1 nohead lt 7
plot "gnuplottmp/$server.proc.dat"  title "$server" with lines
EOF
    fi

  # Plot Context switches/second
    #echo "Generating $server Context switches/second graph"
    if [ "$( sort -n gnuplottmp/$server.ctxsw.dat | perl -nle 'print "ok" if eof && $_ > 100' )" = "ok" ]; then
    cat <<EOF >>$TMP
reset
set terminal $FORMATGP
set output "images/$server.ctxtsw.$FORMAT"
set title "Context switches"
set xlabel "Time in seconds"
set ylabel "Context switches per second"
set yrange [0:*]
set arrow from $RAMUP, graph 0 to $RAMUP, graph 1 nohead lt 7
set arrow from $RUNUP, graph 0 to $RUNUP, graph 1 nohead lt 7
plot "gnuplottmp/$server.ctxsw.dat"  title "$server" with lines
EOF
    fi
  # Plot Disk total transfers
    #echo "Generating $server Disk total transfers graph"
    if [ "$( sort -n gnuplottmp/$server.disk.tps.dat | perl -nle 'print "ok" if eof && $_ > 100' )" = "ok" ]; then
    cat <<EOF >>$TMP
reset
set terminal $FORMATGP
set output "images/$server.disk_tps.$FORMAT"
set title "Disk transfers"
set xlabel "Time in seconds"
set ylabel "Disk transfers per second"
set yrange [0:*]
set arrow from $RAMUP, graph 0 to $RAMUP, graph 1 nohead lt 7
set arrow from $RUNUP, graph 0 to $RUNUP, graph 1 nohead lt 7
plot "gnuplottmp/$server.disk.tps.dat"  title "$server" with lines
EOF
    fi

  # Plot disk read/write requests
    #echo "Generating $server disk read/write requests graph"
    if [ "$( sort -n gnuplottmp/$server.disk.rtps.dat | perl -nle 'print "ok" if eof && $_ > 100' )" = "ok" -o "$( sort -n gnuplottmp/$server.disk.wtps.dat | perl -nle 'print "ok" if eof && $_ > 100' )" = "ok" ]; then
    cat <<EOF >>$TMP
reset
set terminal $FORMATGP
set output "images/$server.disk_rw_req.$FORMAT"
set title "Disk read/write requests"
set xlabel "Time in seconds"
set ylabel "Requests per second"
set yrange [0:*]
set arrow from $RAMUP, graph 0 to $RAMUP, graph 1 nohead lt 7
set arrow from $RUNUP, graph 0 to $RUNUP, graph 1 nohead lt 7
plot "gnuplottmp/$server.disk.rtps.dat"  t "$server read" with lines, "gnuplottmp/$server.disk.wtps.dat"  t "$server write" with lines
EOF
    fi

  # Plot disk blocks read/write requests
    #echo "Generating $server disk blocks read/write requests graph"
    if [ "$( sort -n gnuplottmp/$server.disk.brdps.dat | perl -nle 'print "ok" if eof && $_ > 500' )" = "ok" -o "$( sort -n gnuplottmp/$server.disk.bwrps.dat | perl -nle 'print "ok" if eof && $_ > 500' )" = "ok" ]; then
    cat <<EOF >>$TMP
reset
set terminal $FORMATGP
set output "images/$server.disk_rw_blk.$FORMAT"
set title " Disk read/write blocks"
set xlabel "Time in seconds"
set ylabel "Blocks per second"
set yrange [0:*]
set arrow from $RAMUP, graph 0 to $RAMUP, graph 1 nohead lt 7
set arrow from $RUNUP, graph 0 to $RUNUP, graph 1 nohead lt 7
plot "gnuplottmp/$server.disk.brdps.dat"  t "$server read" with lines, "gnuplottmp/$server.disk.bwrps.dat"  t "$server write" with lines
EOF
    fi

#   # Plot Memory usage
#     #echo "Generating $server Memory usage graph"
#     if [ "x$( uniq gnuplottmp/$server.mem.kbmemused.dat | sort -n | perl -nle 'BEGIN {$c= scalar <>} if (eof){$d=$_; print \"ok\" if $d-$c > 10000}' )" = "xok" ]; then
#     cat <<EOF >>$TMP
# reset
# set terminal $FORMATGP
# set output "images/$server.mem_usage.$FORMAT"
# set title "Memory usage"
# set xlabel "Time in seconds"
# set ylabel "Amount of memory in KB"
# set yrange [0:*]
# set arrow from $RAMUP, graph 0 to $RAMUP, graph 1 nohead lt 7
# set arrow from $RUNUP, graph 0 to $RUNUP, graph 1 nohead lt 7
# plot "gnuplottmp/$server.mem.kbmemused.dat"  title "$server" with lines
# EOF
#     fi

  # Plot Memory & cache usage
    #echo "Generating $server Memory & cache usage graph"
    if [ "$( sort -n gnuplottmp/$server.mem.kbmemused.dat | perl -nle 'BEGIN {$c= scalar <>} print "ok" if eof && $_-$c > 10000' )" = "ok" -o "$( sort -n gnuplottmp/$server.mem.kbcached.dat | perl -nle 'BEGIN {$c= scalar <>} print "ok" if eof && $_-$c > 10000' )" = "ok" ]; then
    cat <<EOF >>$TMP
reset
set terminal $FORMATGP
set output "images/$server.mem_cache.$FORMAT"
set title "Memory & cache usage"
set xlabel "Time in seconds"
set ylabel "Amount of memory in KB"
set yrange [0:*]
set arrow from $RAMUP, graph 0 to $RAMUP, graph 1 nohead lt 7
set arrow from $RUNUP, graph 0 to $RUNUP, graph 1 nohead lt 7
plot "gnuplottmp/$server.mem.kbmemused.dat"  title "$server memory" with lines, "gnuplottmp/$server.mem.kbcached.dat"  title "$server cache" with lines
EOF
    fi

#   # Plot network received/transmitted packets
#     #echo "Generating $server network received/transmitted packets graph"
#     cat <<EOF >>$TMP
# reset
# set terminal $FORMATGP
# set output "images/$server.net_rt_pack.$FORMAT"
# set title "Network received/transmitted packets"
# set xlabel "Time in seconds"
# set ylabel "Packets per second"
# set yrange [0:*]
# set arrow from $RAMUP, graph 0 to $RAMUP, graph 1 nohead lt 7
# set arrow from $RUNUP, graph 0 to $RUNUP, graph 1 nohead lt 7
# EOF
#     for eth in `seq 0 3`; do
#         if [ -e "gnuplottmp/$server.net$eth.rxpck.dat" ]; then
#             if [ ! "$eth" = "0" ]; then
#                 echo ", \\" >> $TMP
#             else
#                 echo "plot \\" >> $TMP
#             fi
#             cat  <<EOF >> $TMP
# "gnuplottmp/$server.net$eth.rxpck.dat"  title "$server eth$eth received" with lines, "gnuplottmp/$server.net$eth.txpck.dat"  title "$server eth$eth transmitted" with lines\\
# EOF
#         fi
#     done;
#     echo "" >> $TMP

  # Plot network received/transmitted bytes
    #echo "Generating $server network received/transmitted Mbytes graph"
    NETOK=0;
    for eth in `seq 0 3`; do
        if [ -e "gnuplottmp/$server.net$eth.rxbyt.dat" ]; then
            if [ "$( sort -n gnuplottmp/$server.net$eth.rxbyt.dat | perl -nle 'print "ok" if eof && $_ > 0.25' )" = "ok" -o "$( sort -n gnuplottmp/$server.net$eth.txbyt.dat | perl -nle 'print "ok" if eof && $_ > 0.25' )" = "ok" ]; then
                NETOK=1;
            fi
        fi
    done;
    if [ "x$NETOK" = "x1" ]; then
    cat <<EOF >>$TMP
reset
set terminal $FORMATGP
set output "images/$server.net_rt_byt.$FORMAT"
set title "Network received/transmitted Mbytes"
set xlabel "Time in seconds"
set ylabel "Mbytes per second"
set yrange [0:*]
set arrow from $RAMUP, graph 0 to $RAMUP, graph 1 nohead lt 7
set arrow from $RUNUP, graph 0 to $RUNUP, graph 1 nohead lt 7
EOF
    for eth in `seq 0 3`; do
        if [ -e "gnuplottmp/$server.net$eth.rxbyt.dat" ]; then
            if [ ! "$eth" = "0" ]; then
                echo ", \\" >>$TMP
            else
                echo "plot \\" >>$TMP
            fi
            cat <<EOF >>$TMP
"gnuplottmp/$server.net$eth.rxbyt.dat"  title "$server eth$eth received" with lines, "gnuplottmp/$server.net$eth.txbyt.dat"  title "$server eth$eth transmitted" with lines\\
EOF
        fi
    done;
    echo "" >> $TMP
    fi

if [ -s gnuplottmp/$server.timings.select -a "x$( wc -l gnuplottmp/$server.timings.select| perl -anle 'print $F[0]')" != "x1" ]; then
    cat <<EOF >>$TMP
reset
set terminal $FORMATGP
set output "images/$server.timings.select.$FORMAT"
set title "Response time"
set xlabel "Response Time in seconds"
set yrange [0:*]
set xrange [0:$MAXLATENCY]
set ylabel "Frequency in percent"
plot "gnuplottmp/$server.timings.select" title "Percentage of response within this second" with steps, "" smooth bezier title "Smoothed" with lines
EOF
fi

  # Plot Sockets usage
    if [ "$( sort -n gnuplottmp/$server.sock.totsck.dat | perl -nle 'BEGIN {$c= scalar <>} print "ok" if eof && $_-$c > 5' )" = "ok" ]; then
    cat <<EOF >>$TMP
reset
set terminal $FORMATGP
set output "images/$server.socks.$FORMAT"
set title "Sockets"
set xlabel "Time in seconds"
set ylabel "Number of sockets"
set arrow from $RAMUP, graph 0 to $RAMUP, graph 1 nohead lt 7
set arrow from $RUNUP, graph 0 to $RUNUP, graph 1 nohead lt 7
plot "gnuplottmp/$server.sock.totsck.dat"  title "$server" with lines
EOF
    fi
    $GNUPLOT  -persist $TMP

    cat <<EOF > ~/public_html/$HTMLDIR/index_$server.html
<html>
<head>
<title>$HTMLDIR $server</title>
</head>
<body>
EOF

    ANYPICS=0
    for img in "images/$server.mysql.$FORMAT" "images/$server.cpu_user_kernel.$FORMAT" "images/$server.procs.$FORMAT" "images/$server.ctxtsw.$FORMAT" "images/$server.disk_tps.$FORMAT" "images/$server.disk_rw_req.$FORMAT" "images/$server.disk_rw_blk.$FORMAT" "images/$server.mem_usage.$FORMAT" "images/$server.mem_cache.$FORMAT" "images/$server.net_rt_pack.$FORMAT" "images/$server.net_rt_byt.$FORMAT" "images/$server.socks.$FORMAT"; do
        if [ -e $img ]; then
            echo "<img src=\"$img\"/>" >> ~/public_html/$HTMLDIR/index_$server.html
            ANYPICS=1
        fi
    done;

    if [ "x$ANYPICS" = "x1" -a -e images/$server.timings.select.$FORMAT ]; then
        echo "<img src=\"images/$server.timings.select.$FORMAT\"/>" >> ~/public_html/$HTMLDIR/index_$server.html
    fi

    cat <<EOF >> ~/public_html/$HTMLDIR/index_$server.html
</body>
</html>
EOF
    if [ "x$ANYPICS" = "x1" ]; then
        cat <<EOF >> ~/public_html/$HTMLDIR/index.html
<tr><td><a href="index_$server.html">$server</a></td><td>
EOF
        perl -ne 'print if 2..2' gnuplottmp/$server.timings.std >> ~/public_html/$HTMLDIR/index.html
        echo "</td><td>" >> ~/public_html/$HTMLDIR/index.html
        cat gnuplottmp/$server.timings.min >> ~/public_html/$HTMLDIR/index.html
        echo "</td><td>" >> ~/public_html/$HTMLDIR/index.html
        cat gnuplottmp/$server.timings.max >> ~/public_html/$HTMLDIR/index.html
        echo "</td><td>" >> ~/public_html/$HTMLDIR/index.html
        cat gnuplottmp/$server.timings.avg >> ~/public_html/$HTMLDIR/index.html
        echo "</td><td>" >> ~/public_html/$HTMLDIR/index.html
        perl -ne 'print if 5..5' gnuplottmp/$server.timings.std >> ~/public_html/$HTMLDIR/index.html
        echo "</td></tr>" >> ~/public_html/$HTMLDIR/index.html
        
    fi

    echo "Plot $HTMLDIR $server"

done
echo "</table>" >> ~/public_html/$HTMLDIR/index.html

cat <<EOF >> ~/public_html/$HTMLDIR/index.html
<table>
<tr><td>Query type</td><td>Count</td><td>Min latency</td><td>Max latency</td><td>Avg latency</td><td>Std varians</td></tr>
EOF
echo "<tr><td>Selects" >> ~/public_html/$HTMLDIR/index.html
echo "</td><td>" >> ~/public_html/$HTMLDIR/index.html
perl -ne 'print if 2..2' gnuplottmp/timings.std >> ~/public_html/$HTMLDIR/index.html
echo "</td><td>" >> ~/public_html/$HTMLDIR/index.html
cat gnuplottmp/timings.min >> ~/public_html/$HTMLDIR/index.html
echo "</td><td>" >> ~/public_html/$HTMLDIR/index.html
cat gnuplottmp/timings.max >> ~/public_html/$HTMLDIR/index.html
echo "</td><td>" >> ~/public_html/$HTMLDIR/index.html
cat gnuplottmp/timings.avg >> ~/public_html/$HTMLDIR/index.html
echo "</td><td>" >> ~/public_html/$HTMLDIR/index.html
perl -ne 'print if 5..5' gnuplottmp/timings.std >> ~/public_html/$HTMLDIR/index.html
echo "</td></tr>" >> ~/public_html/$HTMLDIR/index.html

echo "<tr><td>Begins/commits" >> ~/public_html/$HTMLDIR/index.html
echo "</td><td>N/A" >> ~/public_html/$HTMLDIR/index.html
echo "</td><td>" >> ~/public_html/$HTMLDIR/index.html
cat gnuplottmp/timings.bmin >> ~/public_html/$HTMLDIR/index.html
echo "</td><td>" >> ~/public_html/$HTMLDIR/index.html
cat gnuplottmp/timings.bmax >> ~/public_html/$HTMLDIR/index.html
echo "</td><td>" >> ~/public_html/$HTMLDIR/index.html
cat gnuplottmp/timings.bavg >> ~/public_html/$HTMLDIR/index.html
echo "</td><td>" >> ~/public_html/$HTMLDIR/index.html
echo "</td></tr>" >> ~/public_html/$HTMLDIR/index.html

echo "</table>" >> ~/public_html/$HTMLDIR/index.html

echo "" >$TMP

if [ -s gnuplottmp/timings.select -a "x$( wc -l gnuplottmp/timings.select| perl -anle 'print $F[0]')" != "x1" ]; then
    cat <<EOF >>$TMP
reset
set terminal $FORMATGP
set output "images/timings.select.$FORMAT"
set title "Selects response time"
set xlabel "Response Time in seconds"
set yrange [0:$CEILLAT]
set ylabel "Frequency in percentage"
plot "gnuplottmp/timings.select" smooth bezier title "Percentage of response within this second for ALL nodes" with lines\\
EOF
    for server in `grep 'monitor\b' params.log | perl -aple '$_=$F[-1]' | xargs` ; do
        if [ -s gnuplottmp/$server.timings.select -a "x$( wc -l gnuplottmp/$server.timings.select| perl -anle 'print $F[0]')" != "x1" ]; then
            echo ", \"gnuplottmp/$server.timings.select\" smooth bezier title \"... for $server\" with lines\\" >>$TMP
        fi
    done
    echo "" >> $TMP
fi

cat <<EOF >>$TMP
reset
set terminal $FORMATGP
set output "images/mysql.$FORMAT"
set title "Active mysql queries"
set xlabel "Time in seconds"
set yrange [0:*]
set ylabel "Active mysql queries"
set arrow from $RAMUP, graph 0 to $RAMUP, graph 1 nohead lt 7
set arrow from $RUNUP, graph 0 to $RUNUP, graph 1 nohead lt 7
EOF
ONEMYSQL=0
for server in `grep 'monitor\b' params.log | perl -aple '$_=$F[-1]' | xargs` ; do
    if [ -s "gnuplottmp/$server.mysql.dat" -a "x$(( $(uniq gnuplottmp/$server.mysql.dat | wc -l) ))" != "x1" ]; then
        if [ "x$ONEMYSQL" = "x1" ]; then
            echo ", \\" >>$TMP
        else
            echo "plot \\" >>$TMP
            ONEMYSQL=1
        fi
        echo "\"gnuplottmp/$server.mysql.dat\" smooth csplines title \"$server\" with lines\\" >>$TMP
    fi
done
echo "" >> $TMP
$GNUPLOT  -persist $TMP

rm $TMP

if [ "x$ONEMYSQL" = "x1" ]; then
    echo "<img src=\"images/mysql.$FORMAT\"/>" >> ~/public_html/$HTMLDIR/index.html
fi

if [ -e "images/timings.select.$FORMAT" ]; then
    echo "<img src=\"images/timings.select.$FORMAT\"/>" >> ~/public_html/$HTMLDIR/index.html
fi;

mv images ~/public_html/$HTMLDIR

echo "<pre>" >> ~/public_html/$HTMLDIR/index.html

cat params.log >> ~/public_html/$HTMLDIR/index.html

cat <<EOF >> ~/public_html/$HTMLDIR/index.html
</pre>
</body>
</html>
EOF

chmod -R a+rX ~/public_html/$HTMLDIR

echo "$HTMLDIR plot done"
