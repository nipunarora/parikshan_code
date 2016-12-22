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

if [ ! -d "${ODIR}1" ]; then
    echo "wrong param"
    exit 1
fi

HTMLDIR=$1
shift

if [ "x$HTMLDIR" = "x" ]; then
    HTMLDIR=`basename $ODIR`
fi
export HTMLDIR

cd `dirname $ODIR`

THINGS=`ls -1 -d $HTMLDIR* | grep -v vole | grep -v "^$HTMLDIR$"`

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

if [ -s ${HTMLDIR}1/gnuplottmp/timings.avg ]; then
    cat <<EOF >> ~/public_html/$HTMLDIR/index.html
<h2>Queries</h2>
<table>
<thead><tr><td>Testname</td><td>Min latency</td><td>Max latency</td><td>Avg latency</td><td>Std. var.</td></tr></thead>
EOF

    XBAR=$(cat ${HTMLDIR}1/gnuplottmp/timings.avg)
    S_XBAR=$(perl -ne 'chomp; print if 5..5' ${HTMLDIR}1/gnuplottmp/timings.std)
    THRPUT_NORMAL=$(perl -anle 'print $_/5 if 2..2' ${HTMLDIR}1/gnuplottmp/timings.std)

    for i in $THINGS; do
        echo "$i $(perl -pe 'chomp' $i/gnuplottmp/timings.min)" >> $TMPD/timings.min
        echo "$i $(perl -pe 'chomp' $i/gnuplottmp/timings.max)" >> $TMPD/timings.max
        YBAR=$(perl -pe 'chomp' $i/gnuplottmp/timings.avg)
        S_YBAR=$(perl -ne 'chomp; print if 5..5' $i/gnuplottmp/timings.std)
        COUNT=$(perl -ne 'chomp; print if 2..2' $i/gnuplottmp/timings.std)
        TOTWEIGHT=$(perl -le '$ARGV[0]=~/$ENV{HTMLDIR}(\d)/; if ($1 == 4){print"20"}elsif($1==5){print"21"}else{print"5"}' $i)
        SCALED_COUNT=$(perl -e 'print $ARGV[0]/$ARGV[1]/$ARGV[2]' $COUNT $TOTWEIGHT $THRPUT_NORMAL)
        SCALED_YBAR=$(perl -e 'print $ARGV[0]/$ARGV[1]' $XBAR $YBAR)
        SCALED_S_YBAR=$(perl -e 'my ($YBAR,$XBAR,$S_YBAR,$S_XBAR)=@ARGV; print ( ($XBAR**2)/($YBAR**2)*1.96*($S_XBAR*$S_XBAR * $YBAR*$YBAR + $S_YBAR*$S_YBAR * $XBAR*$XBAR)/($XBAR**4))' $YBAR $XBAR $S_YBAR $S_XBAR)
        echo "$i $YBAR $S_YBAR $SCALED_COUNT $SCALED_YBAR $SCALED_S_YBAR" >> $TMPD/timings.avg

        echo "<tr><td>$i</td><td>" >> ~/public_html/$HTMLDIR/index.html
        cat $i/gnuplottmp/timings.min >> ~/public_html/$HTMLDIR/index.html
        echo "</td><td>" >> ~/public_html/$HTMLDIR/index.html
        cat $i/gnuplottmp/timings.max >> ~/public_html/$HTMLDIR/index.html
        echo "</td><td>" >> ~/public_html/$HTMLDIR/index.html
        cat $i/gnuplottmp/timings.avg >> ~/public_html/$HTMLDIR/index.html
        echo "</td><td>+/- " >> ~/public_html/$HTMLDIR/index.html
        tail -n1 $i/gnuplottmp/timings.std >> ~/public_html/$HTMLDIR/index.html
        echo "</td></tr>" >> ~/public_html/$HTMLDIR/index.html
    done
#    echo "$HTMLDIR 1 1 0 0" >> $TMPD/timings.avg
    cat -b $TMPD/timings.avg > $TMPD/timings2.avg
    perl -ane 'print if 2..eof' $TMPD/timings2.avg > $TMPD/timings3.avg

    echo "</table>" >> ~/public_html/$HTMLDIR/index.html
    
    
    XTICS=$(perl -e 'print join ",", map {@_=split;$_[1]=~s/$ENV{HTMLDIR}(\d)?(?:_b_(\w+))?/$1?"$2($1)":""/e;"\"".($_[0]%2==0?"\\n":"")."$_[1]\" ".($_[0]-1)} <>' $TMPD/timings3.avg)
    XRANGE=$( wc -l $TMPD/timings3.avg | perl -anle 'print $F[0]+1')

    echo "" >$TMP
# 1.96 is a secret statistical constant :-)
    cat <<EOF >>$TMP
reset
set terminal $FORMATGP
set output "$TMPD/images/timings.err2.$FORMAT"
set title "Response Time and Throughput compared to 1 node run"
set ylabel "Speedup ratio"
set xlabel ""
set bmargin 4
set xtics border nomirror ($XTICS)
set xrange [0:$XRANGE]
set yrange [0:*]
onedown(x)=x-1
plot "$TMPD/timings3.avg"  using (onedown(\$1)):6:7 title "Response time" with errorbars, "$TMPD/timings3.avg"  using (onedown(\$1)):5 title "Throughput" with points
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
else
    cat <<EOF >> ~/public_html/$HTMLDIR/index.html
<h2>Begins/commits</h2>
<table>
<thead><tr><td>Testname</td><td>Min latency</td><td>Max latency</td><td>Avg latency</td></tr></thead>
EOF

    for i in $THINGS; do
        echo "<tr><td>$i</td><td>" >> ~/public_html/$HTMLDIR/index.html
        cat $i/gnuplottmp/timings.bmin >> ~/public_html/$HTMLDIR/index.html
        echo "</td><td>" >> ~/public_html/$HTMLDIR/index.html
        cat $i/gnuplottmp/timings.bmax >> ~/public_html/$HTMLDIR/index.html
        echo "</td><td>" >> ~/public_html/$HTMLDIR/index.html
        cat $i/gnuplottmp/timings.bavg >> ~/public_html/$HTMLDIR/index.html
        echo "</td></tr>" >> ~/public_html/$HTMLDIR/index.html

        echo "$i $(cat $i/gnuplottmp/timings.bmin)" >> $TMPD/timings.bmin
        echo "$i $(cat $i/gnuplottmp/timings.bmax)" >> $TMPD/timings.bmax
        echo "$i $(cat $i/gnuplottmp/timings.bavg)" >> $TMPD/timings.bavg
    done

    echo "</table>" >> ~/public_html/$HTMLDIR/index.html
fi


cat <<EOF >> ~/public_html/$HTMLDIR/index.html
</body>
</html>
EOF

mv $TMPD/images ~/public_html/$HTMLDIR
chmod -R a+rX ~/public_html/$HTMLDIR

echo "$HTMLDIR plots done"

rm -f $TMP
#echo $TMPD
#rm -rf $TMPD
