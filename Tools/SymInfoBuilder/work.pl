#!/usr/bin/perl -w

$workdir  = "../../Release";
$workext  = "map";								# can use "(ext1|ext2|ext3)" syntax
$cvt_tool = "../DumpMap/dumpmap";
$dbg_file = "symbols.dbg";

#$sizesWarn = 128;			# maximum size of variable (Kb), which allowed without warning
#$doDump = 1;

sub ProcessMap {
	my	($filename) = @_;
	my	($addr, $name);

	($name) = $filename =~ /.*\/(\w+)\.\w+/;
	# write module name
	syswrite (DBG, "$name\0");

	open (IN, "$cvt_tool $filename | sort |") or die "cannot pipe map file $filename";
	print STDERR "---------- $name ------------\n" if $doDump;
	my $prev = 0; #0x7FFFFFFF;
	my $prevName = "";
	while (<IN>)
	{
		($addr, $name) = / \s* (\S+) \s+ (\S+ .* \S+) /x;
		$name =~ s/\*\s+(\S)/\*$1/g;			# remove spaces on a right side of '*'
		my $curr = hex($addr);
		next if $name =~ /^__/;					# do not include in list names, started with double underscore
		printf(STDERR ".%06X %5X %s\n", $curr, $curr-$prev,$prevName) if $doDump;
		# write symbol info
		syswrite (DBG, pack ("l", $curr));		# symbol RVA
		syswrite (DBG, "$name\0");				# symbol name
		if (defined($sizesWarn) && $curr - $prev > $sizesWarn*1024) {
			printf (STDERR "%4.1fK  %s  ---  %s\n", ($curr-$prev)/1024, $prevName, $name);
		}
		$prev = $curr;
		$prevName = $name;
	}
	close (IN);
	# write end-of-module mark (symbol RVA == 0)
	syswrite (DBG, "\0\0\0\0");
}


# read output directory
opendir (DIR, $workdir) or die "cannot open dir";
@filelist = readdir (DIR);
closedir (DIR);

# open debug file for writting
open (DBG, ">$workdir/$dbg_file") or die "cannot create $dbg_file";
binmode (DBG);
# process map files
for $file (@filelist) {
	if ($file =~ /^ .* \. $workext $/x) {
#		print ("$filelist[$i]\n");
		ProcessMap ("$workdir/$file");
	}
}
# write end-of-file mark (module with null name)
syswrite (DBG, "\0");
close (DBG);
