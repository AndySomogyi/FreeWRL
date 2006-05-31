# $Id$
#
# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# Portions Copyright (C) 1998 Bernhard Reiter
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

#
# $Log$
# Revision 1.218  2006/05/31 14:52:28  crc_canada
# more changes to code for SAI.
#
# Revision 1.217  2006/05/29 17:54:12  crc_canada
# SAI changes - almost complete
#
# Revision 1.216  2006/05/24 19:29:03  crc_canada
# More VRML C Parser code
#
# Revision 1.215  2006/05/24 16:27:15  crc_canada
# more changes for VRML C parser.
#
# Revision 1.214  2006/05/23 23:46:49  crc_canada
# More generated code for VRML C parser.
#
# Revision 1.213  2006/05/23 16:15:10  crc_canada
# remove print statements, add more defines for a VRML C parser.
#
# Revision 1.212  2006/05/16 13:49:24  crc_canada
# Threading of shape loading now works. No menu buttons for it yet, though.
# It is a compile time option -DO_MULTI_OPENGL...
#
# Revision 1.211  2006/05/15 14:05:59  crc_canada
# Various fixes; CVS was down for a week. Multithreading for shape compile
# is the main one.
#
# Revision 1.210  2006/04/13 14:51:41  crc_canada
# EAI changes for SAI additions.
#
# Revision 1.209  2006/04/05 17:49:40  sdumoulin
# Universal binary build
#
# Revision 1.208  2006/03/09 20:43:33  crc_canada
# Initial Event Utilities Component work.
#
# Revision 1.207  2006/03/08 19:26:15  crc_canada
# addRemove children in javascript and EAI changed.
#
# Revision 1.206  2006/03/01 15:16:57  crc_canada
# Changed include file methodology and some Frustum work.
#
# Revision 1.205  2006/02/28 16:19:41  crc_canada
# BoundingBox
#
# Revision 1.204  2006/02/01 20:24:53  crc_canada
# MultiTexture work.
#
# Revision 1.203  2006/01/12 21:25:01  crc_canada
# More Occlusion stuff.
#
# Revision 1.202  2006/01/11 16:31:18  crc_canada
# starting Frustum Culling with Occlusion tests.
#
# Revision 1.201  2006/01/06 14:30:59  crc_canada
# OcclusionCulling starting.
#
# Revision 1.200  2005/12/22 14:50:30  crc_canada
# remove name field of X3D_Virt
#
# Revision 1.199  2005/12/22 14:20:58  crc_canada
# Group rendering problems fixed = sorting of children and alpha channels
#
# Revision 1.198  2005/12/21 18:16:40  crc_canada
# Rework Generation code.
#
# Revision 1.197  2005/12/16 18:31:25  crc_canada
# remove print debug statements
#
# Revision 1.196  2005/12/16 18:07:05  crc_canada
# rearrange perl generation
#
# Revision 1.195  2005/12/16 13:49:23  crc_canada
# updating generation functions.
#
# Revision 1.194  2005/12/15 20:42:01  crc_canada
# CoordinateInterpolator2D PositionInterpolator2D
#
# Revision 1.193  2005/12/10 20:26:18  crc_canada
# Move some functions into new "Component" files.
#
# Revision 1.192  2005/12/07 22:04:44  crc_canada
# replaceWorld functionality being added.
#
# Revision 1.191  2005/11/17 18:51:45  crc_canada
# revisit bindable nodes; add beginnings of TextureBackground
#
# Revision 1.190  2005/11/14 14:18:53  crc_canada
# Texture rework in progress...
#
# Revision 1.189  2005/11/09 14:25:19  crc_canada
# segfault fixing...
#
# Revision 1.188  2005/11/09 13:29:08  crc_canada
# TextureCoordinateGenerator nodes - first try
#
# Revision 1.187  2005/11/08 16:00:20  crc_canada
# reorg for 10.4.3 (OSX) dylib problem.
#
# Revision 1.186  2005/11/07 19:22:51  crc_canada
# OSX 10.4.3 broke FreeWRL - had to move defns out of VRMLC.pm
#
# Revision 1.185  2005/11/03 16:15:06  crc_canada
# MultiTextureTransform - textureTransforms changed considerably.
#
# Revision 1.184  2005/10/30 15:55:53  crc_canada
# Review the way nodes are identified at runtime.
#
# Revision 1.183  2005/10/29 16:24:00  crc_canada
# Polyrep rendering changes - step 1
#
# Revision 1.182  2005/10/27 13:47:23  crc_canada
# repeatS, repeatT flags were not handled correctly if same image but different flags.
# So, removed the "check image already loaded" code.
#
# Revision 1.181  2005/09/30 12:53:21  crc_canada
# Initial MultiTexture support.
#
# Revision 1.180  2005/09/29 03:01:13  crc_canada
# initial MultiTexture support
#
# Revision 1.179  2005/09/27 02:31:48  crc_canada
# cleanup of verbose code.
#
# Revision 1.178  2005/08/05 18:54:38  crc_canada
# ElevationGrid to new structure. works ok; still some minor errors.
#
# Revision 1.177  2005/08/04 14:39:38  crc_canada
# more work on moving elevationgrid to streaming polyrep structure
#
# Revision 1.176  2005/08/03 18:41:40  crc_canada
# Working on Polyrep structure.
#
# Revision 1.175  2005/08/02 13:22:44  crc_canada
# Move ElevationGrid to new face set order.
#
# Revision 1.174  2005/06/29 17:00:11  crc_canada
# EAI and X3D Triangle code added.
#
# Revision 1.173  2005/06/24 18:51:29  crc_canada
# more 64 bit compile changes.
#
# Revision 1.171  2005/06/09 14:52:49  crc_canada
# ColorRGBA nodes supported.
#
# Revision 1.170  2005/06/03 17:05:51  crc_canada
# More add/remove children from scripts/eai problems fixed. The ProdCon
# code had an invalid "complete" flag; the remove code in AddRemoveChildren
# had a pointer/contents of problem.
#
# Revision 1.169  2005/04/18 15:27:06  crc_canada
# speedup shapes that do not have textures by removing a glPushAttrib and glPopAttrib
#
# Revision 1.168  2005/04/06 16:56:08  crc_canada
# more OS X changes.
#
# Revision 1.167  2005/04/05 19:41:39  crc_canada
# some make problems fixed; this time back on Linux.
#
# Revision 1.166  2005/03/22 15:15:44  crc_canada
# more compile bugs; binary files were dinked in last upload.
#
# Revision 1.165  2005/03/22 13:25:24  crc_canada
# compile warnings reduced.
#
# Revision 1.164  2005/03/21 13:39:04  crc_canada
# change permissions, remove whitespace on file names, etc.
#
# Revision 1.163  2005/01/28 14:55:26  crc_canada
# Javascript SFImage works; Texture parsing changed to speed it up; and Cylinder side texcoords fixed.
#
# Revision 1.162  2005/01/18 20:52:33  crc_canada
# Make a ConsoleMessage that displays xmessage for running in a plugin.
#
# Revision 1.161  2005/01/16 20:55:08  crc_canada
# Various compile warnings removed; some code from Matt Ward for Alldev;
# some perl changes for generated code to get rid of warnings.
#
# Revision 1.160  2005/01/12 15:43:55  crc_canada
# TouchSensor hitPoint_changed and hitNormal_changed
#
# Revision 1.159  2004/12/01 21:19:07  crc_canada
# Anchor work.
#
# Revision 1.158  2004/10/22 19:02:25  crc_canada
# javascript work.
#
# Revision 1.157  2004/10/06 13:39:44  crc_canada
# Debian patches from Sam Hocevar.
#
# Revision 1.156  2004/09/30 20:11:55  crc_canada
# Bug fixes for EAI.
#
# Revision 1.155  2004/09/21 17:52:46  crc_canada
# make some rendering improvements.
#
# Revision 1.154  2004/09/15 19:21:18  crc_canada
# woops - problems with previous Sensitive rendering optimizations.
#
# Revision 1.153  2004/09/15 18:34:40  crc_canada
# Sensitive rendering pass now only traverses branches that have
# sensitive nodes in it. Speeds up (on my machine) tests/33.wrl from
# 14.71fps to 17.4fps.
#
# Revision 1.152  2004/09/08 18:58:58  crc_canada
# More Frustum culling work.
#
# Revision 1.151  2004/08/25 14:57:12  crc_canada
# more Frustum culling work
#
# Revision 1.150  2004/08/23 17:46:26  crc_canada
# Bulk commit: IndexedLineWidth width setting, and more Frustum culling work.
#
# Revision 1.149  2004/08/06 15:46:23  crc_canada
# if a fontStyle is a PROTO, expand the proto in NodeIntern.pm (used to
# segfault!)
#
# Revision 1.148  2004/07/21 19:04:00  crc_canada
# working on EXTERNPROTOS
#
# Revision 1.147  2004/07/16 13:17:50  crc_canada
# SFString as a Java .class script field entry.
#
# Revision 1.146  2004/07/12 13:30:36  crc_canada
# more steps to getting frustum culling working.
#
# Revision 1.145  2004/06/25 18:19:09  crc_canada
# EXTERNPROTO geturl; Solaris changes, and general changing the way URLs are
# handled.
#
# Revision 1.144  2004/06/10 20:05:52  crc_canada
# Extrusion (with concave endcaps) bug fixed; some javascript changes.
#
# Revision 1.143  2004/05/25 18:18:51  crc_canada
# more sorting of nodes
#
# Revision 1.140  2004/05/06 14:37:21  crc_canada
# more .class changes.
#
# Revision 1.138  2004/04/20 19:20:23  crc_canada
# Alberto Dubuc cleanup; java .class work.
#
# Revision 1.137  2004/04/02 21:32:42  crc_canada
# anchor work
#
# Revision 1.136  2004/03/29 20:39:54  crc_canada
# Compile for IRIX
#
# Revision 1.135  2004/03/29 19:14:14  crc_canada
# Irix compilation fixes
#
# Revision 1.134  2004/02/25 19:09:14  crc_canada
# code cleanup.
#

# To allow faster internal representations of nodes to be calculated,
# there is the field '_change' which can be compared between the node
# and the internal rep - if different, needs to be regenerated.
#
# the rep needs to be allocated if _intern == 0.
# XXX Freeing?!!?

require 'VRMLFields.pm';
require 'VRMLNodes.pm';
require 'VRMLRend.pm';

#######################################################################
#######################################################################
#######################################################################
#
# gen_struct - Generate a node structure, adding fields for
# internal use
my $interalNodeCommonFields = 
               "       struct X3D_Virt *v;\n"         	.
               "       int _renderFlags; /*sensitive, etc */ \n"                  	.
               "       int _sens; /*THIS is sensitive */ \n"                  	.
               "       int _hit; \n"                   	.
               "       int _change; \n"                	.
	       "       int _dlchange; \n"              	.
               "       GLuint _dlist; \n"              	.
	       "       void **_parents; \n"	  	.
	       "       int _nparents; \n"		.
	       "       int _nparalloc; \n"		.
	       "       int _ichange; \n"		.
	       "       float _dist; /*sorting for blending */ \n".
	       "       float _extent[6]; /* used for boundingboxes - +-x, +-y, +-z */ \n" .
               "       void *_intern; \n"              	.
               "       int _nodeType; /* unique integer for each type */ \n".
               " 	/*** node specific data: *****/\n";

sub gen_struct {
	my($name,$node) = @_;

	my @unsortedfields = keys %{$node->{FieldTypes}};

	# sort the field array, so that we can ensure the order of structure
	# elements.

	my @sf = sort(@unsortedfields);
	my $nf = scalar @sf;
	# /* Store actual point etc. later */
       my $s = "/***********************/\nstruct X3D_$name {\n" . $interalNodeCommonFields;

	my $o = "
void *
get_${name}_offsets(p)
	SV *p;
CODE:
	int *ptr_;
	STRLEN xx;
	SvGROW(p,($nf+1)*sizeof(int));
	SvCUR_set(p,($nf+1)*sizeof(int));
	ptr_ = (int *)SvPV(p,xx);
";
	my $p = " {
		my \$s = '';
		my \$v = get_${name}_offsets(\$s);
		\@{\$n->{$name}{Offs}}{".(join ',',map {"\"$_\""} @sf,'_end_')."} =
			unpack(\"i*\",\$s);
		\$n->{$name}{Virt} = \$v;
 }
	";

	for(@sf) {
		my $cty = "VRML::Field::$node->{FieldTypes}{$_}"->ctype($_);
		$s .= "\t$cty;\n";
		$o .= "\t*ptr_++ = offsetof(struct X3D_$name, $_);\n";
	}



	$o .= "\t*ptr_++ = sizeof(struct X3D_$name);\n";
	$o .= "\tRETVAL=&(virt_${name});\n";
	$o .= "\t /*printf(\"$name virtual: %d \\n \", RETVAL);  */
OUTPUT:
	RETVAL
";
	$s .= $ExtraMem{$name};
	$s .= "};\n";
	return ($s,$o,$p);
}

#########################################################
sub get_offsf {
	my($f) = @_;
	my ($ctp) = ("VRML::Field::$_")->ctype("*");
	my ($ct) = ("VRML::Field::$_")->ctype("*ptr_");
	my ($c) = ("VRML::Field::$_")->cfunc("(*ptr_)", "sv_");
	my ($ca) = ("VRML::Field::$_")->calloc("(*ptr_)");
	my ($cf) = ("VRML::Field::$_")->cfree("(*ptr_)");


	#print "get_offsf; for $f, have ca $ca and cf $cf\n";
	#print "and ctp $ctp\n";
	#print "and ct $ct\n";
	#print "c $c\n\n";

	# ifwe dont have to do anything, then we dont bother with the ctype field.
	# this gets rid of some compiler warnings.

	if ($ca) { #print "ca is something\n";
	} else { $ca = "UNUSED(ptr_);"; }
	if ($cf) { #print "cf is something\n";
	} else { $cf = "UNUSED(ptr_);"; }

	return "

void
set_offs_$f(ptr,offs,sv_)
	void *ptr
	int offs
	SV *sv_
CODE:
	$ct = ($ctp)(((char *)ptr)+offs);
	update_node(ptr);
	$c


void
alloc_offs_$f(ptr,offs)
	void *ptr
	int offs
CODE:
	$ct = ($ctp)(((char *)ptr)+offs);
	$ca

void
free_offs_$f(ptr,offs)
	void *ptr
	int offs
CODE:
	$ct = ($ctp)(((char *)ptr)+offs);
	$cf

"
}
#######################################################

sub get_rendfunc {
	my($n) = @_;
	#JAS print "RENDF $n ";
	# XXX
	my @f = qw/Prep Rend Child Fin RendRay GenPolyRep Light Changed Proximity Collision Compile/;
	my $comma = "";
	my $f = "extern struct X3D_Virt virt_${n};\n";
	my $v = "struct X3D_Virt virt_${n} = { ";

	for (@f) {
		# does this function exist?
		if (exists ${$_."C"}{$n}) {
			# it exists in the specified hash; now is the function in CFuncs, 
			# or generated here? (different names)
			if ($_ eq "Rend") {
				$v .= $comma."(void *)render_".${n};
			} elsif ($_ eq "Prep") {
				$v .= $comma."(void *)prep_".${n};
			} elsif ($_ eq "Fin") {
				$v .= $comma."(void *)fin_".${n};
			} elsif ($_ eq "Child") {
				$v .= $comma."(void *)child_".${n};
			} elsif ($_ eq "Light") {
				$v .= $comma."(void *)light_".${n};
			} elsif ($_ eq "Changed") {
				$v .= $comma."(void *)changed_".${n};
			} elsif ($_ eq "Proximity") {
				$v .= $comma."(void *)proximity_".${n};
			} elsif ($_ eq "Collision") {
				$v .= $comma."(void *)collide_".${n};
			} elsif ($_ eq "GenPolyRep") {
				$v .= $comma."(void *)make_".${n};
			} elsif ($_ eq "RendRay") {
				$v .= $comma."(void *)rendray_".${n};
			} elsif ($_ eq "Compile") {
				$v .= $comma."(void *)compile_".${n};
			} else {
				$v .= $comma."${n}_$_";
			}	
		} else {
			$v .= $comma."NULL";
		}
		$comma = ",";
	}
	$v .= "};\n";

	return ($f,$v);
}


######################################################################
######################################################################
######################################################################
#
# gen - the main function. this contains much verbatim code
#
#


{
        @NodeTypes = keys %VRML::Nodes;
        # foreach my $key (keys (%{$VRML::Nodes})) {print "field $key\n";}
}


#############################################################################################
sub gen {
	# make a table of nodetypes, so that at runtime we can determine what kind a
	# node is - comes in useful at times.
	# also create tables, functions, to create/manipulate VRML/X3D at runtime.


	# DESTINATIONS of arrays:
	#	@genFuncs1,	CFuncs/GeneratedCode.c (printed first)
	#	@genFuncs2,	CFuncs/GeneratedCode.c (printed second)
	#	@str,		CFuncs/Structs.h

	my $nodeIntegerType = 0; # make sure this maps to the same numbers in VRMLCU.pm
	my $fieldTypeCount = 0; 
	my $fieldNameCount = 0;
	my $keywordIntegerType = 0; 
	my %allFields = ();


	#####################
	# for CFuncs/GeneratedCode.c - create a header.
	push @genFuncs1,
		"/******************************************************************* \n".
		 "Copyright (C) 2006 Daniel Kraft, John Stewart (CRC Canada) \n".
		 "DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED. \n".
		 "See the GNU Library General Public License (file COPYING in the distribution) \n".
		 "for conditions of use and redistribution. \n".
		"*********************************************************************/ \n".
		"\n\n\n/* GENERATED BY VRMLC.pm - do NOT change this file */\n\n" .
		"#include \"headers.h\"\n".
		"#include \"Component_Geospatial.h\"\n".
		"#include \"Polyrep.h\"\n".
		"#include \"Bindable.h\"\n".
		"\n#define ARR_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))\n\n";

	#####################
	
	push @str, "/* Data type for index into ID-table. */\ntypedef size_t indexT;\n\n";

	# now, go through nodes, and do a few things:
	#	- create a "#define NODE_Shape 300" line for CFuncs/Structs.h;
	#	- create a "case NODE_Shape: printf ("Shape")" for CFuncs/GeneratedCode.c;
	#	- create a definitive list of all fieldnames.

        my @unsortedNodeList = keys %VRML::Nodes;
        my @sortedNodeList = sort(@unsortedNodeList);
	for (@sortedNodeList) {
		#print "node $_ is tagged as $nodeIntegerType\n";
		# tag each node type with a integer key.

		push @str, "#define NODE_".$_."	$nodeIntegerType\n";
		$nodeIntegerType ++;


		#{ use Devel::Peek 'Dump'; print "start of dump\n"; Dump $VRML::Nodes{$_}, 30; print "end of dump\n"; } 

 		#foreach my $field (keys %{$VRML::Nodes{$_}}) {print "field1 $field ". $VRML::Nodes{$_}{Fields}."\n";}
 		foreach my $field (keys %{$VRML::Nodes{$_}{FieldTypes}}) {
			$allFields{$field} = 1;
			#print "field2 $field\n"
		};
	}
	push @str, "\n";


	#####################
	# we have a list of fields from ALL nodes. print out the ones without the underscores at the beginning
	push @str, "\n/* Table of built-in fieldIds */\nextern const char *FIELDNAMES[];\n";
	push @str, "extern const indexT FIELDNAMES_COUNT;\n";

	push @genFuncs1, "\n/* Table of built-in fieldIds */\n       const char *FIELDNAMES[] = {\n";

	foreach (keys %allFields) { 
		#print "allFields $_\n";
		if (index($_,"_") !=0) {
			push @str, "#define FIELDNAMES_".$_."	$fieldNameCount\n";
			$fieldNameCount ++;
			push @genFuncs1, "	\"$_\",\n";
		}
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst indexT FIELDNAMES_COUNT = ARR_SIZE(FIELDNAMES);\n\n";
	
	# make a function to print field name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the field type */\n". 
		"const char *stringFieldType (int st) {\n".
		"	if ((st < 0) || (st >= FIELDNAMES_COUNT)) return \"FIELD OUT OF RANGE\"; \n".
		"	return FIELDNAMES[st];\n}\n\n";
	push @str, "const char *stringFieldType(int st);\n";

	#####################
	# process keywords 
	push @str, "\n/* Table of built-in keywords */\nextern const char *KEYWORDS[];\n";
	push @str, "extern const indexT KEYWORDS_COUNT;\n";

	push @genFuncs1, "\n/* Table of keywords */\n       const char *KEYWORDS[] = {\n";

        my @sf = keys %KeywordC;
	for (@sf) {
		# print "node $_ is tagged as $nodeIntegerType\n";
		# tag each node type with a integer key.
		push @str, "#define KW_".$_."	$keywordIntegerType\n";
		$keywordIntegerType ++;
		push @genFuncs1, "	\"$_\",\n";
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst indexT KEYWORDS_COUNT = ARR_SIZE(KEYWORDS);\n\n";

	# make a function to print Keyword name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the keyword type */\n". 
		"const char *stringKeywordType (int st) {\n".
		"	if ((st < 0) || (st >= KEYWORDS_COUNT)) return \"KEYWORD OUT OF RANGE\"; \n".
		"	return KEYWORDS[st];\n}\n\n";
	push @str, "const char *stringKeywordType(int st);\n";




	#####################
	# give each field an identifier
	push @str, "\n/* Table of built-in fieldIds */\nextern const char *FIELDTYPES[];\n";
	push @str, "extern const indexT FIELDTYPES_COUNT;\n";

	push @genFuncs1, "\n/* Table of Field Types */\n       const char *FIELDTYPES[] = {\n";

	for(@VRML::Fields) {
		# print "node $_ is tagged as $fieldTypeCount\n";
		# tag each node type with a integer key.
		my $defstr = "#define FIELDTYPE_".$_."	$fieldTypeCount\n";
		push @str, $defstr;
		$fieldTypeCount ++;
		$printNodeStr = "	\"$_\",\n";
		push @genFuncs1, $printNodeStr;
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst indexT FIELDTYPES_COUNT = ARR_SIZE(FIELDTYPES);\n\n";

	for(@VRML::Fields) {
		push @str, ("VRML::Field::$_")->cstruct . "\n";
		push @xsfn, get_offsf($_);
	}
	# make a function to print fieldtype name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the fieldtype type */\n". 
		"const char *stringFieldtypeType (int st) {\n".
		"	if ((st < 0) || (st >= FIELDTYPES_COUNT)) return \"FIELDTYPE OUT OF RANGE\"; \n".
		"	return FIELDTYPES[st];\n}\n\n";
	push @str, "const char *stringFieldtypeType(int st);\n";



	#####################
	# handle the nodes themselves
	push @str, "\n/* Table of built-in nodeIds */\nextern const char *NODES[];\n";
	push @str, "extern const indexT NODES_COUNT;\n";

	push @genFuncs1, "\n/* Table of Node Types */\n       const char *NODES[] = {\n";

        push @str, "\n/* and now the structs for the nodetypes */ \n";
	for(@sortedNodeList) {
		$printNodeStr = "	\"$_\",\n";
		push @genFuncs1, $printNodeStr;
	}
	push @genFuncs1, "};\nconst indexT NODES_COUNT = ARR_SIZE(NODES);\n\n";

	# make a function to print node name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the node type */\n". 
		"const char *stringNodeType (int st) {\n".
		"	if ((st < 0) || (st >= NODES_COUNT)) return \"NODE OUT OF RANGE\"; \n".
		"	return NODES[st];\n}\n\n";
	push @str, "const char *stringNodeType(int st);\n";


	###################
	# create the virtual tables for each node.
	push @str, "\n/* First, a generic struct, contains only the common elements */\n".
	"struct X3D_Node {\n". $interalNodeCommonFields .  "};\n".
	"#define X3D_NODE(node) ((struct X3D_Node*)node)\n".
	"\n/* now, generated structures for each VRML/X3D Node*/\n";


	push @genFuncs1, "/* Virtual tables for each node */\n";
	for(@sortedNodeList) {
		# print "working on node $_\n";
		my $no = $VRML::Nodes{$_};
		my($strret, $offs, $perl) = gen_struct($_, $no);
		push @str, $strret;
		push @xsfn, $offs;
		push @poffsfn, $perl;
		my($externdeclare, $vstru) = get_rendfunc($_);
		push @str, $externdeclare;
		push @genFuncs1, $vstru;
	}
	push @genFuncs1, "\n";


	#####################
	# create a routine to create a new node type

	push @genFuncs2,
	"/* create a new node of type. This can be generated by Perl code, much as the Structs.h is */\n".
	"void *createNewX3DNode (int nt) {\n".
	"	void * tmp;\n".
	"	struct X3D_Box *node;\n".
	"\n".
	"	tmp = NULL;\n".
	"\n".
	"	switch (nt) {\n";

	for (@sortedNodeList) {
		push @genFuncs2,
			"		case NODE_$_ : {tmp = malloc (sizeof (struct X3D_$_)); break;}\n";
	}
	push @genFuncs2, "		default: {printf (\"createNewX3DNode = unknown type %d, this will fail\\n\",nt); return NULL;}\n";
	
	push @genFuncs2,
	"	}\n\n".
	"	/* now, fill in the node to DEFAULT values This mimics \"alloc_struct\" in the Perl code. */\n".
	"	/* the common stuff between all nodes. We'll use a X3D_Box struct, just because. It is used\n".
	"	   in this way throughought the code */\n".
	"	node = (struct X3D_Box *) tmp;\n".
	"	node->_renderFlags = 0; /*sensitive, etc */\n".
	"	node->_sens = FALSE; /*THIS is sensitive */\n".
	"	node->_hit = 0;\n".
	"	node->_change = 153; \n".
	"	node->_dlchange = 0;\n".
	"	node->_dlist = 0;\n".
	"	node->_parents = 0;\n".
	"	node->_nparents = 0;\n".
	"	node->_nparalloc = 0;\n".
	"	node->_ichange = 0;\n".
	"	node->_dist = -10000.0; /*sorting for blending */\n".
	"	INITIALIZE_EXTENT\n".
	"	node->_intern = 0;\n".
	"	node->_nodeType = nt; /* unique integer for each type */\n".
	"	\n".
	"	/* now, fill in the node specific stuff here. the defaults are in VRMLNodes.pm */\n".
	"	switch (nt) {\n";

	for my $node (@sortedNodeList) {
		push @genFuncs2,
			"\t\tcase NODE_$node : {\n\t\t\tstruct X3D_$node * tmp2;\n";
		push @genFuncs2,
			"\t\t\ttmp2 = (struct X3D_$node *) tmp;\n\t\t\ttmp2->v = &virt_$node;\n";


		#print "\nnode $node:\n";
 		foreach my $field (keys %{$VRML::Nodes{$node}{Defaults}}) {
			my $ft = $VRML::Nodes{$node}{FieldTypes}{$field};
			my $fk = $VRML::Nodes{$node}{FieldKinds}{$field};
			my $def = $VRML::Nodes{$node}{Defaults}{$field};
			#print "	fieldX $field \n";
			#print "		fieldDefaults ". $VRML::Nodes{$node}{Defaults}{$field}."\n";
			#print "		fieldKinds ". $VRML::Nodes{$node}{FieldKinds}{$field}."\n";
			#print "		fieldTypes ". $VRML::Nodes{$node}{FieldTypes}{$field}."\n";
			if ($fk ne "eventIn") {
				#print "		do thisfield\n";
				my $cf = ("VRML::Field::$ft")->cInitialize("tmp2->".$field,$def);
				push @genFuncs2, "\t\t\t$cf;\n";
			}
		}
		
		push @genFuncs2,"\t\tbreak;\n\t\t}\n";
	}

	push @genFuncs2, "\t};\n\treturn tmp;\n}\n";

	#####################
	# create an array for each node. The array contains the following:
	# const int OFFSETS_Text[
	# 	FIELDNAMES_string, offsetof (struct X3D_Text, string), MFSTRING, KW_exposedField,
	#	FIELDNAMES_fontStype, offsetof (struct X3D_Text, fontStyle, SFNODE, KW_exposedField,
	# ....
	# 	-1, -1, -1, -1];
	# NOTES:
	# 1) we skip any field starting with an "_" (underscore)
	# 2) addChildren, removeChildren, map to children.
	# 
	for my $node (@sortedNodeList) {
		#print "node $_ is tagged as $nodeIntegerType\n";
		# tag each node type with a integer key.

		push @genFuncs1, "\nconst int OFFSETS_".$node."[] = {\n";

 		foreach my $field (keys %{$VRML::Nodes{$node}{Defaults}}) {
			if (index($field,"_") !=0) {
				my $ft = $VRML::Nodes{$node}{FieldTypes}{$field};
				$ft =~ tr/a-z/A-Z/; # convert to uppercase
				my $fk = $VRML::Nodes{$node}{FieldKinds}{$field};
				push @genFuncs1, "	FIELDNAMES_$field, offsetof (struct X3D_$node, $field), ".
					" $ft, KW_$fk,\n";
			}
		};
		push @genFuncs1, "	-1, -1, -1, -1};\n";
	}

	#####################
	# make an array that contains all of the OFFSETS created above.
	push @str, "\nextern const int *NODE_OFFSETS[];\n";
	push @genFuncs1, "\nconst int *NODE_OFFSETS[] = {\n";
	for my $node (@sortedNodeList) {
		push @genFuncs1, "	OFFSETS_$node,\n";
	}
	push @genFuncs1, "	};\n";


	#####################
	# print out generated functions to a file
	open GENFUNC, ">CFuncs/GeneratedCode.c";
	print GENFUNC join '',@genFuncs1;
	print GENFUNC join '',@genFuncs2;

	# print out structures to a file
	open STRUCTS, ">CFuncs/Structs.h";
	print STRUCTS '
/* Structs.h generated by VRMLC.pm. DO NOT MODIFY, MODIFY VRMLC.pm INSTEAD */

/* Code here comes almost verbatim from VRMLC.pm */

#ifndef STRUCTSH
#define STRUCTSH

/* OS X gives us a compile warning - hopefully this will cure it for all time */
#ifdef AQUA
//struct tm {};
#endif

/* Perl linking */
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

/* for time tick calculations */
#include <sys/time.h>

struct pt {GLdouble x,y,z;};
struct orient {GLdouble x,y,z,a;};

struct X3D_Virt {
	void (*prep)(void *);
	void (*rend)(void *);
	void (*children)(void *);
	void (*fin)(void *);
	void (*rendray)(void *);
	void (*mkpolyrep)(void *);
	void (*light)(void *);
	void (*changed)(void *);
	void (*proximity)(void *);
	void (*collision)(void *);
	void (*compile)(void *);
	/* char *name; */
};

/* Internal representation of IndexedFaceSet, Text, Extrusion & ElevationGrid:
 * set of triangles.
 * done so that we get rid of concave polygons etc.
 */
struct X3D_PolyRep { /* Currently a bit wasteful, because copying */
	int _change;
	int ccw;	/* ccw field for single faced structures */
	int ntri; /* number of triangles */
	int streamed;	/* is this done the streaming pass? */
	int alloc_tri; /* number of allocated triangles */
	int *cindex;   /* triples (per triangle) */
	float *coord; /* triples (per point) */
	int *colindex;   /* triples (per triangle) */
	float *color; /* triples or null */
	int *norindex;
	float *normal; /* triples or null */
        int *tcindex; /* triples or null */
        float *GeneratedTexCoords;	/* triples (per triangle) of texture coords if there is no texCoord node */
	int tcoordtype; /* type of texture coord node - is this a NODE_TextureCoordGenerator... */
};

/* viewer dimentions (for collision detection) */
struct sNaviInfo {
        double width;
        double height;
        double step;
};

';



	# print out the generated structures
	print STRUCTS join '',@NODEDEFS;
	print STRUCTS join '',@str;

	print STRUCTS '
#endif /* ifndef */
';

	open XS, ">VRMLFunc.xs";
	print XS '
/* VRMLFunc.c generated by VRMLC.pm. DO NOT MODIFY, MODIFY VRMLC.pm INSTEAD */

/* Code here comes almost verbatim from VRMLC.pm */

#include "CFuncs/headers.h"
#include "CFuncs/Component_Geospatial.h"
#include "CFuncs/jsUtils.h"
#include "CFuncs/Structs.h"
#include "XSUB.h"

#include <math.h>

#ifndef AQUA
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#else
#include <gl.h>
#include <glu.h>
#include <glext.h>
#endif

#include "CFuncs/Viewer.h"
#include "CFuncs/OpenGL_Utils.h"
#include "CFuncs/Collision.h"
#include "CFuncs/Bindable.h"
#include "CFuncs/Polyrep.h"
#include "CFuncs/sounds.h"
#include "CFuncs/SensInterps.h"

/*********************************************************************
 * Code here is generated from the hashes in VRMLC.pm and VRMLRend.pm
 */
	';

	print XS join '',@func;
	print XS join '',@vstruc;
	print XS <<'ENDHERE'


MODULE = VRML::VRMLFunc PACKAGE = VRML::VRMLFunc
PROTOTYPES: ENABLE




void *
alloc_struct(itype)
	int itype
CODE:
	void *ptr;
	struct X3D_Box *node;

	ptr = createNewX3DNode(itype);

	node = (struct X3D_Box *) ptr;

	/* printf("new Alloc: type %d -> %d\n", itype, ptr);   */
	RETVAL=ptr;
OUTPUT:
	RETVAL

void
set_field_be (ptr, field, value)
	void *ptr
	char *field
	char *value
CODE:
	int foffset;
	int coffset;
	int ctype;
	int ctmp;
	int isChildren;

	struct X3D_Box *node;
	node = (struct X3D_Box *)ptr;

	/* printf ("\nset_field_be, node %d field %s value %s\n", node, field, value);  */
	
	/* is this a child field? */
	if (strncmp("children",field,strlen("children")) == 0) {
		isChildren = TRUE;
	} else {
		isChildren = FALSE;
	}

	

	/* is this a valid field? */
	foffset = findFieldInFIELDNAMES(field);	
	/* printf ("field offset is %d\n",foffset); */
	if (foffset < 0) return;

	/* get offsets for this field in this nodeType */
	/* printf ("getting nodeOffsets for type %d\n",node->_nodeType); */
	findFieldInOFFSETS(NODE_OFFSETS[node->_nodeType], foffset, &coffset, &ctype, &ctmp);

	/* printf ("so, offset is %d, type %d value %s\n",coffset, ctype, value); */

	if (strlen(value)>0) 
		Perl_scanStringValueToMem(ptr, coffset, ctype, value,isChildren);
 
void
release_struct(ptr)
	void *ptr
CODE:
	struct X3D_Box *p;

	p = (struct X3D_Box *) ptr;

	if(p->_parents) free(p->_parents);
	if(p->_dlist) glDeleteLists(p->_dlist,1);
	printf ("release_struct, texture needs deletion \n");
	free(ptr); /* COULD BE MEMLEAK IF STUFF LEFT INSIDE */

void
set_sensitive(ptr,datanode,type)
	void *ptr
	void *datanode
	char *type
CODE:
	setSensitive (ptr,datanode,type);

#*****************************************************************************
# return a C pointer to a func for the interpolator functions. Used in CRoutes
# to enable event propagation to call the correct interpolator
#
void *
InterpPointer(x)
	char *x
CODE:
	void *pt;
	void do_Oint4(void *x);

	if (strncmp("OrientationInterpolator",x,strlen("OrientationInterpolator"))==0) {
		pt = (void *)do_Oint4;
		RETVAL = pt;
	} else if (strncmp("CoordinateInterpolator2D",x,strlen("CoordinateInterpolator2D"))==0) {
		pt = (void *)do_OintCoord2D;
		RETVAL = pt;
	} else if (strncmp("PositionInterpolator2D",x,strlen("PositionInterpolator2D"))==0) {
		pt = (void *)do_OintPos2D;
		RETVAL = pt;
	} else if (strncmp("ScalarInterpolator",x,strlen("ScalarInterpolator"))==0) {
		pt = (void *)do_OintScalar;
		RETVAL = pt;
	} else if (strncmp("ColorInterpolator",x,strlen("ColorInterpolator"))==0) {
		pt = (void *)do_Oint3;
		RETVAL = pt;
	} else if (strncmp("PositionInterpolator",x,strlen("PositionInterpolator"))==0) {
		pt = (void *)do_Oint3;
		RETVAL = pt;
	} else if (strncmp("CoordinateInterpolator",x,strlen("CoordinateInterpolator"))==0) {
		pt = (void *)do_OintCoord;
		RETVAL = pt;
	} else if (strncmp("NormalInterpolator",x,strlen("NormalInterpolator"))==0) {
		pt = (void *)do_OintCoord;
		RETVAL = pt;
	} else if (strncmp("GeoPositionInterpolator",x,strlen("GeoPositionInterpolator"))==0) {
		pt = (void *)do_GeoOint;
		RETVAL = pt;


	} else if (strncmp("BooleanFilter",x,strlen("BooleanFilter"))==0) {
		pt = (void *)do_BooleanFilter;
		RETVAL = pt;
	} else if (strncmp("BooleanSequencer",x,strlen("BooleanSequencer"))==0) {
		pt = (void *)do_BooleanSequencer;
		RETVAL = pt;
	} else if (strncmp("BooleanToggle",x,strlen("BooleanToggle"))==0) {
		pt = (void *)do_BooleanToggle;
		RETVAL = pt;
	} else if (strncmp("BooleanTrigger",x,strlen("BooleanTrigger"))==0) {
		pt = (void *)do_BooleanTrigger;
		RETVAL = pt;
	} else if (strncmp("IntegerTrigger",x,strlen("IntegerTrigger"))==0) {
		pt = (void *)do_IntegerTrigger;
		RETVAL = pt;
	} else if (strncmp("TimeTrigger",x,strlen("TimeTrigger"))==0) {
		pt = (void *)do_TimeTrigger;
		RETVAL = pt;

	} else {
		RETVAL = 0;
	}
OUTPUT:
	RETVAL


#*******************************************************************************
# return lengths if C types - used for Routing C to C structures. Platform
# dependent sizes...
# These have to match the clength subs in VRMLFields.pm
# the value of zero (0) is used in VRMLFields.pm to indicate a "don't know".
# some, eg MultiVec3f have a value of -1, and this is just passed back again.
# (this is handled by the routing code; it's more time consuming)
# others, eg ints, have a value from VRMLFields.pm of 1, and return the
# platform dependent size.

int
getClen(x)
	int x
CODE:
	switch (x) {
		case 1:	RETVAL = sizeof (int);
			break;
		case 2:	RETVAL = sizeof (float);
			break;
		case 3:	RETVAL = sizeof (double);
			break;
		case 4:	RETVAL = sizeof (struct SFRotation);
			break;
		case 5: RETVAL = sizeof (struct SFColor);
			break;
		case 6: RETVAL = sizeof (struct SFVec2f);
			break;
		case 7: RETVAL = sizeof (struct SFColorRGBA);
			break;
		case -11: RETVAL = sizeof (unsigned int);
			break;
		default:	RETVAL = x;
	}
OUTPUT:
	RETVAL



#********************************************************************************
#
# We have a new class invocation to worry about...

int
do_newJavaClass(scriptInvocationNumber,nodeURLstr,node)
	int scriptInvocationNumber
	char *nodeURLstr
	char *node
CODE:
	RETVAL = (int) newJavaClass(scriptInvocationNumber,nodeURLstr,node);
OUTPUT:
	RETVAL


#********************************************************************************
#
# register a route that can go via C, rather than perl.
#CRoutes_Register(int adrem, unsigned int from, int fromoffset, unsigned int to_count, char *tonode_str,
#                                 int length, void *intptr, int scrdir, int extra)


void
do_CRoutes_Register(adrem, from, fromoffset, to_count, tonode_str, len, intptr, scrpt, extra)
	int adrem
	void *from
	int fromoffset
	int to_count
	char *tonode_str
	int len
	void *intptr
	int scrpt
	int extra
CODE:
	CRoutes_Register(adrem, from, fromoffset, to_count, tonode_str, len, intptr, scrpt, extra);

#********************************************************************************
#
# Free memory allocated in CRoutes_Register

void
do_CRoutes_free()
CODE:
	CRoutes_free();


#********************************************************************************
# Viewer functions implemented in C replacing viewer Perl module

unsigned int
do_get_buffer()
CODE:
	RETVAL = get_buffer();
OUTPUT:
	RETVAL

void
set_root(rn)
	unsigned long  rn
CODE:
	/* printf ("VRMLC; set_root to %d\n", rn); */
	rootNode = (void *) rn;

#********************************************************************************
# Register a timesensitive node so that it gets "fired" every event loop

void
add_first(clocktype,node)
	char *clocktype
	void *node
CODE:
	add_first(clocktype,node);

#********************************************************************************

# save the specific FreeWRL version number from the Config files.
void
SaveVersion(str)
	char *str
CODE:
	BrowserVersion = (char *)malloc (strlen(str)+1);
	strcpy (BrowserVersion,str);


# EAI/SAI might want to know what kind of a file was just read in.
# SAI spec - getSpecification tells us that this is a 3 (VRML) or
# 4 (X3D)...
void
SaveFileType(c)
	int c
CODE:
	currentFileVersion = c;


SV *
GetBrowserURL()
CODE:
	RETVAL = newSVpv(BrowserURL, strlen(BrowserURL));
OUTPUT:
	RETVAL

SV *
GetBrowserFullPath()
CODE:
	RETVAL = newSVpv(BrowserFullPath, strlen(BrowserFullPath));
OUTPUT:
	RETVAL

# get the last file read in in InputFunctions.c
SV *
GetLastReadFile()
CODE:
	RETVAL = newSVpv(lastReadFile, strlen(lastReadFile));
OUTPUT:
	RETVAL


#****************JAVASCRIPT FUNCTIONS*********************************

## worry about garbage collection here ???
void
jsinit(num, sv_js)
	int num
	SV *sv_js
CODE:
	JSInit(num,sv_js);

int
jsrunScript(num, script, rstr, rnum)
	int num
	char *script
	SV *rstr
	SV *rnum
CODE:
	RETVAL = JSrunScript (num, script, rstr, rnum);
OUTPUT:
RETVAL
rstr
rnum


int
jsGetProperty(num, script, rstr)
	int num
	char *script
	SV *rstr
CODE:
	RETVAL = JSGetProperty (num, script, rstr);
OUTPUT:
RETVAL
rstr

int
addSFNodeProperty(num, nodeName, name, str)
	int num
	char *nodeName
	char *name
	char *str
CODE:
	RETVAL = JSaddSFNodeProperty(num, nodeName, name, str);
OUTPUT:
RETVAL


int
addGlobalAssignProperty(num, name, str)
	int num
	char *name
	char *str
CODE:
	RETVAL = JSaddGlobalAssignProperty(num, name, str);
OUTPUT:
RETVAL


int
addGlobalECMANativeProperty(num, name)
	int num
	char *name
CODE:
	RETVAL = JSaddGlobalECMANativeProperty(num, name);
OUTPUT:
RETVAL

int
paramIndex(evin, evtype)
	char *evin
	char *evtype
CODE:
	RETVAL = JSparamIndex (evin,evtype);
OUTPUT:
RETVAL


# allow Javascript to add/remove children when the parent is a USE - see JS/JS.pm.
void
jsManipulateChild(ptr, par, fiel, child)
	void *ptr
	void *par
	char *fiel
	int child
CODE:
	char onechildline[100];
	int flag;

	/* add (1), remove (2) or replace (0) */
	/* jsManipulateChild does only add or remove, not set... */
	if (strncmp(fiel,"addChild",strlen ("addChild")) == 0) {
		flag = 1;
	} else {
		flag = 2;
	}

	sprintf (onechildline, "[ %d ]",child);

	getMFNodetype (onechildline, (struct Multi_Node *) ptr,
		(struct X3D_Box *)par, flag);

# link into EAI.

int
EAIExtraMemory (type,size,data)
	char *type
	int size
	SV *data
	CODE:
	RETVAL = EAI_do_ExtraMemory (size,data,type);
OUTPUT:
RETVAL

# simple malloc - used for Java CLASS parameters
void *
malloc_this (size)
	int size
	CODE:
	RETVAL = malloc(size);
OUTPUT:
RETVAL

# print a string to the console, - this will be an xmessage
# window if we are running as a plugin.
void
ConsoleMessage (str)
	char *str;
	CODE:
	ConsoleMessage(str);


# remove comments, etc, from a string.
SV *
sanitizeInput(string)
	char *string
CODE:
	char *buffer;
	buffer = sanitizeInputString(string);
	RETVAL = newSVpv(buffer,strlen(buffer));
	OUTPUT:
RETVAL

# read in a string from a file
SV *
readFile(fn,parent)
	char *fn
	char *parent
CODE:
	char *buffer;

	buffer = readInputString(fn,parent);
	RETVAL= newSVpv(buffer,strlen(buffer));
	OUTPUT:
RETVAL


#****************END JAVASCRIPT FUNCTIONS*********************************

ENDHERE
;
	print XS '#**************************START XSFN*************************';
	print XS join '',@xsfn;
	print XS '#**************************END XSFN*************************
';

	open PM, ">VRMLFunc.pm";
	print PM "
# VRMLFunc.pm, generated by VRMLC.pm. DO NOT MODIFY, MODIFY VRMLC.pm INSTEAD
package VRML::VRMLFunc;
require DynaLoader;
require Exporter;
\@ISA=qw(Exporter DynaLoader);
";
	print PM "
bootstrap VRML::VRMLFunc;
sub load_data {
	my \$n = \\\%VRML::CNodes;
";
	print PM join '',@poffsfn;
	print PM "
}
";

}


gen();


