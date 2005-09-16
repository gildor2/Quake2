#/usr/bin/perl

$srcpath  = "res";

# temporary files
$arcfile  = "archive";


# generate list of files

@list = ();

sub walkdir {
	my ($name, $prefix) = (@_);
	my ($f, @filelist, $outname);

	opendir(DIR, $name);
	@filelist = readdir(DIR);
	closedir(DIR);

	for $f (@filelist) {
		if (-d "$name/$f") {
			next if $f eq ".";
			next if $f eq "..";
			walkdir("$name/$f", "$prefix$f/");
			next;
		}
		$outname = "$prefix$f";

		push @list, $outname;
#		print "$outname\n";
	}
}

walkdir($srcpath, "");


# create uncompressed archive file
#?? send data directly to gzip stdin ?

open (ARC, ">$arcfile") || die;
binmode (ARC);

# reserve place for data pointer
syswrite (ARC, pack ("l", 0));

# produce header

foreach $name (@list)
{
	open (F, "$srcpath/$name") || die "Input file ", $name, " is not found";
	sysseek (F, 0, 2);
	# write file length
	syswrite (ARC, pack ("l", tell (F)));
	# write filename (ASCIIZ)
	syswrite (ARC, "$name\0");
	close (F);
}
$hdrLen = sysseek (ARC, 0, 1);		# implementation of "systell(ARC)"

# append data
foreach $name (@list)
{
	open (F, "$srcpath/$name") || die;
	binmode (F);
	while (sysread (F, $data, 4096))
	{
		syswrite (ARC, $data);
	}
	close (F);
}
close (LST);

# complete header
sysseek (ARC, 0, 0);				# rewind to file start
syswrite (ARC, pack ("l", $hdrLen));

close (ARC);

system ("gzip --no-name --best --force $arcfile");

# cut gzip header
open (ARC, "$arcfile.gz") || die;
$len = sysseek (ARC, 0, 2);
sysseek (ARC, 10, 0);				# skip header (10 bytes)
sysread (ARC, $data, $len-18);		# read whole file into $data (skip 10 bytes at start and 8 at end)
close (ARC);

open (ARC, ">$arcfile.res") || die;
# write "length" field
syswrite (ARC, pack ("l", $len-18));
# write data
syswrite (ARC, $data);
close (ARC);

# remove GZip archive
#unlink ("$arcfile.gz");


# VC settings:
#   build: nasm -f win32 -o $(IntDir)\resources.obj -Darc=\"$(InputPath)\" $(InputDir)\make.asm
#   out:   $(IntDir)\resources.obj
