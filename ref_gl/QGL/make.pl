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
$log         = "fprintf (logFile, ";


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
		printf (CODE "\tcase GL_%s:\n\t\treturn \"%s\";\n", $str, $str);
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
	printf (HDR "\t\t%s\t(APIENTRY * %s) (%s);\n", $type, $func, $args);
}

sub EmitDefine {
	printf (HDR "#define %s\t%s.%s\n", $funcname, $strucname, $func);
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

	printf (CODE "static %s APIENTRY log%s (%s)\n{\n", $type, $func, $args);
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
			printf (CODE ")\\n\", \"%s\"", $funcname);
			$i = 0;
			for $s (@parm)
			{
				if ($atype[$i] =~ /\w*(enum)/) {
					printf (CODE ", EnumName(%s)", $s);
				} else {
	                printf (CODE ", %s", $s);
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
		printf (CODE "lib.%s (", $func);
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
	my $numFuncs;

	print (CODE ",\n") if $numExtensions[0] > 1 || $platform > 0;
	# name
	printf (CODE "\t{\"%s\", ", $extName);
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
			$extCvar = "";			# undefine cvar
			$firstExtFunc = $numExt[$platform];
			$prefer = "0";
			$require = "0";
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

print (HDR "typedef void (APIENTRY * dummyFunc_t) (void);\n\n");
print (HDR "typedef union\n{\n\tstruct {\n");
Parse ("EmitStruc", "EmitPlatformHDR");
printf (HDR "\t};\n\tdummyFunc_t funcs[1];\n} %s_t;\n\n", $strucname);
printf (HDR "extern %s_t $strucname;\n\n", $strucname);

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
	printf (CODE "#  define %s\t%d\n", $constname, $numBase[0] + $numBase[$i]);
	printf (CODE "#  define %s\t%d\n", $constname2, $numExt[0] + $numExt[$i]);
	printf (CODE "#  define %s\t%d\n", $constname3, $numExtensions[0] + $numExtensions[$i]);
	$i++;
}
print (CODE "#else\n");
printf (CODE "#  define %s\t%d\n", $constname, $numBase[0]);
printf (CODE "#  define %s\t%d\n", $constname2, $numExt[0]);
printf (CODE "#  define %s\t%d\n", $constname3, $numExtensions[0]);
print (CODE "#endif\n\n");

printf (CODE "static char *%sNames[%s + %s] = {", $strucname, $constname, $constname2);
Parse ("EmitString", "EmitPlatformCODE2");
print (CODE "\n};\n\n");

#------------------------------------------------------------------------------
#	Create EnumName ()
#------------------------------------------------------------------------------

print (CODE "static char *EnumName (GLenum v)\n{\n\tswitch (v)\n\t{\n");

open (IN, $constfile) || die "Input file ", $infile, " not found";

while (getline ())
{
	const ($line);
}



close (IN);

print (CODE "\tdefault:\n\t\treturn \"???\";\n\t}\n}\n\n");

#------------------------------------------------------------------------------
#	Create log functions
#------------------------------------------------------------------------------

Parse ("EmitLogDecl", "EmitPlatformCODE");

printf (CODE "static %s_t logFuncs = {", $strucname);
Parse ("EmitLogInit", "EmitPlatformCODE2");
print (CODE "\n};\n\n");


#------------------------------------------------------------------------------
#	Extensions suport
#------------------------------------------------------------------------------

print (CODE "typedef struct {\n\tchar\t*name;\n\tchar\t*cvar;\n\tshort\tfirst, count;\n\tunsigned require, deprecate;\n} ");
print (CODE $extArrName, "_t;\n\n");

printf (CODE "static %s_t %s[%s] = {\n", $extArrName, $extArrName, $constname3);
Parse (undef, "EmitPlatformCODE_EXT", "EmitExtensionCODE");
print (CODE "};\n\n");

print (HDR "\n\n");
Parse (undef, "EmitPlatformHDR_EXT", "EmitExtensionHDR");
print (HDR "\n");

# close files
close (HDR);
close (CODE);
