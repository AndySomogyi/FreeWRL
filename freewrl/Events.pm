# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart - CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

# Events.pm -- event handling code for VRML::Browser.
#
# We take all the nodes referenced by events in the scenegraph
# and at each timestep ask each node able to produce events
# whether it wants to right now.
#
# Then we propagate the events along the routes. 

package VRML::EventMachine;


# to save processing power, we only render when we need to.
my $havevent = 0;

sub new {
	my($type) = @_;
	bless {
		   First => {},
		   Listen => undef,
		   IS => undef,
		   Queue => undef,
		   Route => undef,
		  },$type;
}

sub print {
	my($this) = @_;
	return unless $VRML::verbose::events;
	my $handle;

	print "DUMPING EventModel\nFIRST:\n";
	for(values %{$this->{First}}) {
		print "\t",VRML::NodeIntern::dump_name($_),":\t$_->{TypeName}\n";
	}

	print "ROUTE hash: ", VRML::Debug::toString($this->{Route}), "\n";
	print "\nIS hash: ", VRML::Debug::toString($this->{IS}), "\n";
}

#################################################################################
#
# a PROTO interface declaration may have a variable that is referenced in Routes,
# but is never IS'd.  EAI uses things like this. We REQUIRE memory in order to route
# to,from; so tell Events where we are going to store it.
# 
# eg:
#
#	PROTO VNetInfo [ exposedField  SFFloat   brightness   0.5 ] { Group {} }
#	Transform {
#	  children [
#	    DEF VNET VNetInfo { isConnected TRUE  brightness 0.75}
#	    DEF LIGHT DirectionalLight....
#	  ]
#	  ROUTE LIGHT.intensity TO VNET.brightness
#	}

my %ExtraMem = ();
sub ExtraMemory {
	my ($node,$field) = @_;

	my $memptr;
	my $value;
	
	print "ExtraMemory: wanting field $field for node ",VRML::NodeIntern::dump_name($node),"\n";
	foreach (keys%{$node}) {print "	node key $_\n";}
	foreach (keys%{$node->{Scene}}) {print "	nodeScene key $_\n";}
	foreach (keys%{$node->{Scene}{Pars}}) {print "	nodeScenePars key $_\n";}

	my $type = $node->{Scene}{Pars}{$field}[1];

	# test to see if this is a valid type
	if ($type eq "") { return (0, 0, 0);}	

	my $clen = VRML::VRMLFunc::getClen("VRML::Field::$type"->clength($type));

	# have we seen this one before?
	if (exists $ExtraMem{"$node$field"}) {
		# print "ExtraMemory, already found $node $field, returning \n";
		$memptr = $ExtraMem{"$node$field"};
	} else {
		$value = $node->{Scene}{NodeParent}{Fields}{$field};
		$memptr = VRML::VRMLFunc::EAIExtraMemory ($type,$clen,$value);

		# save this, so we know if we have already allocated memory for it.
		$ExtraMem{"$node$field"} = $memptr;
	}

	print "Events.pm: ExtraMemory: field $field Clength $clen  type $type value $value address $memptr\n";
	return ($memptr ,$type,$clen);
}




###############################################################################
#
# Nodes get stored in many ways, depending on whether it is a PROTO, normal
# node, etc, etc. This tries to find a backend (CNode, or script) for a node.

# node = the node pointer; whatever we can get
# field = the field name, eg, "clicked"
# direction = "eventOut" or "eventIn".

# returns node, field, script, ok

###
# this needs some cleaning up!

sub resolve_node_cnode {
	my ($this, $scene, $node, $field, $direction) = @_;

	# return values.
	my $outptr = 0;
	my $outoffset = 0;
	my $to_count = 0;
	my $tonode_str = "";
	my @tonodes;
	my $scrpt = 0;
	my $il = 0;
	my $ok = 0;
	#JAS my $cs;
	my $proto_node;
	my $proto_field;
	my $is_proto;
	my $tmp;
	my $fieldtype = "";
	my $clen = 0;		# length of the data  - check out "sub clength"

	#print "\nVRML::EventMachine::resolve_node_cnode: ",
	#	VRML::Debug::toString(\@_), "\n" ;

	$tmp = VRML::Handles::get($node);
	if (ref $tmp eq "VRML::NodeIntern") {
		$node = $tmp;
	} else {
		$node = $scene->getNode($tmp);
		if (!defined $node) {
			warn("DEF node $tmp is not defined");
			return (0,0,0,0,0);
		}
	}
	#print "resolve node cnode, node $node, field $field, value ",
	#$node->{Fields}{$field},"\n";
	#	print "handle got $node ",
	#	($node->{IsProto} ?
	#	 "PROTO ".VRML::NodeIntern::dump_name($node->{ProtoExp})." " : " is not Proto "),
	#		 "$node->{TypeName} ", VRML::NodeIntern::dump_name($node),"\n";

	my $f;
	my @is;

#JAS - IsProto now cleanly handled in getNodeCnode and associated calls.

#JAS	# is this an IS?
#JAS	if ($node->{IsProto} && exists $this->{IS}{$node}{$field}) {
#JAS		print "VRML::EventMachine::resolve_node_cnode:ISPROTO\n";
#JAS		$is_proto = 1;
#JAS		push @is, @{$this->{IS}{$node}{$field}};
#JAS
#JAS		if ($direction =~ /eventIn/) {
#JAS			while (@is) {
#JAS				$f = shift @is;
#JAS				$proto_node = $f->[0];
#JAS				$proto_field = $f->[1];
#JAS
#JAS				next if ($proto_node->{TypeName} =~ /script/i);
#JAS
#JAS				## see VRML::Field::SFNode::parse for why this is necessary
#JAS				if (defined ($outptr = $proto_node->{BackNode}{CNode})) {
#JAS					if ($proto_field =~ /^[0-9]+($VRML::Error::Word)$/) {
#JAS						$proto_field = $1;
#JAS					}
#JAS					if (!defined ($outoffset =
#JAS								  $VRML::CNodes{$proto_node->{TypeName}}{Offs}{$proto_field})) {
#JAS						print "No offset for $proto_field.\n";
#JAS						$outptr = undef;
#JAS					} else {
#JAS						#print "$proto_node->{TypeName} ",
#JAS						#	VRML::NodeIntern::dump_name($proto_node),
#JAS						#			" CNode: $outptr, $proto_field eventIn: $outoffset.\n";
#JAS						push @tonodes, "$outptr:$outoffset";
#JAS						$to_count++;
#JAS					}
#JAS				} elsif ($field eq $proto_field) { # EXTERNPROTO handling
#JAS					push @is, @{$this->{IS}{$proto_node}{$field}};
#JAS					next;
#JAS				} else {
#JAS					print "No CNode for $proto_node->{TypeName} ",
#JAS						VRML::NodeIntern::dump_name($proto_node), " from IS hash.\n";
#JAS				}
#JAS			}
#JAS			$tonode_str = join(" ", @tonodes);
#JAS		} else { # XXX PROTO multiple eventOuts not handled yet
#JAS			while (@is) {
#JAS				$f = shift @is;
#JAS				$proto_node = $f->[0];
#JAS				$proto_field = $f->[1];
#JAS
#JAS				next if ($proto_node->{TypeName} =~ /script/i);
#JAS
#JAS				## see VRML::Field::SFNode::parse for why this is necessary
#JAS				if (defined ($outptr = $proto_node->{BackNode}{CNode})) {
#JAS					if ($proto_field =~ /^[0-9]+($VRML::Error::Word)$/) {
#JAS						$proto_field = $1;
#JAS					}
#JAS					if (!defined ($outoffset =
#JAS								  $VRML::CNodes{$proto_node->{TypeName}}{Offs}{$proto_field})) {
#JAS						print "No offset for $proto_field.\n";
#JAS						$outptr = undef;
#JAS					} else {
#JAS						#print "$proto_node->{TypeName} ",
#JAS						#	VRML::NodeIntern::dump_name($proto_node),
#JAS						#			" CNode: $outptr, $proto_field eventOut: $outoffset.\n";
#JAS						last;
#JAS					}
#JAS				} elsif ($field eq $proto_field) { # EXTERNPROTO handling
#JAS					push @is, @{$this->{IS}{$proto_node}{$field}};
#JAS					next;
#JAS				} else {
#JAS					print "No CNode for $proto_node->{TypeName} ",
#JAS						VRML::NodeIntern::dump_name($proto_node), " from IS hash.\n";
#JAS				}
#JAS			}
#JAS			if (! ($outptr || $offset) &&
#JAS				! $proto_node->{TypeName} =~ /script/i) {
#JAS				print "Failed to find CNode info for PROTO $node->{TypeName} ",
#JAS					VRML::NodeIntern::dump_name($node), " in IS hash.\n";
#JAS				return (0,0,0,0,0);
#JAS			}
#JAS		}
#JAS	}


	#addChildren really is Children
	if (($field eq "addChildren") || ($field eq "removeChildren")) {
		$field = "children";
	}

#JAS	# Protos, when expanded, still have the same script number. We need to make
#JAS	# sure that a script within a proto is uniquely identified by the scene
#JAS	# proto expansion; otherwise routing will cross-pollinate .
#JAS
#JAS	if (defined $node->{ProtoExp}) {
#JAS		#print "this is a protoexp, I am ",VRML::NodeIntern::dump_name($scene), 
#JAS		#		" ProtoExp ",VRML::NodeIntern::dump_name($node->{ProtoExp}),"\n";
#JAS		$node = $node->real_node();
#JAS		#print "ProtoExp node now is $node->{TypeName} ", VRML::NodeIntern::dump_name($node), "\n";
#JAS
#JAS		my $testnode = $node->{Fields}{children}[0];
#JAS		if (ref $testnode eq "VRML::DEF") {$testnode = $testnode->node();}
#JAS
#JAS		#print "my testnode is ", VRML::NodeIntern::dump_name($testnode),"\n";
#JAS		#print "FN $fieldname\n";
#JAS		#foreach (keys%{$testnode->{Fields}}) {print "	node key $_\n";}
#JAS
#JAS		if (defined {$testnode->{Fields}{$fieldname}}) {
#JAS			#print "field exists1! making testnode realnode\n";
#JAS			$node = $testnode;
#JAS		}
#JAS	}

	# ElevationGrid, Extrusion, IndexedFaceSet and IndexedLineSet
	# eventIns (see VRML97 node reference)
	# these things have set_xxx and xxx... if we have one of these...
	#print "node TypeName is ",$node->{TypeName},"\n";
	
	if (($node->{TypeName} eq "Extrusion") ||
	    ($node->{TypeName} eq "ElevationGrid") ||
	    ($node->{TypeName} eq "GeoElevationGrid") ||
	    ($node->{TypeName} eq "IndexedFaceSet") ||
	    ($node->{TypeName} eq "IndexedLineSet")) {
		if ($field =~ /^set_($VRML::Error::Word+)/) {
			$field = $1;
		}
	}
	#print "events, here, isproto $is_proto, typename ",
	#$node->{TypeName},"\n";

	if (!$is_proto && $node->{TypeName} =~ /script/i) {
		$outoffset = VRML::VRMLFunc::paramIndex($field, $node->{Type}{FieldTypes}{$field});
		$outptr =$node->{scriptInvocationNumber};
		#print "scriptInvocationNumber:", $node->{scriptInvocationNumber},"\n";

		if ($direction eq "eventOut") {
			$scrpt = 1;
		} else {
			#### Alendubri: Point where is entering.
			$scrpt = 2;
			$to_count = 1;
			$tonode_str = "$outptr:$outoffset";
			#print "outptr:outoffset->",$outptr,":",$outoffset,"\n";
		}
	} elsif ($proto_node->{TypeName} =~ /script/i) {
		#print "this is a script node. proto_field is $proto_field\n";
		$outoffset = VRML::VRMLFunc::paramIndex($proto_field, $proto_node->{Type}{FieldTypes}{$proto_field});
		$outptr = $proto_node->{scriptInvocationNumber};

		#print "now, this is a script, outptr is $outptr outoffset = $outoffset\n";

		if ($direction eq "eventOut") {
			$scrpt = 1;
		} else {
			$scrpt = 2;
			$to_count = 1;
			$tonode_str = "$outptr:$outoffset";
			}
			# print "PROTO got a script: outptr $outptr, offset $outoffset, scenenum $scenenum\n";
	} else {
		if (!defined $node->{BackNode}{CNode}) {
			# check if this node resides within a Javascript invocation...
			# if so, we have to ensure the equivalence of nodes between 
			# the C structures and the Javascript invocation.
			# check out tests/8.wrl in the FreeWRL source distribution for
			# a script with a DEF'd node in it's parameter declarations.

			# Javascript will "know" to send/get values from this
			# VRML node, and will use perl calls to do so.


			my $brow = $scene->get_browser();
			# print "No backend, but browser has ",$brow->{BE}, " for node ",
			#	VRML::NodeIntern::dump_name($node),"\n";

			# make a backend
			$node->make_backend($brow->{BE},$brow->{BE});
		}

		if (!defined ($outptr=$node->{BackNode}{CNode})) {
			# are there backend CNodes made for both from and to nodes?
			print "add_route, from $field - no backend CNode node\n";
			return (0,0,0,0,0);
		}

		# are there offsets for these eventins and eventouts?
		if (!defined ($outoffset=$VRML::CNodes{$node->{TypeName}}{Offs}{$field})) {

			# this node is a proto interface node, but is not IS'd anywhere. Lets
			# get the browser to give us some memory for it.
			#print "Calling ExtraMemory\n";
			($outptr,$fieldtype,$clen) = ExtraMemory($node,$field);
			#print "Called....\n";
			$outoffset = 0;

			if ($outptr eq 0) {  # browser call failed to alloc more memory. - maybe
					     # the field did not exist? Whatever, return.

				#print "resolve_node_cnode: CNodes entry ",
				#VRML::Debug::toString($VRML::CNodes{$node->{TypeName}}),
				#		", $node->{TypeName} ",
				#			VRML::NodeIntern::dump_name($node),
				#					", event $field offset not defined\n";
				return (0,0,0,0,0);
			}
		}
		if ($direction =~ /eventIn/i) {
			$to_count = 1;
			$tonode_str = "$outptr:$outoffset";
		}
	}

	# ok, we have node and field, lets find either field length, or interpolator
	# if this is a node that had to be allocated by ExtraMemory, we must bypass some of this.
	if ($fieldtype ne "") {   # we have handled this by "ExtraMemory", above.
		if ($direction =~ /eventOut/i) {
			$il = $clen;
		} else {
			$il = 0;
		}
	} else {
		if ($direction =~ /eventIn/i) {
			# is this an interpolator that is handled by C yet?
			if ($is_proto) {
				$il = VRML::VRMLFunc::InterpPointer($proto_node->{Type}{Name});
				$fieldtype = $proto_node->{Type}{FieldTypes}{$proto_field};
			} else {
				$il = VRML::VRMLFunc::InterpPointer($node->{Type}{Name});
				$fieldtype = $node->{Type}{FieldTypes}{$field};
			}
		} else {
			# do we handle this type of data within C yet?
			if ($is_proto) {
				$il = VRML::VRMLFunc::getClen(
						"VRML::Field::$proto_node->{Type}{FieldTypes}{$proto_field}"->
						clength($proto_field));
				$fieldtype = $proto_node->{Type}{FieldTypes}{$proto_field};
			} else {
				$il = VRML::VRMLFunc::getClen("VRML::Field::$node->{Type}{FieldTypes}{$field}"->clength($field));
				$fieldtype = $node->{Type}{FieldTypes}{$field};
			}
			if ($il == 0) {
				print "add_route, dont handle ",
					$node->{Type}{FieldTypes}{$field}, " types in C yet\n";
				return (0,0,0,0,0);
			}
		}
	}
	if ($direction =~ /eventIn/i) {
		return ($to_count, $tonode_str, $scrpt, 1, $il,$fieldtype);
	} else {
		return ($outptr, $outoffset, $scrpt, 1, $il,$fieldtype);
	}
}

# does a node exist in the scenes DEF nodes??? If so, return it, if not,
# try to get it in other ways.
sub getSceneNode {
	my($scene, $fromNode) = @_;
	my $node;
	#print "getSceneNode, scene $scene, node $fromNode\n";

	# Is this a DEF'd node within the scene?
	if (exists $scene->{DEF}{$fromNode}) {
		#print "getSceneNode, $fromNode is part of scene ",
		#VRML::NodeIntern::dump_name($scene),"\n";
		$node = $scene->{DEF}{$fromNode}{Node};
	} else {
		#print "getSceneNode, $fromNode is NOT part of scene ",
		#VRML::NodeIntern::dump_name($scene),"\n";
		
		$tmp = VRML::Handles::return_EAI_name($fromNode);
		#print "tmp now $tmp ref ",ref $tmp,"\n";
		#print "tmp as a name is ",VRML::NodeIntern::dump_name($tmp),"\n";
	        if (ref $tmp eq "VRML::NodeIntern") {
	                $node = $tmp;
	        } else {
	                $node = $scene->getNode($tmp);
	                if (!defined $node) {
	                        warn("DEF node $tmp is not defined");
			}
		}
	}

	if (defined $node->{IsProto}) {
		#print "getSceneNode - this is a PROTO\n";
		$node = $node->{ProtoExp}{Nodes}[0]->real_node(); 
	} else {
		#print "getSceneNode - this is NOT a PROTO\n";
	}
	#print "getSceneNode, returning $node, ",VRML::NodeIntern::dump_name($node),"\n";
	return $node;
}

################################################################################
# add_route
#
# go through, and find:
#	- pointer to datastructures in C
#	- offsets of actual data within these datastructures
#	- length of the data.
#	- function pointer for an Interpolator function, if required.
#
# with the following caveats:
#	- data length of "-1" is special - it signifies that the field is a MF
#	  field. (eg, Orientation/Normal Interpolators) the copy length is
#	  found at "event prop" time.
#
#	- script value of 1 - fromNode is a script node
#	- script value of 2 - toNode is a script node
#	- script value of 3 - fromNode and toNodes are script nodes
#
#  if the parameter $add_rem is NOT 1, this is a delete, not an add.

sub add_route {
	my($this, $scene, $add_rem, $fromNode, $eventOut, $toNode, $eventIn) = @_;

	my $outptr;
	my $outoffset;
	my $to_count = 0;
	my $tonode_str = "";
	my $datalen;
	my $scrpt = 0;
	my $is_count = 0;
	my $is_str = "";
	my $extraparam = 0;
	my $fc = 0; my $tc = 0; #from and to script nodes.

	#print "\nstart of add_route, $add_rem, $scene, $fromNode, $eventOut, $toNode, $eventIn\n";

	# Use the Browser::EAI_LocateNode to get the "real", unPROTO'd
	# nodes.

	# get the FROM node/field
	my $node;
	$node = getSceneNode($scene,$fromNode);
	($fromNode, $eventOut, $direction) = 
		VRML::Browser::EAI_LocateNode($node, $eventOut, "eventOut");

	# FROM NODE step 2.
	my ($outptr, $outoffset, $fc, $ok, $datalen) = $this->resolve_node_cnode($scene, $fromNode, $eventOut, "eventOut");
	if ($ok == 0) {return 1;} # error message already printed


	# TO NODE
	# get the TO node/field
	$node = getSceneNode($scene,$toNode);
	($toNode, $eventIn, $direction) = 
		VRML::Browser::EAI_LocateNode($node, $eventIn, "eventIn");

	my ($to_count, $tonode_str, $tc, $ok, $intptr) = $this->resolve_node_cnode($scene, $toNode, $eventIn, "eventIn");


	if ($eventIn eq "addChildren") {
		$extraparam = 1;
	}
		
	if ($ok == 0) {return 1;} # error message already printed

	$scrpt = $fc + $tc;

	#print "\nVRML::EventMachine::add_route: outptr $outptr, ofst $outoffset, in $to_count '$tonode_str', len $datalen interp $intptr sc $scrpt\n";

	VRML::VRMLFunc::do_CRoutes_Register($add_rem, $outptr, $outoffset,
		$to_count, $tonode_str,
		$datalen, $intptr, $scrpt, $extraparam);
}

sub add_is {
	my ($this, $nodeParent, $is, $node, $field) = @_;

	## VRML97 4.83: multiple IS statements for the fields, eventIns, and
	## eventOuts in the prototype interface declaration is valid
	push @{$this->{IS}{$nodeParent}{$is}}, [ $node, $field ];
}

sub register_listener {
	my($this, $node, $field, $sub) = @_;
	$this->{Listen}{$node}{$field} = $sub;
}

1;
