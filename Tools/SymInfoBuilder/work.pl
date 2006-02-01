#!/usr/bin/perl -w

$workext  = "map";								# can use "(ext1|ext2|ext3)" syntax
$cvt_tool = "dumpmap";
$dbg_file = "symbols.dbg";

#$sizesWarn = 128;			# maximum size of variable (Kb), which allowed without warning
#$doDump = 1;

sub ProcessType {
	my $arg = $_[0];
	$arg =~ s/\b\w+\:\://;						# remove namespace qualifier from argument type
	$arg =~ s/\b(struct|class|union)\s+//;		# remove struct/class qualifier from type name
	$arg =~ s/\b(virtual|static)\s+//;			# remove storage type qualifier from type name
	$arg =~ s/\b const \s* \* \s* const /const */x;	# replace "const * const" -> "const *"
	# convert some type names
	$arg =~ s/\b unsigned \s+ char \b/uchar/x;
	$arg =~ s/\b unsigned \s+ int \b/uint/x;
	$arg =~ s/\b unsigned \s+ short \b/ushort/x;
	$arg =~ s/\b_iobuf\b/FILE/x;				# Visual C++ FILE structure
	$arg =~ s/\s+\*$/*/x;						# replace "name *" -> "name*"
	return $arg;
}

sub ProcessMap {
	my ($filename) = @_;
	my ($addr, $name);

	($name) = $filename =~ /.*\/(\w+)\.\w+/;
	# write module name
	syswrite (DBG, "$name\0");

	open (IN, "$cvt_tool $filename | sort |") or die "cannot pipe map file $filename";
	print STDERR "---------- $name ------------\n" if $doDump;
	my $prev = 0; #0x7FFFFFFF;
	my $prevName = "";
	while (<IN>)
	{
		$_ =~ s/\*\s+(\S)/\*$1/g;					# remove spaces on a right side of '*'
		($_) = $_ =~ /^\s*(\S.*\S)\s*$/;
		next if !defined($_);
#		print "LINE: $_\n";	#!!!
		($addr, $name, undef, $args) = / \s* (\S+) \s+ ([\w\.<>:*,~\s]+) \s* (\( (.*) \))? /x;
		my $curr = hex($addr);
		next if $name =~ /^__/;						# do not include in list names, started with double underscore
		# process return type
		my ($t, $n);
		($t, $n) = $name =~ /^ (\S.*?\S) \s* (\b (\w+\:\:)? operator\b \s* \S+ $)/x;
		($t, undef, $n) = $name =~ /^ (\S+.*(\*|\s+)) ([\w\:]+) $/x if !defined($n);
		if (defined $n && defined $t) {
			$t =~ s/\s*$//;
			$name = ProcessType($t)." ".$n;
		}
#		print "  [$t/$n] -> [$name] [$args]\n";	#!!!
		# cut "virtual"
		if ($name =~ /^virtual\s/) {
			($name) = $name =~ /^virtual\s+(\S.*)/;
		}
		# cut "void" at start
		if ($name =~ /^void\s/) {
			($name) = $name =~ /^void\s+(\S.*)/;
		}
		# detect constructors/destructors
		($t, $n) = $name =~ /^ (.*) \:\: (.*) $/x;
		if (defined $t && defined $n)
		{
			if ($n eq $t) {
				$name = "$t\:\:construc";
			} elsif ($n eq "~".$t) {
				$name = "$t\:\:destruc";
			}
		}

		if (defined($args)) {
			$args = "" if $args eq "void";			# replace func(void) -> func()
			# some processing on arguments list
			my @args = split(',', $args);
			$name .= "(";
			my $i = 0;
			for my $arg (@args) {
				$name .= "," if $i;
				$arg = ProcessType ($arg);
#				$arg =~ s/\b\w+\:\://;				# remove namespace qualifier from argument type
#				$arg =~ s/\b(struct|class)\s+//;	# remove struct/class qualifier from type name
				($arg) = $arg =~ /\s*(\S.*\S)\s*/;	# remove leading/trailing spaces (for GCC map)
				$name .= $arg;
				$i++;
			}
			$name .= ")";
		}
#		print "  FUNC=$name\n";	#!!!
		printf(STDERR ".%06X %5X %s\n", $curr, $curr-$prev,$prevName) if $doDump;
		# write symbol info
		syswrite (DBG, pack ("l", $curr));			# symbol RVA
		syswrite (DBG, "$name\0");					# symbol name
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

# usage information
die "Generate symbols.dbg\nUsage: work.pl workdir compiler\n" if $#ARGV != 1;

# cvt_tool placed in a directory of this script
($tmp) = $0 =~ /(.*\/)[^\/]+/ ;						# extract path
$tmp = "./" if !defined($tmp);
$cvt_tool = $tmp.$cvt_tool."-".$ARGV[1];			# append filename and compiler

$workdir = $ARGV[0];
# read output directory
opendir (DIR, $workdir) or die "cannot open dir \"$workdir\"\n";
@filelist = readdir (DIR);
closedir (DIR);

# open debug file for writting
open (DBG, ">$workdir/$dbg_file") or die "cannot create $dbg_file";
binmode (DBG);
# process map files
$found = 0;
for $file (@filelist) {
	if ($file =~ /^ .* \. $workext $/x) {
#		print ("$filelist[$i]\n");
		ProcessMap ("$workdir/$file");
		$found = 1;
	}
}
die "No *.map files found\n" if !$found;
# write end-of-file mark (module with null name)
syswrite (DBG, "\0");
close (DBG);
