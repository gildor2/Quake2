#!/usr/bin/perl

#--------------------- Defines -------------------------------

if (0) {
	#...... work .......
	die "Not implemented !!";
	$infile = "keywords.in";
	$outcode = "../qgl_impl.h";		#!! change !
} else {
	#...... test .......
	$infile = "keywords.in.test";
	$outcode = "impl.h";
}

$arrayname = "tokens";
$enumname = "shaderToken_t";
$constname = "NUM_TOKENS";


#------------------- Parsing input file ----------------------


sub EmitString {
	print (CODE ",") if $num > 0;
	printf (CODE "\n\t\"%s\"", $str);
}

sub EmitEnum {
	print (CODE ",") if $num > 0;
	printf (CODE "\n\tTOK_%s", uc($str));
}


#--------------------- Functions -----------------------------


sub getline {
	while ($line = <IN>)
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


# Usage: Parse (<per-header func> <per-header-end>)
sub Parse {
	my $sub;
	my $subBegin = @_[0];

	open (IN, $infile) || die "Input file ", $infile, " not found";
	$num = 0;
	while (getline())
	{
		($str) = $line =~ / \s* (\S.*\S) \s* /x;	# truncate leading and trailing spaces
			# restriction: string MUST have at least 2 non-space chars

		&$subBegin () if defined $subBegin;
		$num++;
	}
	close (IN);
}


#----------------------- Main program --------------------------------

# open file
open (CODE, ">$outcode") || die "Cannot create file ", $outcode;

Parse ();
$numTokens = $num;
printf ("Found %d tokens ...\n", $numTokens);

printf (CODE "#define %s %d\n\n", $constname, $numTokens);

print (CODE "typedef enum\n{");
Parse ("EmitEnum");
printf (CODE "\n} %s;\n\n", $enumname);

printf (CODE "static char *%s[%s] = {", $arrayname, $constname);
Parse ("EmitString");
print (CODE "\n};\n\n");

# close file
close (CODE);
