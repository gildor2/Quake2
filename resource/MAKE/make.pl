#/usr/bin/perl

$listfile = "files.lst";

# temporary giles
$arcfile  = "archive";


sub getline {
	while ($line = <LST>)
	{
		# remove CR/LF
		$line =~ s/\r//;
		$line =~ s/\n//;
		# remove comments
		$line =~ s/\s*\/\/.*//;
		# ignore empty lines
		next if $line eq "";
		return 1;
	}
	return 0;
}


open (ARC, ">$arcfile") || die;
binmode (ARC);

# produce header
open (LST, $listfile) || die "List file ", $listfile, " is not found";

chdir ("..");	#!!

while (getline())
{
	open (F, $line) || die "Input file ", $line, " is not found";
	seek (F, 0, 2);
	# write file length
	syswrite (ARC, pack ("l", tell (F)));
	# write filename (ASCIIZ)
	syswrite (ARC, $line);
	syswrite (ARC, "\0");
	close (F);
}

# mark end of file directory ("size = 0")
syswrite (ARC, "\0\0\0\0");

# append data
seek (LST, 0, 0);
while (getline())
{
	open (F, $line) || die;
	binmode (F);
	while (sysread (F, $data, 4096))
	{
		syswrite (ARC, $data);
	}
	close (F);
}
close (LST);

close (ARC);

system ("gzip --no-name --best --force make/$arcfile");
# VC settings:
#   build: nasmw -f win32 -o $(IntDir)\resources.obj -Darc=\"$(InputPath)\" $(InputDir)\make.asm
#   out:   $(IntDir)\resources.obj
