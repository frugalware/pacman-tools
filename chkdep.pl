#!/usr/bin/perl -w

=head1 NAME

chkdep - checks dependencies of packages for Frugalware Linux

=head1 SYNOPSIS

chkdep [-vi] [-n packagename] -d dir | -p file

=head1 DESCRIPTION

Outputs a dependency array for a FrugalBuild scripts, containing the
the packages that are needed.

=head1 WARNING

This version of chkdep is _not_ compatible with version <= 1.0,
listing files in the command argument is deprecated.

=head1 OPTIONS

=over 4

=item B<-d dir>

Check all executable files in dir directry with ldd and make depend array

=item B<-p file>

If file is a tgz package, it will be extracted, making depend array

=item B<-n packagename>

When the script cant determine the name of the package you're checking,
you can give it manually with this(ie. when you use the -d option).

=item B<-v>

Be verbose

=item B<-i>

Ignore fakeroot library if found

=back

=head1 AUTHOR

Zsolt Szalai

=head1 COPYRIGHT

chkdep may be copied and modified under the terms of the GNU General Public
License v2.

=cut

use strict;
use Getopt::Std;

$Getopt::Std::STANDARD_HELP_VERSION = 1;
our $VERSION = "1.5";

my %opts;
getopts('vid:p:f:n:', \%opts);

sub HELP_MESSAGE(){
    print <<END
chkdep - checks dependencies of packages for Frugalware Linux
usage: chkdep [-vi] [-n packagename] -d dir | -p file
         -d directory
         -p samepackage.fpm
         -n packagename(use with -d if necessary)
	 -i ignore fakeroot
         -v be verbose
       
It works correctly only if all the libraries have been 
installed from fpm packages with pacman!
END
}


sub extractfpm{ #pkgfile
    my $pkg = shift;
    my @dir = split /\//, $pkg;
    my $name = "/tmp/" . pop @dir;
    die $! unless mkdir $name;
    my $comm = "tar xzf $pkg -C $name";
    $comm .= ' 2>/dev/null' unless $opts{v};
    qx/$comm/;
    return $name;
}

sub ldddir{ #dir
    my $dir = shift;
    my $comm = 'find ' . $dir .' -perm -u+x ! -type d ! -type l -exec ldd {} \;';
    $comm .= ' 2>/dev/null' unless $opts{v};
    return qx/$comm/;
}


HELP_MESSAGE && die "Wrong option!" unless %opts;

my $dir = extractfpm $opts{p} if $opts{p};
$dir = $opts{d} if $opts{d};
my $pkgname;
$pkgname = $opts{n} or 
    ($pkgname) = qx"grep pkgname $dir/.PKGINFO 2>/dev/null" =~ /pkgname = (.*)$/;
if ($opts{v}){
    if (! $pkgname){ 
	print "Could not determine packagname!\n"; $pkgname = '';
    } else {
	print "Package: $pkgname\n";
    }
}

my @ldd = ldddir $dir;

my $comm = "rm -rf $dir";
$comm .= ' 2>/dev/null' unless $opts{v};
qx/$comm/;

my %pkgs; #dependecies
my %libs;
my %depsdep; # the dependencies' dependencies 

for my $line (@ldd){
    if ($line =~ /.* => (.+) \(.*\)/){
        my $lib = $1;
	if ($lib =~ /fakeroot/){
	    print "fakeroot found in dependencies\n" if $opts{v};
	    next if $opts{i};
	}
	if (! $libs{$lib}) {
	    $libs{$lib} = 1;
	    my ($pkg) = qx/pacman -Qo $lib/ =~ /owned by (.*?)\s/;
	    print "WARNING: No package found containing $lib\n" if !$pkg && $opts{v};
	    
	    if ($pkg ne $pkgname) {
		my ($pkgdeps) = qx/pacman -Qi $pkg/ =~ /Depends.*?: (.*?)Removes/s;
		foreach my $dd (split(' ',$pkgdeps)){
		    $depsdep{$dd} = 1;
		}
	    
		# handle provides directive!
		$pkg = 'x' if ($pkg eq 'xorg');
		
		$pkgs{$pkg} = 1;
	    }
	}
    }
}
my $deps = 'depends=(';
foreach my $key (keys %pkgs){
    $deps .= "\'$key\' " unless $depsdep{$key};
}
chop $deps if %pkgs;
$deps = $deps . ')';
print "$deps\n";
