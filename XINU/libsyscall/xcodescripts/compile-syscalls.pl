

# Copyright (c) 2010 Apple Inc. All rights reserved.




# as defined in and that are subject to the Apple Public Source License





















use warnings;
use strict;

use Data::Dumper;
use File::Spec;
use IO::File;
use File::Basename ();

my $basename = File::Basename::basename($0);

sub usage {
	print "$basename: <source list> <output archive>";
	exit 1;
}

usage unless scalar(@ARGV) == 2;

my $sourceList = $ARGV[0];
my $outputFile = $ARGV[1];

my $f = IO::File->new($sourceList, 'r');
die "$basename: $sourceList: $!\n" unless defined($f);

my @objects;
my @archs = split / /, $ENV{"ARCHS"};
my @sources = <$f>;
chomp @sources;

undef $f;


chomp(my $CC = `xcrun -sdk "$ENV{'SDKROOT'}" -find cc`);
my @CFLAGS = (
	"-x assembler-with-cpp",
	"-c",
	"-isysroot", $ENV{'SDKROOT'} || "/",
	"-I".$ENV{"SDKROOT"}."/".$ENV{"SDK_INSTALL_HEADERS_ROOT"}."/usr/include",
	"-I".$ENV{"SDKROOT"}."/".$ENV{"SDK_INSTALL_HEADERS_ROOT"}."/usr/local/include",
	"-I".$ENV{"SDKROOT"}."/".$ENV{"SDK_INSTALL_HEADERS_ROOT"}."/System/Library/Frameworks/System.framework/PrivateHeaders",
);

chomp(my $LIBTOOL = `xcrun -sdk "$ENV{'SDKROOT'}" -find libtool`);
my @LIBTOOLFLAGS = (
	"-static",
);


for my $arch (@archs) {
	push(@CFLAGS, "-arch $arch");
}


my $jobs = `sysctl -n hw.ncpu` + 2;

for my $src (@sources) {
	if ($jobs == 0) {
		if (wait != -1) {
			$jobs++;
		} else {
			printf "wait exited with -1 (no children) and exhausted allowed jobs. Exiting.\n";
			exit 1;
		}
		
		if ($? != 0) {
			printf "$CC exited with value %d\n", $? >> 8;
			exit 1;
		}
	}
	
	(my $o = $src) =~ s/\.s$/\.o/;
	my $compileCommand = "$CC " . join(' ', @CFLAGS) . " -o $o $src";
	printf $compileCommand . "\n";

	$jobs--;
	my $pid = fork();
	if ($pid == 0) {
		exec($compileCommand);
	}
	push(@objects, $o);
}

while (wait != -1) {
	if ($? != 0) {
		printf "$CC exited with value %d\n", $? >> 8;
		exit 1;
	}
}

printf "Finished assembly, beginning link.\n";



if (-f $outputFile) {
	unlink($outputFile);
}

my $linkCommand = "$LIBTOOL " . join(' ', @LIBTOOLFLAGS) . " -o $outputFile " . join(' ', @objects);

printf $linkCommand . "\n";
system($linkCommand);
if ($? != 0) {
	print "$LIBTOOL exited with value %d\n", $? >> 8;
	exit 1;
}
