#!/usr/bin/perl

#--------------------- Defines -------------------------------

sub TEST {0}	# 0|1 - work|test

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if (not TEST) {
	#......... work .........
	$infile  = "qgl.in";
	$outhdr  = "../qgl_decl.h";
	$outcode = "../qgl_impl.h";
} else {
	#......... test .........
	print "*** TEST RUN ***\n";
	$infile  = "qgl.in.test";
	$outhdr  = "decl.h";
	$outcode = "impl.h";
}

$strucname   = "qgl";
$constname   = "NUM_GLFUNCS";
$constname2  = "NUM_EXTFUNCS";
$constname3  = "NUM_EXTENSIONS";
$extArrName  = "extInfo";
$log         = "fprintf (glw_state.log_fp, ";


#-------------- Platform-dependent stuff ---------------------

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


#--------------------- Functions -----------------------------


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
	my $str;

	return if TEST;		# do not generate this constants for test version

	for $str (@_)
	{
		printf (CODE "\tcase %s:\n\t\treturn \"%s\";\n", $str, $str =~ /GL_(\w+)/);
	}
}


#------------------- Parsing input file ----------------------


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
	printf (HDR "#define q%s%s\t%s.%s\n", $platform ? "" : "gl", $func, $strucname, $func);
}

sub EmitString {
	my $str;

	print (CODE ",") if $num > 0;
	if ($end =~ /NAME/)
	{
		($str) = $end =~ / .* NAME \s+ (\w+)/x;
	}
	else
	{
		$str = $func;
	}
	printf (CODE "\n\t\"%s%s\"", $platform ? "" : "gl", $str);
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
				if ($s =~ /\w*(int|sizei|byte|boolean)/) {
					$fmt = "%d";
				} elsif ($s =~ /\w*(float|double|clampd|clampf)/) {
					$fmt = "%g";
				} elsif ($s =~ /\w*(enum)/) {
					$fmt = "GL_%s";
				} elsif ($s =~ /.*\*/) {
					$fmt = "\$%X";
				} else {
					die "Unknown argument type \"", $s, "\"";
				}
				print (CODE ", ") if $i > 0;
				print (CODE $fmt);
				$i++;
			}
			printf (CODE ")\\n\", \"%s%s\"", $platform ? "" : "gl", $func);
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
		}
		elsif ($end =~ /;/)
		{
			#................... log function name only ..........................
			print (CODE "\t$log\"%s\\n\", \"", $platform ? "" : "gl", $func, "\");\n");	# make fprintf(..., "%s", "glFunction\n");
		}
		elsif ($end !~ /NAME/)
		{
			die "Invalid token: \"", $end, "\"";
		}

		# generate call to a library function
		if ($type !~ /\w*void$/)		# may be GLvoid
		{
			print (CODE "\treturn ");
		}
		else
		{
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


#----------------------- Main program --------------------------------

# open files
open (HDR, ">$outhdr") || die "Cannot create file ", $outhdr;
open (CODE, ">$outcode") || die "Cannot create file ", $outcode;

Parse ();
$i = 0;
for $ct (@numBase)
{
	printf ("Found %d + %d functions for %s\n", $ct, $numExt[$i], $platformNames[$i]);
	$i++;
}


#------------------- Creating header part ----------------------------

print (HDR "typedef void (APIENTRY * dummyFunc_t) (void);\n\n");
print (HDR "typedef union\n{\n\tstruct {\n");
Parse ("EmitStruc", "EmitPlatformHDR");
printf (HDR "\t};\n\tdummyFunc_t funcs[1];\n} %s_t;\n\n", $strucname);
printf (HDR "extern %s_t $strucname;\n\n", $strucname);

Parse ("EmitDefine", "EmitPlatformHDR");


#------------------- Creating code part ------------------------------

$i = 1;
while ($i <= $#numBase)
{
	print (CODE "#ifdef ", PlatformDefine ($i), "\n");
	printf (CODE "#  define %s\t%d\n", $constname, $numBase[0] + $numBase[$i]);
	printf (CODE "#  define %s\t%d\n", $constname2, $numExt[0] + $numExt[$i]);
	printf (CODE "#  define %s\t%d\n", $constname3, $numExtensions[0] + $numExtensions[$i]);
	print (CODE "#endif\n\n");
	$i++;
}
printf (CODE "static char *%sNames[%s + %s] = {", $strucname, $constname, $constname2);
Parse ("EmitString", "EmitPlatformCODE2");
print (CODE "\n};\n\n");

#------------------- Create EnumName () ------------------------------

print (CODE "static char *EnumName (GLenum v)\n{\n\tswitch (v)\n\t{\n");

const ("GL_LINE_LOOP", "GL_LINE_STRIP", "GL_TRIANGLES", "GL_TRIANGLE_STRIP", "GL_TRIANGLE_FAN", "GL_QUADS",
	"GL_QUAD_STRIP", "GL_POLYGON");											# begin mode
	#-- no GL_LINES (same as GL_ONE)
const ("GL_NEVER", "GL_LESS", "GL_EQUAL", "GL_LEQUAL", "GL_GREATER", "GL_NOTEQUAL", "GL_GEQUAL", "GL_ALWAYS");		# alpha func
const ("GL_ZERO", "GL_ONE", "GL_SRC_COLOR", "GL_ONE_MINUS_SRC_COLOR", "GL_SRC_ALPHA", "GL_ONE_MINUS_SRC_ALPHA",
	"GL_DST_ALPHA", "GL_ONE_MINUS_DST_ALPHA", "GL_DST_COLOR", "GL_ONE_MINUS_DST_COLOR", "GL_SRC_ALPHA_SATURATE");	# blend mode
const ("GL_FRONT", "GL_BACK");												# draw buffer
const ("GL_MODELVIEW", "GL_PROJECTION", "GL_TEXTURE");						# matrix mode
const ("GL_MODULATE", "GL_DECAL", "GL_REPLACE", "GL_ADD");					# texenv mode
const ("GL_TEXTURE_ENV_MODE", "GL_TEXTURE_ENV_COLOR");
const ("GL_TEXTURE_ENV", "GL_NEAREST", "GL_LINEAR", "GL_NEAREST_MIPMAP_NEAREST", "GL_LINEAR_MIPMAP_NEAREST",
	"GL_NEAREST_MIPMAP_LINEAR", "GL_LINEAR_MIPMAP_LINEAR", "GL_TEXTURE_MAG_FILTER", "GL_TEXTURE_MIN_FILTER",
	"GL_TEXTURE_WRAP_S", "GL_TEXTURE_WRAP_T");
const ("GL_CLAMP", "GL_REPEAT");
const ("GL_RGB5", "GL_RGB8", "GL_RGBA4", "GL_RGB5_A1", "GL_RGBA8",
	"GL_RGB4_S3TC", "GL_COMPRESSED_RGB_ARB", "GL_COMPRESSED_RGBA_ARB");		# texture formats
const ("GL_ACTIVE_TEXTURE_ARB", "GL_CLIENT_ACTIVE_TEXTURE_ARB", "GL_MAX_TEXTURE_UNITS_ARB", "GL_TEXTURE0_ARB", "GL_TEXTURE1_ARB",
	"GL_MAX_TEXTURES_SGIS", "GL_TEXTURE0_SGIS", "GL_TEXTURE1_SGIS");		# multitexturing
const ("GL_FOG", "GL_TEXTURE_2D", "GL_CULL_FACE", "GL_ALPHA_TEST", "GL_BLEND", "GL_DEPTH_TEST",
	"GL_VERTEX_ARRAY", "GL_COLOR_ARRAY", "GL_TEXTURE_COORD_ARRAY");			# enables/disables
const ("GL_BYTE", "GL_UNSIGNED_BYTE", "GL_SHORT", "GL_UNSIGNED_SHORT", "GL_INT", "GL_UNSIGNED_INT",
	"GL_FLOAT", "GL_DOUBLE");												# data types

print (CODE "\tdefault:\n\t\treturn \"???\";\n\t}\n}\n\n");

#------------------ Create log functions ---------------------------

Parse ("EmitLogDecl", "EmitPlatformCODE");

printf (CODE "static %s_t logFuncs = {", $strucname);
Parse ("EmitLogInit", "EmitPlatformCODE2");
print (CODE "\n};\n\n");


#-------------------- Extensions suport ----------------------------

print (CODE "typedef struct {\n\tchar\t*name;\n\tchar\t*cvar;\n\tshort\tfirst, count;\n\tint\t\trequire, deprecate;\n} ");
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
