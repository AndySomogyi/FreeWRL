# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.


# Use the C routines..

use strict;
no strict "refs";

package VRML::CU;

use VRML::VRMLFunc;

VRML::VRMLFunc::load_data(); # Fill hashes.

sub alloc_struct_be {
	my($type) = @_;
	if(!defined $VRML::CNodes{$type}) {
		die("No CNode for $type\n");
	}
	# print "ALLNod: '$type' $VRML::CNodes{$type}{Offs}{_end_} $VRML::CNodes{$type}{Virt}\n";
	my $s = VRML::VRMLFunc::alloc_struct($VRML::CNodes{$type}{Offs}{_end_},
		$VRML::CNodes{$type}{Virt}
		);
	my($k,$o);
	while(($k,$o) = each %{$VRML::CNodes{$type}{Offs}}) {
		next if $k eq '_end_';
		my $ft = $VRML::Nodes{$type}{FieldTypes}{$k};
		# print "ALLS: $type $k $ft $o\n";
		&{"VRML::VRMLFunc::alloc_offs_$ft"}(
			$s, $o
		);
	}
	return $s;
	print "end of alloc_struct_be\n";
}

sub free_struct_be {
	my($node,$type) = @_;
	if(!defined $VRML::CNodes{$type}) {
		die("No CNode for $type\n");
	}
	my($k,$o);
	while(($k,$o) = each %{$VRML::CNodes{$type}{Offs}}) {
		my $ft = $VRML::Nodes{$type}{FieldTypes}{$k};
		&{"VRML::VRMLFunc::free_offs_$ft"}(
			$node, $o
		);
	}
	VRML::VRMLFunc::free_struct($node)
}

sub set_field_be {
	my($node, $type, $field, $value) = @_;
	my $o;
	#print "VRMLCU.pm:set_field_be, node $node, type $type, field $field, value $value\n";
	#if ("ARRAY" eq ref $value) {
	#	print "VRMLCU.pm:set_field_be, value of array is @{$value}\n";
	#}

	if(!defined ($o=$VRML::CNodes{$type}{Offs}{$field})) {
		print "Field $field undefined for $type in C\n" if $VRML::verbose::warn;
		return;
	}

	if((ref $value) eq "HASH") {
		if(!defined $value->{CNode}) {
			print "UNABLE TO RETURN UNCNODED\n";
			return undef;
		}
		$value = $value->{CNode};
	}
	if((ref $value) eq "ARRAY" and (ref $value->[0]) eq "HASH") {
		$value = [map {$_->{CNode}} @{$value}];
	}

	my $ft = $VRML::Nodes{$type}{FieldTypes}{$field};
	my $fk = $VRML::Nodes{$type}{FieldKinds}{$field};
	# if($fk !~ /[fF]ield$/ and !defined $value) {return}
	print "SETS: $node $type $field '$value' (",(
		"ARRAY" eq ref $value ? join ',',@$value : $value ),") $ft $o\n"
		if $VRML::verbose::be;

	&{"VRML::VRMLFunc::set_offs_$ft"}(
		$node, $o,
		$value
	);
}

1;
