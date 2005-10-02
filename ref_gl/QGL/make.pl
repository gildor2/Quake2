#!/usr/bin/perl

#------------------------------------------------------------------------------
#	Defines
#------------------------------------------------------------------------------

sub TEST {0}	# 0|1 - work|test

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if (not TEST) {
	#......... work .........
	$infile  = "funcs.in";
	$constfile = "consts.in";
	$outhdr  = "../qgl_decl.h";
	$outcode = "../qgl_impl.h";
} else {
	#......... test .........
	print "*** TEST RUN ***\n";
	$infile  = "funcs.in.test";
	$constfile = "consts.in";
	$outhdr  = "decl.h";
	$outcode = "impl.h";
}

$strucname   = "qgl";
$constname   = "NUM_GLFUNCS";
$constname2  = "NUM_EXTFUNCS";
$constname3  = "NUM_EXTENSIONS";
$extArrName  = "extInfo";
$log         = "LogFile->Printf (";


#------------------------------------------------------------------------------
#	Platform-dependent stuff
#------------------------------------------------------------------------------

sub PlatformArray { (0, 0, 0) }

@platformNames = ("base", "win32", "linux");

sub PlatformIndex {
	my $i;
	my $name = $_[0];

	$i = 0;
	while ($i <= $#platformNames)
	{
		return $i if $platformNames[$i] eq $name;
		$i++;
	}
	die "Unknown platform name: ", $name;
}

sub PlatformDefine {
	my $idx = $_[0];

	if ($idx == 1) {
		return "_WIN32";
	} elsif ($idx == 2) {
		return "__linux__";
	} else {
		die "Unknown platform index: ", $idx;
	}
}


#------------------------------------------------------------------------------
#	Functions
#------------------------------------------------------------------------------


sub getline {
	while ($line = <IN>)
	{
		# remove CR/LF
		$line =~ s/\r//;
		$line =~ s/\n//;
		# remove "//..." comments
		$line =~ s/\s*\/\/.*//;
		# ignore empty lines
		next if $line eq "";
		return 1;
	}
	return 0;
}


sub const {
	my ($args) = @_;
	my ($str);

	return if TEST;		# do not generate this constants for test version

	@consts = split (' ', $args);
	for $str (@consts)
	{
		print (CODE <<EOF
	case GL_$str:
		return "$str";
EOF
		);
	}
}


#------------------------------------------------------------------------------
#	Parsing input file
#------------------------------------------------------------------------------


sub EmitPlatformHDR {
	print (HDR "#endif\n") if $oldPlatform != 0;
	printf (HDR "\n#ifdef %s\n", PlatformDefine ($platform)) if $platform != 0;
}

sub EmitPlatformHDR_EXT {
	if ($isExt)
	{
		print (HDR "#endif\n") if $oldPlatform != 0;
		printf (HDR "\n#ifdef %s\n", PlatformDefine ($platform)) if $platform != 0;
	}
}

sub EmitPlatformCODE {
	print (CODE "\n#endif\n\n") if $oldPlatform != 0;
	printf (CODE "\n#ifdef %s\n\n", PlatformDefine ($platform)) if $platform != 0;
}

sub EmitPlatformCODE2 {
	print (CODE "\n#endif\n") if $oldPlatform != 0;
	printf (CODE "\n#ifdef %s\n", PlatformDefine ($platform)) if $platform != 0;
}

sub EmitPlatformCODE_EXT {
	if ($isExt)
	{
		print (CODE "\n#endif\n") if $oldPlatform != 0;
		printf (CODE "\n#ifdef %s\n", PlatformDefine ($platform)) if $platform != 0;
	}
}

sub EmitStruc {
	print (HDR <<EOF
		$type	(APIENTRY * $func) ($args);
EOF
	);
}

sub EmitDefine {
	print (HDR <<EOF
#define $funcname	$strucname.$func
EOF
	);
}

sub EmitString {
	print (CODE ",") if $num > 0;
	printf (CODE "\n\t\"%s\"", $funcname);
}

sub EmitLogInit {
	print (CODE ",") if $num > 0;
	printf (CODE "\n\tlog%s", $func);
}

sub EmitLogDecl {
	my ($s, $i);
	my (@atype, @parm, @arr);

	print (CODE <<EOF
static $type APIENTRY log$func ($args)
{
EOF
	);
	if ($end ne "")
	{
		# separating arguments
		@args = split (',', $args);

		# extracting type info for each argument
		@atype = @ptr = @parm = @arr = ();
		$i = 0;
		for $arg (@args)
		{
			($atype[$i], $parm[$i], $arr[$i]) = $arg =~ / \s* (\S+ .* [\*\s]) (\w+) (\[.*\])? /x;
			($atype[$i]) = $atype[$i] =~ /\s*(\S.*\S)\s*/;	# truncate leading and trailing spaces
			$i++;
		}

		# generate logging
		if ($end =~ /ARGS/)
		{
			print (CODE "\t$log\"%s (");
			$i = 0;
			for $s (@atype)
			{
				my $fmt;

				#............... log with parsing params ........................
				die ("Array used for ", $s, " in func ", $func) if $arr[$i] ne "";
				if ($s =~ /.*\*/) {
					$fmt = "\$%X";
				} elsif ($s =~ /\w*(int|sizei|byte|boolean)/) {
					$fmt = "%d";
				} elsif ($s =~ /\w*(float|double|clampd|clampf)/) {
					$fmt = "%g";
				} elsif ($s =~ /\w*(enum)/) {
					$fmt = "GL_%s";
				} else {
					die "Unknown argument type \"", $s, "\"";
				}
				print (CODE ", ") if $i > 0;
				print (CODE $fmt);
				$i++;
			}
			print (CODE ")\\n\", \"$funcname\"");
			$i = 0;
			for $s (@parm)
			{
				if ($atype[$i] =~ /\w*(enum)/) {
					print (CODE ", EnumName($s)");
				} else {
	                print (CODE ", $s");
				}
                $i++;
			}
			print (CODE ");\n");
		} elsif ($end =~ /;/) {
			#................... log function name only ..........................
			print (CODE "\t$log\"%s\\n\", \"", $funcname, "\");\n");	# make fprintf(..., "%s", "glFunction\n");
		} elsif ($end !~ /NAME/) {
			die "Invalid token: \"", $end, "\"";
		}

		# generate call to a library function
		if ($type !~ /\w*void$/) {		# may be GLvoid
			print (CODE "\treturn ");
		} else {
			print (CODE "\t");
		}
		print (CODE "lib.$func (");
		# make arguments for call
		$i = 0;
		for $s (@parm)
		{
			print (CODE ", ") if $i > 0;
			print (CODE $s);
			$i++;
		}
		print (CODE ");\n");
	}
	print (CODE "}\n\n");
}

sub EmitExtensionHDR {
	my $num;

	$num = $numExtensions[$platform] - 1;		# position = count - 1
	$num += $numExtensions[0] if $platform;
	printf (HDR "#define Q%s\t(1 << %d)\n", uc($extName), $num);
}

sub EmitExtensionCODE {
	my ($numFuncs, $alias);

	print (CODE ",\n") if $numExtensions[0] > 1 || $platform > 0;
	# name
	printf (CODE "\t{\"%s\\0\"", $extName);
	foreach $alias (@aliases) {
		printf (CODE " \"%s\\0\"", $alias);
	}
	print (CODE ", NULL, ");
	# cvar
	if ($extCvar ne "") {
		printf (CODE "\"%s\", ", $extCvar);
	} else {
		print (CODE "NULL, ");
	}
	# funcs
	$numFuncs = $numExt[$platform] - $firstExtFunc;
	if ($numFuncs) {
		$firstExtFunc += $numExt[0] if $platform;
		printf (CODE "%s+%d, %d, ", $constname, $firstExtFunc, $numFuncs);
	} else {
		print (CODE "0, 0, ");
	}
	# require/deprecate
	print (CODE $require, ", ", $prefer);
	print (CODE "}");
}

sub CloseExtension {
	&$subExtension () if $extName ne "" && defined $subExtension;
	$extName = "";
}


# Close opened platform-depended code
sub ClosePlatform {
	CloseExtension ();
	$firstExtFunc = 0;
	if ($platform != 0 && defined $subPlatform)
	{
		$oldPlatform = $platform;
		$platform = 0;		# base
		&$subPlatform ();
	}
}

# Change work platform
sub OpenPlatform {
	ClosePlatform ();
	$oldPlatform = $platform;
	$platform = PlatformIndex ($_[0]);
	&$subPlatform () if defined $subPlatform;
}

sub ExtNameRequired {
	die "\"#", $_[0], "\" without \"#name\"" if $extName eq "";
}

sub ExtExpression {
	my ($str, $i);

	$str = "";
	for $i (split (' ', $_[0]))
	{
		$i = "Q".uc($i);
		$str = $str." | " if $str ne "";
		$str = $str.$i;
	}
	return $str;
}

# Usage: Parse (<per-header func> <per-platform func>)
sub Parse {
	($subBegin, $subPlatform, $subExtension) = @_;

	open (IN, $infile) || die "Input file ", $infile, " not found";
	@numBase = PlatformArray;
	@numExt = PlatformArray;
	@numExtensions = PlatformArray;
	$num = 0;
	$platform = 0;
	$oldPlatform = 0;
	$isExt = 0;

	while (getline())
	{
		my ($cmd, undef, $cmda) = $line =~ /^\# \s* (\w+) (\s+ (\w+ (\s+ \w+)*)\s?)? $/x;

		#.............. our "preprocessor" ...............
		if ($cmd eq "platform") {
			OpenPlatform ($cmda);
		} elsif ($cmd eq "extensions") {
			ClosePlatform ();
			$isExt = 1;
			$platform = 0;
		} elsif ($cmd eq "name") {
			die "\"#name\" without \"#extensions\"" if $isExt == 0;
			CloseExtension ();
			$numExtensions[$platform]++;
			$extName = $cmda;
			@aliases = ();
			$extCvar = "";			# undefine cvar
			$firstExtFunc = $numExt[$platform];
			$prefer = "0";
			$require = "0";
		} elsif ($cmd eq "alias") {
			ExtNameRequired ($cmd);
			# alternative ways to add $cmda to array @aliases:
#			@aliases = (@aliases, $cmda);	#1
#			$aliases[++$#aliases] = $cmda;	#2
			push @aliases, $cmda;			#3
		} elsif ($cmd eq "cvar") {
			ExtNameRequired ($cmd);
			$extCvar = $cmda;
		} elsif ($cmd eq "prefer") {
			ExtNameRequired ($cmd);
			$prefer = ExtExpression ($cmda);
		} elsif ($cmd eq "require") {
			ExtNameRequired ($cmd);
			$require = ExtExpression ($cmda);
		} elsif ($cmd ne "") {
			die "Unknown command: ", $cmd
		} else {
			#.......... function declaration .............
			($type, $func, $args, $end) = $line =~ / \s* (\S+ .* [\*\s]) (\w+) \s+ \( (.*) \) \s* (\S*.*) /x;
			($type) = $type =~ /\s*(\S.*\S)\s*/;	# truncate leading and trailing spaces
			if ($end =~ /NAME/) {
				($funcname) = $end =~ / .* NAME \s+ (\w+)/x;
			}
			else {
				if ($platform == 0) {
					$funcname = "gl".$func if $platform == 0;
				} else {
					$funcname = $func;
					($func) = $func =~ / [a-z]* ([A-Z]\w+) /x;
				}
			}

			&$subBegin () if defined $subBegin;
			if ($isExt) {
				$numExt[$platform]++;
			} else {
				$numBase[$platform]++;
			}
			$num++;
		}
	}
	ClosePlatform ();

	close (IN);
}


#------------------------------------------------------------------------------
#	Main program
#------------------------------------------------------------------------------

# open files
open (HDR, ">$outhdr") || die "Cannot create file ", $outhdr;
print (HDR "// Autogenerated file: do not modify\n\n");
open (CODE, ">$outcode") || die "Cannot create file ", $outcode;
print (CODE "// Autogenerated file: do not modify\n\n");

Parse ();
$i = 0;
for $ct (@numBase)
{
	printf ("Found %d + %d functions (%d extensions) for %s\n", $ct, $numExt[$i], $numExtensions[$i], $platformNames[$i]);
	$i++;
}


#------------------------------------------------------------------------------
#	Creating header part
#------------------------------------------------------------------------------

print (HDR <<EOF
typedef void (APIENTRY * dummyFunc_t) (void);

typedef union
{
	struct {
EOF
);
Parse ("EmitStruc", "EmitPlatformHDR");
print (HDR <<EOF
	};
	dummyFunc_t funcs[1];
} ${strucname}_t;

extern ${strucname}_t $strucname;


EOF
);

Parse ("EmitDefine", "EmitPlatformHDR");


#------------------------------------------------------------------------------
#	Creating code part
#------------------------------------------------------------------------------

$i = 1;
while ($i <= $#numBase)
{
	if ($i == 1) {
		print (CODE "#if defined(", PlatformDefine ($i), ")\n");
	} else {
		print (CODE "#elif defined(", PlatformDefine ($i), ")\n");
	}
	printf (CODE <<EOF
#	define $constname	%d
#	define $constname2	%d
#	define $constname3	%d
EOF
	, $numBase[0] + $numBase[$i], $numExt[0] + $numExt[$i], $numExtensions[0] + $numExtensions[$i]);
	$i++;
}

print (CODE <<EOF
#else
#	define $constname	$numBase[0]
#	define $constname2	$numExt[0]
#	define $constname3	$numExtensions[0]
#endif


EOF
);

printf (CODE "static const char *%sNames[%s + %s] = {", $strucname, $constname, $constname2);
Parse ("EmitString", "EmitPlatformCODE2");
print (CODE "\n};\n\n");


# begin of logging part
print (CODE "#if !NO_GL_LOG\n\n\n");

#------------------------------------------------------------------------------
#	Create EnumName ()
#------------------------------------------------------------------------------

print (CODE <<EOF
static const char *EnumName (GLenum v)
{
	switch (v)
	{
EOF
);

open (IN, $constfile) || die "Input file ", $infile, " not found";
while (getline ())
{
	const ($line);
}
close (IN);

print (CODE <<EOF
	default:
		return va("UNK_%X", v);
	}
}


EOF
);

#------------------------------------------------------------------------------
#	Create log functions
#------------------------------------------------------------------------------

Parse ("EmitLogDecl", "EmitPlatformCODE");

printf (CODE "static const %s_t logFuncs = {", $strucname);
Parse ("EmitLogInit", "EmitPlatformCODE2");
print (CODE "\n};\n\n");

# end of logging part
print (CODE "#endif // NO_GL_LOG\n\n");

#------------------------------------------------------------------------------
#	Extensions suport
#------------------------------------------------------------------------------

print (CODE <<EOF
typedef struct {
	const char *names;				// "alias1\\0alias2\\0...\\0" or "name\\0\\0"
	const char *name;				// current name (points to a primary name or to any alias)
	const char *cvar;				// name of cvar to disable extension
	short	first, count;			// positions of provided functions in name table
	unsigned require, deprecate;	// dependent extensions
} ${extArrName}_t;

EOF
);

printf (CODE "static %s_t %s[%s] = {\n", $extArrName, $extArrName, $constname3);
Parse (undef, "EmitPlatformCODE_EXT", "EmitExtensionCODE");
print (CODE "};\n\n");

print (HDR "\n\n");
Parse (undef, "EmitPlatformHDR_EXT", "EmitExtensionHDR");
print (HDR "\n");

# close files
close (HDR);
close (CODE);
