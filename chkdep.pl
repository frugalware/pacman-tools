#!/usr/bin/perl -w

=head1 NAME

chkdep - checks dependencies of packages for Frugalware Linux

=head1 SYNOPSIS

chkdep [-vif] [-n package_name] -d dir | -p file

=head1 DESCRIPTION

Outputs a dependency array for a FrugalBuild scripts, containing the
the packages that are needed.

=head1 WARNING

This version of chkdep is _not_ compatible with version <= 1.0,
listing files in the command argument is deprecated.

=head1 OPTIONS

=over 4

=item B<-d dir>

Check all executable files in dir directory with ldd and make depends array.

=item B<-p file>

If file is a tgz package, it will be extracted, making depends array.

=item B<-n package_name>

When the script can't determine the name of the package you're checking,
you can give it manually with this (ie. when you use the -d option).

=item B<-f>

Outputs the full dependency list.

=item B<-e>

Tells the groups of the full dependency list.

=item B<-v>

Be verbose.

=item B<-i>

Ignore fakeroot library if found.

=back

=head1 AUTHOR

Written by Zsolt Szalai.

=head1 BUGS

Report bugs to <frugalware-devel@frugalware.org>.

=head1 COPYRIGHT

chkdep may be copied and modified under the terms of the GNU General Public
License v2.

=cut

use strict;
no warnings qw(uninitialized);
use Getopt::Std;
use Data::Dumper;
$Getopt::Std::STANDARD_HELP_VERSION = 1;
our $VERSION = "1.9";

my %alias = ( 'xorg' => 'x');
my %reversalias = map { $alias{$_} => $_ } keys %alias;

my %opts;
getopts('vid:p:fn:e', \%opts);

sub HELP_MESSAGE(){
    print <<END
chkdep - checks dependencies of packages for Frugalware Linux
usage: chkdep [-vi] [-n package_name] -d dir | -p file
         -d directory
         -p some_package.fpm
         -n package_name (use with -d if necessary)
         -f full list
         -e add groups to output
	 -i ignore fakeroot
         -v be verbose
       
It works correctly only if all the libraries have been 
installed from fpm packages with pacman!
END
}


sub extractfpm{ #pkgfile
  my $pkg = shift;
  my @dir = split /\//, $pkg;
  my $name = "/tmp/" . (pop @dir) . '.' . $$;
  die $! unless mkdir $name;
  my $comm = "tar x".(qx "/usr/bin/file $pkg" =~ /bzip2/ ? 'j' : 'z')."f $pkg -C $name" . (($opts{v}) ? '' : ' 2>/dev/null');
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
    print "Could not determine packag name!\n"; $pkgname = '';
  } else {
    print "Package: $pkgname\n";
  }
} elsif (! $pkgname){$pkgname='';}

my @ldd = ldddir $dir;

if ($opts{p}){
  my $comm ="rm -rf $dir" . (($opts{v}) ? '' : ' 2>/dev/null');
  qx/$comm/;
}

my %pkgs; #dependecies
my %libs;
my %depsdep; # the dependencies' dependencies 

for my $line (@ldd){
  if ($line =~ /(.*) => (.+) (?:\(.*\))?/){
    my ($linked, $lib) = ($1,$2);
    if ($lib =~ /fakeroot/){
      print "fakeroot found in dependencies\n" if $opts{v};
      next if $opts{i};
    }
    if ($lib =~ /not/) {
      print "WARNING: $linked not found by ldd\n";
    } else {
      if (! $libs{$lib}) {
	$libs{$lib} = 1;
	my ($pkg) = qx/LANG= LC_ALL= pacman -Qo $lib/ =~ /owned by (.*?)\s/;
	print "WARNING: No package found containing $lib\n" if !$pkg && $opts{v};
	
	if ($pkg ne $pkgname) {
	  unless ($opts{f}){
	    my ($pkgdeps) = qx/LANG= LC_ALL= pacman -Qi $pkg/ =~ /Depends.*?: (.*?)Removes/s;
	    foreach my $dd (split(' ',$pkgdeps)){
	      $depsdep{$dd} = 1;
	    }
	  }
	  # handle provides directive!
	  $pkg = $alias{$pkg} if $alias{$pkg};
	  
	  $pkgs{$pkg} = 1;
	}
      }
    }
  }
}
my $deps = 'depends=(';
foreach my $key (keys %pkgs){
  $deps .= "\'$key\' " unless $depsdep{$key};
  if ($opts{e}){
    my $comm = "LANG= LC_ALL= pacman -Qi " . ($reversalias{$key} || $key);
    my ($group) = qx/$comm/ =~ /Groups.*?: (.*?)$/sm;
    printf "%-20s%s\n", "$key is in", $group;
  }
}
chop $deps if %pkgs;
$deps = $deps . ')';
print "$deps\n";
