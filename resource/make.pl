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

		@list[$#list+1] = $outname;
#		print "$outname\n";
	}
}

walkdir($srcpath, "");


# create uncompressed archive file
#?? send data directly to gzip stdin ?

open (ARC, ">$arcfile") || die;
binmode (ARC);

# produce header

foreach $name (@list)
{
	open (F, "$srcpath/$name") || die "Input file ", $name, " is not found";
	seek (F, 0, 2);
	# write file length
	syswrite (ARC, pack ("l", tell (F)));
	# write filename (ASCIIZ)
	syswrite (ARC, "$name\0");
	close (F);
}

# mark end of file directory ("size = 0")
syswrite (ARC, "\0\0\0\0");

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

close (ARC);

system ("gzip --no-name --best --force $arcfile");
# VC settings:
#   build: nasm -f win32 -o $(IntDir)\resources.obj -Darc=\"$(InputPath)\" $(InputDir)\make.asm
#   out:   $(IntDir)\resources.obj
