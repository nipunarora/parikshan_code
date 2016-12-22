#!/usr/bin/perl -l
use warnings;
use strict;
use Storable;

$ARGV[0] || die;

open F, $ARGV[0] || die $!;
my $lines = `wc -l $ARGV[0]`;
$lines =~ s/.*?(\d+).*/$1/;
$lines/=20;
my @tran = ();
my %tran = ();
my $last = 0;
my $trnr = 1;
my $line = 0;
open G, ">", "out.txt";
while (<F>) {
  $line++;
  if ($line%$lines == 0) {
    print "$line";
  }
  if (m/^(\d{6}\s+\d{1,2}:\d\d:\d\d\s+|\s+)(\d+)\s+Query\s+(select.+)$/i) { # get connection ID($2) and querystring
    #print "S1-$1--$2--$3------\n";
    if (!$tran{$2}) {
      $tran{$2} = ++$last;
    }
    print G "xxx xxx S $tran{$2} $3";
  }
  elsif (m/^(\d{6}\s+\d{1,2}:\d\d:\d\d\s+|\s+)(\d+)\s+Query\s+(update.+)$/i) { # get connection ID($2) and querystring
    #print "S2--$1--$2--$3------\n";
    if (!$tran{$2}) {
      $tran{$2} = ++$last;
    }
    print G "xxx xxx W $tran{$2} $3";
  }
  elsif (m/^(\d{6}\s+\d{1,2}:\d\d:\d\d\s+|\s+)(\d+)\s+Query\s+(.+)$/i) { # get connection ID($2)
    if ($3 eq 'BEGIN') { #new begin as in new tran
      $tran{$2} = ++$last;
      print G "xxx xxx B $tran{$2}";
    }
    elsif ($3 eq 'COMMIT' && $tran{$2}) {
      print G "xxx xxx C $tran{$2}";
      delete $tran{$2};
    }
    elsif ($3 eq 'ROLLBACK' && $tran{$2}) {
      print G "xxx xxx R $tran{$2}";
      delete $tran{$2};
    }
    elsif ($3 ne 'COMMIT' && $3 ne 'BEGIN') {
      warn "unknown line $_";
    }
  }
  elsif (m/^(\d{6}\s+\d{1,2}:\d\d:\d\d\s+|\s+)(\d+)\s+Init\s+(.+)$/i) { # get connection ID($2)
    #$tran{$2} = ++$last;
    #print G "xxx xxx B $tran{$2}";
  }
  elsif (m/^(\d{6}\s+\d{1,2}:\d\d:\d\d\s+|\s+)(\d+)\s+Connect.+\s+on\s+(.*)$/i) { # get connection ID($2) and database($3)
    #ignore
  }
  elsif (m/^(\d{6}\s+\d{1,2}:\d\d:\d\d\s+|\s+)(\d+)\s+Connect.+$/i) { # get connection ID($2) and database($3)
    #ignore
  }
  elsif (m/^(\d{6}\s+\d{1,2}:\d\d:\d\d\s+|\s+)(\d+)\s+Change user .*\s+on\s+(.*)$/i) { # get connection ID($2) and database($3)
    #ignore
  }
  elsif (m/^(\d{6}\s+\d{1,2}:\d\d:\d\d\s+|\s+)(\d+)\s+Quit\s+$/i) { # remove connection ID($2) and querystring
    #ignore
  }
  elsif (m/^(\d{6}\s+\d{1,2}:\d\d:\d\d\s+|\s+)(\d+)\s+Statistics\s+(.*)$/i) { # get connection ID($2) and info?
    #ignore
  }
  elsif (m/^(\d{6}\s+\d{1,2}:\d\d:\d\d\s+|\s+)(\d+)\s+Refresh\s+(.+)$/i) { # get connection ID($2)
    #ignore
  }
  elsif (m/^(\d{6}\s+\d{1,2}:\d\d:\d\d\s+|\s+)(\d+)\s+Field\s+(.+)$/i) { # get connection ID($2)
    #ignore
  }
  elsif (m/^\s+(.+)$/ ) { # command could be some lines ...
    warn "multi-lined ($1)\n";
    #my ($A)=$1;
    #chomp $A;
    #@{$tran{$last}}[-1] .= " $1";
  }
}
close F;
close G;

#print "Nr trans ".scalar(@tran)."\n";
#print "Nr uncom trans ".scalar(keys %tran)."\n";
