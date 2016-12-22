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

if [ ! -d "bench/mediumcpu4_b_${ODIR}" ]; then
    echo "wrong param"
    exit 1
fi

HTMLDIR=$ODIR
export HTMLDIR

cd bench

TMPD=$HTMLDIR
rm -rf $TMPD
mkdir $TMPD
mkdir $TMPD/images

THINGS=`ls -1 -d *_$HTMLDIR | grep -v vole | grep -v latency | grep -v "^$HTMLDIR$"`

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

cat <<EOF >> ~/public_html/$HTMLDIR/index.html
<h2>Queries</h2>
<table>
<thead><tr><td>Testname</td><td>Avg latency compared to one node</td><td>Std. var.</td><td>Throughput compared to one node</td></tr></thead>
EOF

CNT=0
XTICS=

for i in $THINGS; do
    TEST=$(perl -e '$ARGV[0]=~s/\d_b_\w+//;print $ARGV[0]' $i)
    #echo $TEST $i
    TESTW_HOST_CNT=$(perl -e '$ARGV[0]=~s/(\d)_b_\w+/($1)/;print $ARGV[0]' $i)
    DATA=$(grep $i $TEST/timings3.avg | perl -ane 'print "@F[2..$#F]"')
    echo $i $DATA >> $TMPD/timings.avg

    CNT=$(( $CNT + 1 ))
    EVEN=$(perl -le 'print $ARGV[0]%3' $CNT)
    if [ "x$EVEN" = "x0" ]; then
        GNUPLOTNEWLINE=""
    else
        if [ "x$EVEN" = "x1" ]; then
            GNUPLOTNEWLINE="\\n"
        else
            GNUPLOTNEWLINE="\\n\\n"
        fi
    fi
    XTICS="$XTICS, \"$GNUPLOTNEWLINE$TESTW_HOST_CNT\" $CNT"
    
    echo "<tr><td>$TESTW_HOST_CNT</td><td>" >> ~/public_html/$HTMLDIR/index.html
    echo $DATA | perl -ane 'print $F[3]' >> ~/public_html/$HTMLDIR/index.html
    echo "</td><td>+/- " >> ~/public_html/$HTMLDIR/index.html
    echo $DATA | perl -ane 'print $F[4]' >> ~/public_html/$HTMLDIR/index.html
    echo "</td><td>" >> ~/public_html/$HTMLDIR/index.html
    echo $DATA | perl -ane 'print $F[2]' >> ~/public_html/$HTMLDIR/index.html
    echo "</td></tr>" >> ~/public_html/$HTMLDIR/index.html
done
XTICS=$(echo $XTICS | perl -pe 's|^, ||')

#    echo "$HTMLDIR 1 1 0 0" >> $TMPD/timings.avg
cat -b $TMPD/timings.avg > $TMPD/timings2.avg
echo "</table>" >> ~/public_html/$HTMLDIR/index.html

    
echo "" >$TMP

XRANGE=$( wc -l $TMPD/timings2.avg | perl -anle 'print $F[0]+1')

# 1.96 is a secret statistical constant :-)
cat <<EOF >>$TMP
reset
set terminal $FORMATGP
set output "$TMPD/images/timings.err2.$FORMAT"
set title "Response Time and Throughput compared to 1 node run"
set ylabel "Normalized speedup ratio"
set xlabel ""
set bmargin 4
set ytics 0.1
set xtics border nomirror ($XTICS)
set xrange [0:$XRANGE]
set yrange [0:2]
plot "$TMPD/timings2.avg"  using 1:6:7 title "Response time" with errorbars, "$TMPD/timings2.avg"  using 1:5 title "Throughput" with points
EOF

$GNUPLOT -persist $TMP

if [ -e "$TMPD/images/timings.select.$FORMAT" ]; then
    echo "<img src=\"images/timings.select.$FORMAT\"/>" >> ~/public_html/$HTMLDIR/index.html
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
