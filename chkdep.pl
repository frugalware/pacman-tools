#!/usr/bin/perl -w

=head1 NAME

chkdep - checks dependencies for packages of Frugalware Linux

=head1 SYNOPSIS

chkdep [-d dir] [-p file]

=head DESCRIPTION

Outputs a dependency array for a FrugalBuild scripts, containing the
the packages that are needed.

=head1 WARNING

This version of chkdep is _not_ compatible with version <= 1.0,
listing files in the command argument is deprecated.

=head1 OPTIONS

=over 4

=item B<-d dir>

check all executable files in dir directry with ldd and make depend array

=item B<-p file>

if file is a tgz package, it will be extracted, making depend array

=item B<-n packagename>

when the script cant determine the name of the package you're checking,
you can give it manually with this(ie. when you use the -d option).

=back

=head1 COPYRIGHT

chkdep may be copied and modified under the terms of the GNU General Public
License.

=cut

use strict;
use Getopt::Std;

$Getopt::Std::STANDARD_HELP_VERSION = 1;
our $VERSION = "1.3";

sub HELP_MESSAGE(){
    print <<END
chkdep - checks dependencies for packages of Frugalware Linux
usage: chkdep -d directory
       chkdep -p samepackage.fpm
       chkdep -n packagename(use with -d if necessary)

It works correctly only if all the libraries have been 
installed from fpm packages with pacman!
END
}


sub extractfpm{ #pkgfile
    my $pkg = shift;
    my @dir = split /\//, $pkg;
    my $name = "/tmp/" . pop @dir;
    die $! unless mkdir $name;
    `tar xzf $pkg -C $name`;
    #my ($pkgname) = $name =~ /.*\/(.*)-.*-.*.fpm$/;
    return $name;
}

sub ldddir{ #dir
    my $dir = shift;
    my $comm = 'find ' . $dir .' -perm -u+x ! -type d ! -type l -print -exec ldd {} \;';
    return `$comm`;
}


my %opts;
getopts('d:p:f:n:', \%opts);

HELP_MESSAGE && die "Wrong option!" unless %opts;

my $dir = extractfpm $opts{p} if $opts{p};
$dir = $opts{d} if $opts{d};
my $pkgname;
$pkgname = $opts{n} or 
    ($pkgname) = `grep pkgname $dir/.PKGINFO 2>/dev/null` =~ /pkgname = (.*)$/;
if (not defined $pkgname){ 
    print "Could not determine packagname!\n"; $pkgname = '';
} else {
    print "Package: $pkgname\n";
}

my @ldd = ldddir $dir;

`rm -rf $dir` if $opts{p};

my %pkgs = (); #dependecies
my %libs = ();
my %depsdep =(); # the dependencies' dependencies 

for my $line (@ldd){
    if ($line =~ /.* => (.+) \(.*\)/){
        my $lib = $1;
	if (!defined $libs{$lib}) {
	    $libs{$lib} = '1';
	    `pacman -Qo $lib` =~ /owned by (.*?)\s(.*)\Z/;
	    my $pkg = $1;
	    if ($pkg eq '' or !defined $pkg) {
		print "WARNING: No package found containing $lib\n"; 
	    }
	    
	    if ($pkg ne $pkgname) {
		my ($pkgdeps) = `pacman -Qi $pkg` =~ /Depends.*?: (.*?)Removes/s;
		foreach my $dd (split ' ', $pkgdeps){
		    $depsdep{$dd} = '1';
		}
	    
		# handle provides directive!
		$pkg = 'x' if ($pkg eq 'xorg');
		
		$pkgs{$pkg} = '1' if ($pkg ne $pkgname);
	    }
	}
    }
}
my $deps = 'depends=(';
foreach my $key (keys %pkgs){
    $deps .= "\'$key\' " if (not defined $depsdep{$key});
}
chop $deps if %pkgs;
$deps = $deps . ')';
print "$deps\n";
