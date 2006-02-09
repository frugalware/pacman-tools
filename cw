#!/usr/bin/perl -w

# NOTES:
#  -b can be used only with '' if multiple arguments given (no Getopt::Long pkg yet)
#
#  . is the only directory it scans! ADD $input{dirs} support!
#  write the html output functions
#  write a good pod

=head1 NAME
    
chkworld - Compares FrugalBuild's up2date and pkgver informations
   
=head1 SYNOPSIS
    
chkdep [-vches] [-t num] [-d developer] directory1...
    
=head1 DESCRIPTION

Scans the given directories for FrugalBuild scripts, and compares up2date with pkgver.
This way, you will always known, how fresh and crispy the packages you're maintaining.

=head1 OPTIONS

=over 4

=item B<-t num>

num is a value in seconds. After num, the checking continues with the next package, 
shutting down child processes.
Default is 30.

=item B<-e>

Enables standard error output. By default STDERR is redirected to /dev/null.

=item B<-v>

Be verbose.

=item B<-d>

Matches the string given with the Maintainer in FrugalBuild, 
filtering the output with the specified developer.

=item B<-s>

Output sorted by developers

=item B<-c>

Colorized output.

=item B<-h>

A little help.

=head1 AUTHORS
    
Written by Zsolt Szalai.

Daniel Peder, Daniel.Peder@Infoset.COM - code from IO::FILE::String
    
=head1 COPYRIGHT
    
chkworld may be copied and modified under the terms of the GNU General Public
License v2.
    
=cut

package Chkworld;

use warnings;
no warnings qw( uninitialized );
use File::Find;
use IO::File;

use Data::Dumper;

sub contents {
    my $sfh = IO::File->new( "< $_" );
    local $/ = undef;
    my $cont = $sfh->getline;
    $sfh->close;
    return $cont;
}

sub getpids{
    my @pids = @_;
    my @allpids;
    for my $pid (@pids){
        my @childs = qx/pgrep -P $pid/;
        chomp @childs;
        push @allpids, $pid, getpids(@childs);
    }
    @allpids;
}

sub timeout{ #time, command
    my ($time, $comm) = @_;
    my $output;
    my $pid;

    eval{                                                     
	local $SIG{ALRM} = sub {die "alarm\n" };
	alarm $time;            
	$pid = open(COMM, "$comm|");
	$output = scalar <COMM>;
	close COMM;                                                                 
    };                                                                              
    if ($@) {                
	do{                                 
	    my @chldpids = getpids($pid);
	    for my $chldpid (@chldpids) {
		kill TERM => $chldpid;
	    }          
	    return 'TIMEOUTED';
	} if $@ eq "alarm\n";
	#	die $@ unless $@ && $@ eq "alarm\n";                                        
    }                    
    return $output;
}


#IN:   dirs*, time*, devel* sort* output preoutput postoutput
#OUT:#
sub init_chk { #generate the iterator
    
    
    my %input = @_;
    
    $input{dirs} = $input{dirs} || ['.',];
    $input{time} = $input{time} || 30;

    $input{output} = $input{output} || sub {print "@_\n";};
    $input{preoutput} = $input{preoutput} || sub {};
    $input{postoutput} = $input{postoutput} || sub {};
    
    my @blacklist = split / /,$input{blacklist};
    my %m8rs;
    my $dev = $input{devel};
    my ($m8r,$pkgname,$pkgver,$group,$up2date,$signal);    
    my $prog;

    if ($input{sort}){
	find sub{
	    if (/^FrugalBuild\z/) {
		do {next if $File::Find::dir =~ /$blacklist[$_]/} for 0..scalar @blacklist-1;
		my $buildscript = contents $_;
		my ($m8r) = $buildscript =~ /^# Maintainer: (.*?) </m;
		$m8rs{$m8r}++;
	    }
	}, @{$input{dirs}};
    } else { if ($input{devel}) {$m8rs{$input{devel}}++;} else { $m8rs{all}++ }}
    
    return sub {
	$input{preoutput}->();
	for $dev (keys %m8rs){
	    $input{blacklist} = 'apps base';
	    find sub{
		if (/^FrugalBuild\z/){
		    do {goto OUT if $File::Find::dir =~ /$blacklist[$_]/} for 0..scalar @blacklist-1;
		    my $buildscript = contents $_;
		    ($m8r) = $buildscript =~ /^# Maintainer: (.*?) </m;
		    do {next unless $m8r =~ /\Q$dev\E/i } if $input{devel} || $input{sort};
		    ($pkgname) = qx'source ./FrugalBuild; echo -n $pkgname';
		    ($pkgver) = $buildscript =~ /^pkgver=(\S*)/m;
		    ($group) = $buildscript =~ /^groups=\('?(.*?)('|\s|\))/m;
		    $group = '?' unless $group;
		
		    $up2date = timeout $input{time}, 'CHROOT=1; source /usr/lib/frugalware/fwmakepkg;source ./FrugalBuild;if ( `echo $up2date|grep -q " "` ); then eval $up2date; else echo $up2date; fi';
		    chomp $up2date unless $up2date eq 'TIMEOUTED';
		
		    if ($up2date eq 'TIMEOUTED') {	
			$signal = -1;
		    } elsif ($pkgver ne $up2date){
			$signal = 0;
		    } else {
			$signal = 1;
		    }
		    $input{output}->($m8r,$pkgname,$pkgver,$up2date,$group,$signal);
	      OUT: next;
	        }
	     }, @{$input{dirs}};
    
        }
        $input{postoutput}->();
    }
}


package main;

use strict;
use Data::Dumper;
use Getopt::Std;    #### change to ::Long!!


$Getopt::Std::STANDARD_HELP_VERSION = 1;
our $VERSION = "0.5";

my %opts;
getopts('svcmhet:d:b:', \%opts);

sub HELP_MESSAGE(){
    print <<END
chkworld - Compares FrugalBuild's up2date and pkgver informations
usage: chkworld [-vcth] directory ...
         -t num
         -d developer
         -m htmlized output
         -s sort by developers
         -b blacklists given dirs
         -e enable standard error output
         -v be verbose
         -c colorized output
	 -h this help
       
END
}#'
    
HELP_MESSAGE && exit if $opts{h};

# better be in a module!!!!!!!!!!!!!! 
my $count = 0;
my $needupdate = 0;
my $maybebroken = 0;
my $timeouted = 0;
my $passed = 0;
my ($preout, $out, $postout);

sub std_preout {open STDERR, "/dev/null" unless $opts{e}; }
sub std_out {
    my ($m8r,$pkgname,$pkgver,$up2date,$group,$signal) = @_;

    $count++;
    my $info = "Checking for $group/$pkgname-$pkgver... ";
    print $info if $opts{v};
    if ($signal == -1) {	
        $timeouted++; #$noout = 0;
	print $info unless $opts{v};
	if ($opts{c}){
	    print "\033[1;33mTimeouted!\033[1;0m $m8r\n";
	} else {
	    print "Timeouted! $m8r\n";
	}
    } elsif ($signal == 0){
#        $noout = 0;
	print $info unless $opts{v};
	if ($opts{c}){
	    print ($up2date ? ("!= \033[1;31m" . substr($up2date, 0, 12)."\033[1;0m" )  : "\033[1;33mThere were no output!\033[1;0m");
	} else {
	    print ($up2date ? ("!= " . substr($up2date, 0, 12)) : "There were no output!");
	}
	($up2date ? ($needupdate++) : ($maybebroken++));
	print " $m8r\n";
    } else {
	$passed++;
	print "Passed $m8r\n" if $opts{v} && !$opts{c};
	print "\033[1;32mPassed\033[1;0m\n" if $opts{v} && $opts{c};
    }
}

sub std_postout {
    print "\nTotal packages checked: $count\n";
    print "Passed                : $passed\n";
    print "Need to update        : $needupdate\n";
    print "Timeouted             : $timeouted\n";
    print "Maybe broken up2date  : $maybebroken\n";
}

#if (@ARGV) { @dirs = @ARGV; } else  {@dirs = ('.',);}  # // cant handler arrays :-/

$preout = $opts{m} ? \&html_preout : \&std_preout;
$out = $opts{m} ? \&html_out : \&std_out;
$postout = $opts{m} ? \&html_postout : \&std_postout;

my $chkw = Chkworld::init_chk(
			      time => $opts{t},
			      devel => $opts{d},
			      sort => $opts{s},
			      blacklist => $opts{b},   # a string!
			      preoutput => $preout,
			      output => $out,
			      postoutput => $postout,
);

$chkw->();

