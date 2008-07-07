#
# $Id$
#
# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.
#
# Field types, parsing and printing, Perl, C and Java.
#
# SFNode is in Parse.pm
#
# $Log$
# Revision 1.82  2008/07/07 15:43:04  crc_canada
# SFDouble, MFDouble, some compiler warnings reduced.
#
# Revision 1.81  2008/07/04 18:19:44  crc_canada
# GeoPositionInterpolator, and start on GeoElevationGrid
#
# Revision 1.80  2008/06/24 19:37:48  crc_canada
# Geospatial, June 24 2008 checkin
#
# Revision 1.79  2008/06/13 13:50:48  crc_canada
# Geospatial, SF/MFVec3d support.
#
# Revision 1.78  2007/12/13 14:54:13  crc_canada
# code cleanup and change to inputOnly, outputOnly, initializeOnly, inputOutput
# ----------------------------------------------------------------------
#
# Revision 1.77  2007/03/20 20:36:10  crc_canada
# MALLOC/REALLOC macros to check mallocs for errors.
#
# Revision 1.76  2007/02/27 13:32:15  crc_canada
# initialize inputOnly fields to a zero value.
#
# Revision 1.75  2007/02/13 22:45:24  crc_canada
# PixelTexture default image should now be ok
#
# Revision 1.74  2006/12/19 19:05:01  crc_canada
# Memory leaks and corruptions being worked on.
#
# Revision 1.73  2006/10/23 18:28:11  crc_canada
# More changes and code cleanups.
#
# Revision 1.72  2006/10/19 18:28:46  crc_canada
# More changes for removing Perl from the runtime
#
# Revision 1.71  2006/10/18 20:22:43  crc_canada
# More removal of Perl code
#
# Revision 1.70  2006/08/18 17:47:37  crc_canada
# Javascript initialization
#
#

# XXX Decide what's the forward assertion..

@VRML::Fields = qw/
	SFFloat
	MFFloat
	SFRotation
	MFRotation
	SFVec3f
	MFVec3f
	SFBool
	MFBool
	SFInt32
	MFInt32
	SFNode
	MFNode
	SFColor
	MFColor
	SFColorRGBA
	MFColorRGBA
	SFTime
	MFTime
	SFString
	MFString
	SFVec2f
	MFVec2f
	SFImage
	FreeWRLPTR
	SFVec3d
	MFVec3d
	SFDouble
	MFDouble
	DFRotation
/;

###########################################################
package VRML::Field;
VRML::Error->import();

# The C type interface for the field type, encapsulated
# By encapsulating things well enough, we'll be able to completely
# change the interface later, e.g. to fit together with javascript etc.
sub ctype ($) {my ($type) = @_; die "VRML::Field::ctype - fori $type abstract function called"}
sub cstruct () {"/*cstruct*/"}

###########################################################
package VRML::Field::SFFloat;
@ISA=VRML::Field;
VRML::Error->import();

sub ctype {"float $_[1]"}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {$val = 0.0} # inputOnlys, set it to any value
	return "$field = $val";
}


###########################################################
package VRML::Field::SFTime;
@ISA=VRML::Field::SFFloat;

sub ctype {"double $_[1]"}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {$val = 0.0} # inputOnlys, set it to any value
	return "$field = $val";
}


###########################################################
package VRML::Field::SFDouble;
@ISA=VRML::Field;
VRML::Error->import;

sub ctype {return "double $_[1]"}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {$val = 0} # inputOnlys, set it to any value
	return "$field = $val";
}

###########################################################
package VRML::Field::FreeWRLPTR;
@ISA=VRML::Field;
VRML::Error->import;



sub ctype {return "void * $_[1]"}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {$val = 0} # inputOnlys, set it to any value
	return "$field = $val";
}

###########################################################
###########################################################
package VRML::Field::SFInt32;
@ISA=VRML::Field;
VRML::Error->import;



sub ctype {return "int $_[1]"}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {$val = 0} # inputOnlys, set it to any value
	return "$field = $val";
}


###########################################################
package VRML::Field::SFColor;
@ISA=VRML::Field;
VRML::Error->import;



sub cstruct {return "struct SFColor { float c[3]; };"}
sub ctype {return "struct SFColor $_[1]"}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {print "undefined in SFColor\n"} # inputOnlys, set it to any value
	return 	"$field.c[0] = @{$val}[0];".
		"$field.c[1] = @{$val}[1];".
		"$field.c[2] = @{$val}[2];";
}

###########################################################
package VRML::Field::SFColorRGBA;
@ISA=VRML::Field;
VRML::Error->import();


sub cstruct {return "struct SFColorRGBA { float r[4]; };"}
sub ctype {return "struct SFColorRGBA $_[1]"}
sub cInitialize { print "SFCOLORRGBA INIT\n"; return 0;
}

###########################################################
package VRML::Field::SFVec3f;
@ISA=VRML::Field::SFColor;
sub cstruct {return ""}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {print "undefined in SFVec3f\n"} # inputOnlys, set it to any value
	return 	"$field.c[0] = @{$val}[0];".
		"$field.c[1] = @{$val}[1];".
		"$field.c[2] = @{$val}[2];";
}

###########################################################
package VRML::Field::SFVec3d;
@ISA=VRML::Field;
sub cstruct {return "struct SFVec3d { double c[3]; };"}
sub ctype {return "struct SFVec3d $_[1]";}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {print "undefined in SFVec3d\n"} # inputOnlys, set it to any value
	return 	"$field.c[0] = @{$val}[0];".
		"$field.c[1] = @{$val}[1];".
		"$field.c[2] = @{$val}[2];";
}

###########################################################
package VRML::Field::SFVec2f;
@ISA=VRML::Field;
VRML::Error->import();

sub cstruct {return "struct SFVec2f { float c[2]; };"}
sub ctype {return "struct SFVec2f $_[1]"}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {print "undefined in SFVec2f\n"} # inputOnlys, set it to any value
	return 	"$field.c[0] = @{$val}[0];".
		"$field.c[1] = @{$val}[1];";
}


###########################################################
package VRML::Field::SFRotation;
@ISA=VRML::Field;
VRML::Error->import();


sub cstruct {return "struct SFRotation { float r[4]; };"}

sub ctype {return "struct SFRotation $_[1]"}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {print "undefined in SFRotation\n"} # inputOnlys, set it to any value
	return 	"$field.r[0] = @{$val}[0];".
		"$field.r[1] = @{$val}[1];".
		"$field.r[2] = @{$val}[2];".
		"$field.r[3] = @{$val}[3];";
}

package VRML::Field::DFRotation;
@ISA=VRML::Field;
VRML::Error->import();


sub cstruct {return "struct DFRotation { double r[4]; };"}

sub ctype {return "struct DFRotation $_[1]"}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {print "undefined in DFRotation\n"} # inputOnlys, set it to any value
	return 	"$field.r[0] = @{$val}[0];".
		"$field.r[1] = @{$val}[1];".
		"$field.r[2] = @{$val}[2];".
		"$field.r[3] = @{$val}[3];";
}


###########################################################
package VRML::Field::SFBool;
@ISA=VRML::Field;


sub ctype {return "int $_[1]"}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {$val = 0} # inputOnlys, set it to any value
	return "$field = $val";
}


###########################################################
package VRML::Field::SFString;
@ISA=VRML::Field;

sub ctype {return "struct Uni_String *$_[1]"}
sub cInitialize {
	my ($this,$field,$val) = @_;

	if (!defined $val) {$val = "";} # inputOnlys, set it to any value
	if ($field eq "tmp2->__parenturl") {
		return "$field = newASCIIString(getInputURL())";
	} else {
		return "$field = newASCIIString(\"$val\")";
	}
}

###########################################################
package VRML::Field::MFString;
@ISA=VRML::Field::Multi;


sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	my $retstr;
	my $tmp;

	if (!defined $val) {$count=0} # inputOnlys, set it to any value
	#print "MFSTRING field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		#print "MALLOC MFSTRING field $field val @{$val} has $count INIT\n";
		$retstr = $restsr . "$field.p = MALLOC (sizeof(struct Uni_String)*$count);";
		for ($tmp=0; $tmp<$count; $tmp++) {
			$retstr = $retstr .  "$field.p[$tmp] = newASCIIString(\"".@{$val}[$tmp]."\");";
		}
		$retstr = $retstr . "$field.n=$count; ";
		
	} else {
		$retstr = "$field.n=0; $field.p=0";
	}
	return $retstr;
}

# XXX Should be optimized heavily! Other MFs are ok.

###########################################################
package VRML::Field::MFFloat;
@ISA=VRML::Field::Multi;

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	my $retstr;
	my $tmp;

	if (!defined $val) {$count = 0;} # inputOnlys, set it to any value
	#print "MFFLOAT field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		#print "MALLOC MFFLOAT field $field val @{$val} has $count INIT\n";
		$retstr = $restsr . "$field.p = MALLOC (sizeof(float)*$count);\n";
		for ($tmp=0; $tmp<$count; $tmp++) {
			$retstr = $retstr .  "\t\t\t$field.p[$tmp] = @{$val}[tmp];\n";
		}
		$retstr = $retstr . "\t\t\t$field.n=$count;";
		
	} else {
		$retstr =  "$field.n=0; $field.p=0";
	}
	return $retstr;
}



###########################################################
package VRML::Field::MFTime;
@ISA=VRML::Field::MFFloat;

sub cInitialize {
	print "MFTIME INIT\n";
}

###########################################################
package VRML::Field::MFVec3f;
@ISA=VRML::Field::Multi;

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	my $retstr;
	my $tmp;
	my $whichVal;

	if (!defined $val) {$count = 0;} # inputOnlys, set it to any value
	#print "MFVEC3F field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		#print "MALLOC MFVEC3F field $field val @{$val} has $count INIT\n";

		$retstr = "$field.p = MALLOC (sizeof(struct SFColor)*$count);\n";
		for ($tmp=0; $tmp<$count; $tmp++) {
			my $arline = @{$val}[$tmp];
			for ($whichVal = 0; $whichVal < 3; $whichVal++) {
				$retstr = $retstr. "\n\t\t\t$field.p[$tmp].c[$whichVal] = ".@{$arline}[$whichVal]."; ";
			}
		}
		$retstr = $retstr."\n\t\t\t$field.n=$count;";
	} else {
		$retstr = "$field.n=0; $field.p=0";
	}
	return $retstr;
}

###########################################################
package VRML::Field::MFVec3d;
@ISA=VRML::Field::Multi;

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	my $retstr;
	my $tmp;

	if (!defined $val) {$count = 0;} # inputOnlys, set it to any value
	#print "MFVEC3F field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		#print "MALLOC MFVEC3d field $field val @{$val} has $count INIT\n";

		$retstr = "$field.p = MALLOC (sizeof(struct SFVec3d)*$count);\n";
		for ($tmp=0; $tmp<$count; $tmp++) {
			my $arline = @{$val}[$tmp];
			for ($whichVal = 0; $whichVal < 3; $whichVal++) {
				$retstr = $retstr. "\n\t\t\t$field.p[$tmp].c[$whichVal] = ".@{$arline}[$whichVal]."; ";
			}
		}
		$retstr = $retstr."\n\t\t\t$field.n=$count;";
	} else {
		$retstr = "$field.n=0; $field.p=0";
	}
	return $retstr;
}


###########################################################
package VRML::Field::MFNode;
@ISA=VRML::Field::Multi;

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	if (!defined $val) {$count=0;} # inputOnlys, set it to any value
	#print "MFNODE field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		print "MFNODE HAVE TO MALLOC HERE\n";
	} else {
		return "$field.n=0; $field.p=0";
	}
}


###########################################################
package VRML::Field::MFColor;
@ISA=VRML::Field::Multi;

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	my $retstr;
	my $tmp;
	my $whichVal;

	if (!defined $val) {$count=0} # inputOnlys, set it to any value
	#print "MFCOLOR field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		#print "MALLOC MFCOLOR field $field val @{$val} has $count INIT\n";

		$retstr = "$field.p = MALLOC (sizeof(struct SFColor)*$count);\n";
		for ($tmp=0; $tmp<$count; $tmp++) {
			my $arline = @{$val}[$tmp];
			for ($whichVal = 0; $whichVal < 3; $whichVal++) {
				$retstr = $retstr. "\n\t\t\t$field.p[$tmp].c[$whichVal] = ".@{$arline}[$whichVal]."; ";
			}
		}
		$retstr = $retstr."\n\t\t\t$field.n=$count;";
	} else {
		$retstr = "$field.n=0; $field.p=0";
	}
	return $retstr;
}

###########################################################
package VRML::Field::MFColorRGBA;
@ISA=VRML::Field::Multi;


sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	my $retstr;
	my $tmp;

	if (!defined $val) {$count=0} # inputOnlys, set it to any value
	#print "MFCOLORRGBA field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		#print "MALLOC MFROTATION field $field val @{$val} has $count INIT\n";

		$retstr = "$field.p = MALLOC (sizeof(struct SFColorRGBA)*$count);\n";
		for ($tmp=0; $tmp<$count; $tmp++) {
			my $arline = @{$val}[$tmp];
			for ($whichVal = 0; $whichVal < 4; $whichVal++) {
				$retstr = $retstr. "\n\t\t\t$field.p[$tmp].r[$whichVal] = ".@{$arline}[$whichVal]."; ";
			}
		}
		$retstr = $retstr."\n\t\t\t$field.n=$count;";
	} else {
		$retstr = "$field.n=0; $field.p=0";
	}
	return $retstr;
}

###########################################################
package VRML::Field::MFVec2f;
@ISA=VRML::Field::Multi;


sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	my $retstr;
	my $tmp;
	my $whichVal;

	if (!defined $val) {$count=0} # inputOnlys, set it to any value
	#print "MFVec2F field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		#print "MALLOC MFVec2F field $field val @{$val} has $count INIT\n";

		$retstr = "$field.p = MALLOC (sizeof(struct SFVec2f)*$count);";
		for ($tmp=0; $tmp<$count; $tmp++) {
			my $arline = @{$val}[$tmp];
			for ($whichVal = 0; $whichVal < 2; $whichVal++) {
				$retstr = $retstr. "\n\t\t\t$field.p[$tmp].c[$whichVal] = ".@{$arline}[$whichVal]."; ";
			}
		}
		$retstr = $retstr."\n\t\t\t$field.n=$count";

	} else {
		$retstr = "$field.n=0; $field.p=0";
	}
	return $retstr;
}

###########################################################
package VRML::Field::MFInt32;
@ISA=VRML::Field::Multi;

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	#print "MFINT32 field $field val @{$val} has $count INIT\n";
	if (!defined $val) {$count=0} # inputOnlys, set it to any value
	if ($count > 0) {
		print "MFINT32 HAVE TO MALLOC HERE\n";
	} else {
		return "$field.n=0; $field.p=0";
	}
}
###########################################################
package VRML::Field::MFDouble;
@ISA=VRML::Field::Multi;

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	my $retstr;
	my $tmp;

	#print "MFDouble field $field val @{$val} has $count INIT\n";
	if (!defined $val) {$count=0} # inputOnlys, set it to any value
	if ($count > 0) {
		#print "MALLOC MFDouble field $field val @{$val} has $count INIT\n";
		$retstr = $restsr . "$field.p = MALLOC (sizeof(double)*$count);\n";
		for ($tmp=0; $tmp<$count; $tmp++) {
			$retstr = $retstr .  "\t\t\t$field.p[$tmp] = @{$val}[tmp];\n";
		}
		$retstr = $retstr . "\t\t\t$field.n=$count;";
	} else {
		$retstr =  "$field.n=0; $field.p=0";
	}
	return $retstr;
}

###########################################################
package VRML::Field::MFBool;
@ISA=VRML::Field::Multi;

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	my $retstr;
	my $tmp;

	if (!defined $val) {$count=0} # inputOnlys, set it to any value
	#print "MFBOOL field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		#print "MALLOC MFBOOL field $field val @{$val} has $count INIT\n";

		$retstr = $restsr . "$field.p = MALLOC (sizeof(int)*$count);\n";
		for ($tmp=0; $tmp<$count; $tmp++) {
			my $arline = @{$val}[$tmp];
			$retstr = $retstr. "\n\t\t\t$field.p[$tmp] = $arline; ";
		}
		$retstr = $retstr."$field.n=$count;";
	} else {
		$retstr = "$field.n=0; $field.p=0";
	}
	return $retstr;
}

###########################################################
package VRML::Field::MFRotation;
@ISA=VRML::Field::Multi;


sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	my $retstr;
	my $tmp;
	my $whichVal;

	if (!defined $val) {$count=0} # inputOnlys, set it to any value
	#print "MFROTATION field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		#print "MALLOC MFROTATION field $field val @{$val} has $count INIT\n";

		$retstr = "$field.p = MALLOC (sizeof(struct SFRotation)*$count);\n";
		for ($tmp=0; $tmp<$count; $tmp++) {
			my $arline = @{$val}[$tmp];
			for ($whichVal = 0; $whichVal < 4; $whichVal++) {
				$retstr = $retstr. "\n\t\t\t$field.p[$tmp].r[$whichVal] = ".@{$arline}[$whichVal]."; ";
			}
		}
		$retstr = $retstr."\n\t\t\t$field.n=$count;";
	} else {
		$retstr =  "$field.n=0; $field.p=0";
	}
	return $retstr;
}

###########################################################
package VRML::Field::Multi;
@ISA=VRML::Field;


sub ctype {
	my $r = (ref $_[0] or $_[0]);
	$r =~ s/VRML::Field::MF//;
	return "struct Multi_$r $_[1]";
}
sub cstruct {
	my $r = (ref $_[0] or $_[0]);
	my $t = $r;
	$r =~ s/VRML::Field::MF//;
	$t =~ s/::MF/::SF/;
	my $ct = $t->ctype;
	return "struct Multi_$r { int n; $ct *p; };"
}


###########################################################
package VRML::Field::SFNode;


sub ctype {"void *$_[1]"}      # XXX ???
sub cstruct {""}

sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {print "undefined in SFNode\n"} # inputOnlys, set it to any value
	return "$field = $val";
}

###########################################################
package VRML::Field::SFImage;
@ISA=VRML::Field;
VRML::Error->import;


sub ctype {return "struct Multi_Int32 $_[1]"}

sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {print "undefined in SFImage\n"} # inputOnlys, set it to any value
	my $count = @{$val};
	#SFImage defaults to 0,0,0\n";
	return "$field.n=3; $field.p=MALLOC (sizeof(int)*3); $field.p[0] = 0; $field.p[1] = 0; $field.p[2] = 0;";
}

1;

