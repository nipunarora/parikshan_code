#!/usr/bin/perl -l
use strict;
use warnings;
my $server = $ARGV[0];
#print "Generating $server data files ...";

open MYSQL, ">", "gnuplottmp/$server.mysql.dat" || die $!;
open(G, "$server.mymonitor") || die $!;
open(H, ">$server.mymonitor2") || die $!;
my $num=0;
while (<G>) {
  next if /Binlog Dump|Sleep|processlist|Command|system user|---------------/;
  chomp;
  print H $_;
  if (!$_) {
    print MYSQL $num;
    $num=0;
  }
  else {
    $num++;
  }
}
print MYSQL $num if $num;
close MYSQL;
close G;
close H;
system("mv $server.mymonitor2 $server.mymonitor");

open (Q, "queries") || die $!;
my @TIMINGSQ;
my $line =0;
my $ip = join".",unpack "C4", (gethostbyname($server))[-1];
my $maxlat = -1;
my $minlat = 999999;
my $avglat = 0;
my $avgcnt = 0;
my $sumlat2 = 0;
while (<Q>) {
  chomp;
  next unless $_;
  print $line if ++$line%25000==0;
  /^(\d+) (\d) ([^ ]+) (\d+.\d+) "([^"]*)"$/;
  warn "$1,$2,$3,$4,$5 bad line $_" unless $3;
  next unless $3 eq $server || $3 eq $ip;
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
}
close Q;
open (TIMINGSQ, ">gnuplottmp/$server.timings.select") || die $!;
if ($avgcnt) {
  print TIMINGSQ "$_ ".(100*($TIMINGSQ[$_]||0)/$avgcnt) for (0 .. @TIMINGSQ);
}
close TIMINGSQ;

open (TIMINGSQ, ">gnuplottmp/$server.timings.max") || die $!;
print TIMINGSQ $maxlat if $maxlat != -1;;
close TIMINGSQ;

open (TIMINGSQ, ">gnuplottmp/$server.timings.min") || die $!;
print TIMINGSQ $minlat if $minlat != 999999;
close TIMINGSQ;

open (TIMINGSQ, ">gnuplottmp/$server.timings.avg") || die $!;
print TIMINGSQ $avglat/$avgcnt if $avgcnt;
close TIMINGSQ;

open (TIMINGSQ, ">gnuplottmp/$server.timings.std") || die $!;
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


open(F, "$server.monitor") || die $!;

my $network_max_bandwidth_in_byte = 10000000;
my $network_max_packet_per_second = 1000000;
my $n = '';
my %opened;

open CPUUSER, ">", "gnuplottmp/$server.cpu.user.dat" || die $!;
open CPUSYSTEM, ">", "gnuplottmp/$server.cpu.system.dat" || die $!;
open CPUNICE, ">", "gnuplottmp/$server.cpu.nice.dat" || die $!;
open CPUBUSY, ">", "gnuplottmp/$server.cpu.busy.dat" || die $!;
open PROC, ">", "gnuplottmp/$server.proc.dat" || die $!;
open CTX, ">", "gnuplottmp/$server.ctxsw.dat" || die $!;
open DISKTPS, ">", "gnuplottmp/$server.disk.tps.dat" || die $!;
open DISKRTPS, ">", "gnuplottmp/$server.disk.rtps.dat" || die $!;
open DISKWTPS, ">", "gnuplottmp/$server.disk.wtps.dat" || die $!;
open DISKBRDPS, ">", "gnuplottmp/$server.disk.brdps.dat" || die $!;
open DISKBWRPS, ">", "gnuplottmp/$server.disk.bwrps.dat" || die $!;
open KBMEMFREE, ">", "gnuplottmp/$server.mem.kbmemfree.dat" || die $!;
open KBMEMUSED, ">", "gnuplottmp/$server.mem.kbmemused.dat" || die $!;
#open MEMUSED, ">", "gnuplottmp/$server.mem.memused.dat" || die $!;
open KBCACHED, ">", "gnuplottmp/$server.mem.kbcached.dat" || die $!;
#open SWPUSED, ">", "gnuplottmp/$server.mem.swpused.dat" || die $!;
open TOTSCK, ">", "gnuplottmp/$server.sock.totsck.dat" || die $!;
open TCPSCK, ">", "gnuplottmp/$server.sock.tcpsck.dat" || die $!;

my @eth;
my @ETH;
my $lines = 0;
my $cpu5 = 0;
while (<F>) {
  next if /^Average/;
  chomp;
  my @F = split/\s+/;
  shift @F;

  if ($F[0] && ($F[0] eq 'AM' || $F[0] eq 'PM')) {
    shift @F;
  }
  next unless $F[0];

  if ($F[0] eq 'CPU') {
    $cpu5 = 1 if /iowait/;
  }

  if ($F[0] eq 'all') {
    # This is the cpu info
    print CPUUSER $F[1];
    print CPUNICE $F[2];
    print CPUSYSTEM $F[3];
    print CPUBUSY $F[1] + $F[2] + $F[3];
    #print CPUIDLE ($cpu5 ? $F[5] : $F[4]);
    next;
  }

  if ($F[0] =~ /eth(\d+)/) {
    # This is the ethX network info
    my $fh1;
    my $fh2;
    my $fh3;
    my $fh4;
    if (!defined $eth[$1]) {
      open $fh1, ">", "gnuplottmp/$server.net$1.rxpck.dat" || die $!;
      open $fh2, ">", "gnuplottmp/$server.net$1.txpck.dat" || die $!;
      open $fh3, ">", "gnuplottmp/$server.net$1.rxbyt.dat" || die $!;
      open $fh4, ">", "gnuplottmp/$server.net$1.txbyt.dat" || die $!;
      $eth[$1] = [$fh1,$fh2,$fh3,$fh4];
    }
    else {
      ($fh1,$fh2,$fh3,$fh4) = @{$eth[$1]};
    }
    if (!int($F[1]) && !int($F[2]) && !int($F[3]) && !int($F[4]) && defined $ETH[$1]) {
      print $fh1 $ETH[$1][0];
      print $fh2 $ETH[$1][1];
      print $fh3 $ETH[$1][2];
      print $fh4 $ETH[$1][3];
      undef $ETH[$1];
    }
    else {
      print $fh1 $F[1];
      print $fh2 $F[2];
      print $fh3 $F[3]/(1024**2);
      print $fh4 $F[4]/(1024**2);
      $ETH[$1] = [$F[1], $F[2], $F[3]/(1024**2), $F[4]/(1024**2)];
    }
    next;
  }

  # Detect which is the next info to be parsed
  if ($F[0] =~ /proc|cswch|tps|kbmemfree|totsck/) {
    $n = $F[0];
    next;
  }

  # Only get lines with numbers (real data !)
  if ($F[0] =~ /[0-9]/) {
    if ($n eq 'proc/s') {
      last if ++$lines == $ARGV[1];
      # This is the proc/s info
      print PROC $F[0];
      $n = '';
    }
    elsif ($n eq 'cswch/s') {
      # This is the context switches per second info
      print CTX $F[0];
      $n = '';
    }
    elsif ($n eq 'tps') {
      # This is the disk info

      print DISKTPS $F[0];
      print DISKRTPS $F[1];
      print DISKWTPS $F[2];
      print DISKBRDPS $F[3];
      print DISKBWRPS $F[4];
      $n = '';
    }
    elsif ($n eq 'kbmemfree') {
      # This is the mem info
      # Amount of free memory available in kilobytes.
      print KBMEMFREE $F[0];
      # Amount of used memory in kilobytes. This does not take into account memory used by the kernel itself.
      print KBMEMUSED $F[1]-$F[5];
      # Percentage of used memory.
      #print MEMUSED $F[2];
      # Amount of memory used to cache data by the kernel in kilobytes.
      print KBCACHED $F[5];
      # Percentage of used swap space.
      #print SWPUSED $F[8];
      #     print xxx > FILENAME".mem.kbmemshrd.dat"; # Amount of memory shared by the system in kilobytes.  Always zero with 2.4 kernels.
      #     print xxx > FILENAME".mem.kbbuffers.dat"; # Amount of memory used as buffers by the kernel in kilobytes.
      #     print xxx > FILENAME".mem.kbswpfree.dat"; # Amount of free swap space in kilobytes.
      #     print xxx > FILENAME".mem.kbswpused.dat"; # Amount of used swap space in kilobytes.
      $n = '';
    }
    elsif ($n eq 'totsck') {
      # This is the socket info
      # Total number of used sockets.
      print TOTSCK $F[0];
      # Number of TCP sockets currently in use.
      print TCPSCK $F[1];
      #     print xxx > FILENAME".sock.udpsck.dat"; # Number of UDP sockets currently in use.
      #     print xxx > FILENAME".sock.rawsck.dat"; # Number of RAW sockets currently in use.
      #     print xxx > FILENAME".sock.ip-frag.dat"; # Number of IP fragments currently in use.
      $n = '';
    }
  }
}

#print " ... data files done";

