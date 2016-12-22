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

export SOMEALGORITHM=smcpu3to1

HTMLDIR=all
export HTMLDIR

cd bench

TMPD=$HTMLDIR
rm -rf $TMPD
mkdir $TMPD
mkdir $TMPD/images

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

# cat <<EOF >> ~/public_html/$HTMLDIR/index.html
# <h2>Queries</h2>
# <table>
# <thead><tr><td>algorithm</td><td>Testname</td><td>Avg latency compared to one node</td><td>Std. var.</td><td>Throughput compared to one node</td></tr></thead>
# EOF

GRAPHS=
LASTTEST=
THINGS=`ls -1 -d *_li | grep -v vole | grep -v latency | grep "_b_"`
for i in $THINGS; do
    for alg in ji jis li rr wrandom  wrr; do
        i=$(perl -e '$ARGV[0]=~s/[^_]+$//;print $ARGV[0]' $i)$alg
        TEST=$(perl -e '$ARGV[0]=~s/\d_b_\w+//;print $ARGV[0]' $i)
        TESTW_HOST_CNT=$alg$(perl -e '$ARGV[0]=~s/.*(\d)_b_\w+/($1)/;print $ARGV[0]' $i)
        DATA=$(grep $i $TEST/timings3.avg | perl -ane 'print "@F[2..$#F]"')
        echo $i $DATA >> $TMPD/${TEST}timings.avg
    done
    if [ "x$LASTTEST" != "x$TEST" ]; then
        GRAPHS="$GRAPHS $TEST"
        LASTTEST=$TEST
    fi
    cat -b $TMPD/${TEST}timings.avg > $TMPD/${TEST}timings2.avg
done

XRANGE=$( wc -l $TMPD/${SOMEALGORITHM}timings2.avg | perl -anle 'print $F[0]+1')
XTICS=$(perl -e 'print join ",", map {@_=split;$_[1]=~s/$ENV{SOMEALGORITHM}(\d)?(?:_b_(\w+))?/$1?"$2($1)":""/e;"\"".($_[0]%2==0?"\\n":"")."$_[1]\" ".($_[0])} <>' $TMPD/${SOMEALGORITHM}timings2.avg)

for test in `seq 1 $(( $XRANGE - 1 ))`; do
    AVG=$(perl -ne "print if s/^\s+$test\s+//" $TMPD/*timings2.avg | perl -lane '$cnt++;$sum1+=$F[4];$sum2+=$F[3];END{print $sum1/$cnt." ".$sum2/$cnt." $cnt"}')
    AVG2=$(perl -ne "print if s/^\s+$test\s+//" $TMPD/*timings2.avg | grep -v smdisk100 | perl -lane '$cnt++;$sum1+=$F[4];$sum2+=$F[3];END{print $sum1/$cnt." ".$sum2/$cnt." $cnt"}')
    echo "$test $AVG $AVG2" >> $TMPD/avg
    #using 1:6:7 title "${test}" with errorbars\\
done

#    echo "$HTMLDIR 1 1 0 0" >> $TMPD/timings.avg
#echo "</table>" >> ~/public_html/$HTMLDIR/index.html

echo "" >$TMP

# 1.96 is a secret statistical constant :-)
cat <<EOF >>$TMP
reset
set terminal $FORMATGP
set output "$TMPD/images/timings.err2.$FORMAT"
set title "Response Time compared to 1 node run"
set ylabel "Normalized speedup ratio"
set xlabel ""
set bmargin 4
set xtics border nomirror ($XTICS)
set xrange [0:$XRANGE]
set yrange [0:*]
EOF

FIRST=0
for test in $GRAPHS; do
#for test in smdisk3to1; do
    if [ "x$FIRST" = "x1" ]; then
        echo ", \\" >>$TMP
    else
        echo "plot \\" >>$TMP
        FIRST=1
    fi
    cat <<EOF >>$TMP
"$TMPD/${test}timings2.avg"  using 1:6:7 title "${test}" with errorbars\\
EOF
done
echo "" >>$TMP

cat <<EOF >>$TMP
reset
set terminal $FORMATGP
set output "$TMPD/images/speedup.$FORMAT"
set title ""
set ylabel "Normalized speedup ratio"
set xlabel ""
set bmargin 4
set xtics border nomirror ($XTICS)
set xrange [0:$XRANGE]
set yrange [0:1.2]
plot "$TMPD/avg" using 1:2 title "Response time" with points, "$TMPD/avg" using 1:3 title "Throughput" with points
EOF

cat <<EOF >>$TMP
reset
set terminal $FORMATGP
set output "$TMPD/images/speedup_wo_smdisk100.$FORMAT"
set title ""
set ylabel "Normalized speedup ratio"
set xlabel ""
set bmargin 4
set xtics border nomirror ($XTICS)
set xrange [0:$XRANGE]
set yrange [0:1.2]
plot "$TMPD/avg" using 1:5 title "Response time" with points, "$TMPD/avg" using 1:6 title "Throughput" with points
EOF

# 1.96 is a secret statistical constant :-)
cat <<EOF >>$TMP
reset
set terminal $FORMATGP
set output "$TMPD/images/throughput.err2.$FORMAT"
set title "Throughput compared to 1 weight on one node"
set ylabel "Normalized speedup ratio"
set xlabel ""
set bmargin 4
set xtics border nomirror ($XTICS)
set xrange [0:$XRANGE]
set yrange [0:*]
EOF

FIRST=0
for test in $GRAPHS; do
    if [ "x$FIRST" = "x1" ]; then
        echo ", \\" >>$TMP
    else
        echo "plot \\" >>$TMP
        FIRST=1
    fi
    cat <<EOF >>$TMP
"$TMPD/${test}timings2.avg"  using 1:5 title "${test}" with points\\
EOF
done
echo "" >>$TMP

$GNUPLOT -persist $TMP

if [ -e "$TMPD/images/speedup.$FORMAT" ]; then
    echo "<img src=\"images/speedup.$FORMAT\"/>" >> ~/public_html/$HTMLDIR/index.html
fi;

if [ -e "$TMPD/images/speedup_wo_smdisk100.$FORMAT" ]; then
    echo "<img src=\"images/speedup_wo_smdisk100.$FORMAT\"/>" >> ~/public_html/$HTMLDIR/index.html
fi;

if [ -e "$TMPD/images/timings.err2.$FORMAT" ]; then
    echo "<img src=\"images/timings.err2.$FORMAT\"/>" >> ~/public_html/$HTMLDIR/index.html
fi

if [ -e "$TMPD/images/throughput.err2.$FORMAT" ]; then
    echo "<img src=\"images/throughput.err2.$FORMAT\"/>" >> ~/public_html/$HTMLDIR/index.html
fi

cat <<EOF >> ~/public_html/$HTMLDIR/index.html
</body>
</html>
EOF

mv $TMPD/images ~/public_html/$HTMLDIR
chmod -R a+rX ~/public_html/$HTMLDIR

echo "$HTMLDIR plots done"

rm -f $TMP
