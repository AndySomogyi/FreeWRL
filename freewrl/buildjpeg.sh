#!/bin/sh

# Berkeley Open Infrastructure for Network Computing
# http://boinc.berkeley.edu
# Copyright (C) 2005 University of California
#
# This is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation;
# either version 2.1 of the License, or (at your option) any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU Lesser General Public License for more details.
#
# To view the GNU Lesser General Public License visit
# http://www.gnu.org/copyleft/lesser.html
# or write to the Free Software Foundation, Inc.,
# 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#
# Script to build the wxMac-2.6.2 library for BOINC as a Universal Binary
#
# Script to build Macintosh Universal Binary library of jpeg-6b for
# use in building BOINC.
#
# by Charlie Fenton 2/17/06
#
## In Terminal, CD to the wxMac-2.6.2 directory.
##     cd [path]/jpeg-6b/
## then run this script:
##    source [ path_to_this_script ] [ -clean ]
#
# the -clean argument will force a full rebuild.
#


export PATH=/usr/local/bin:$PATH
export CC=/usr/bin/gcc-3.3;export CXX=/usr/bin/g++-3.3
export LDFLAGS="-arch ppc"
export CPPFLAGS="-arch ppc"
export CFLAGS="-arch ppc"

./configure --disable-shared --host=ppc

export LDFLAGS="-isysroot /Developer/SDKs/MacOSX10.3.9.sdk -Wl,-syslibroot,/Developer/SDKs/MacOSX10.3.9.sdk"
export CPPFLAGS="-I/Developer/SDKs/MacOSX10.3.9.sdk/Developer/Headers/FlatCarbon -I/Developer/SDKs/MacOSX10.3.9.sdk/usr/include -isystem /Developer/SDKs/MacOSX10.3.9.sdk/usr/include/gcc/darwin/3.3 -I/Developer/SDKs/MacOSX10.3.9.sdk/usr/include/gcc/darwin/3.3/c++ -I/Developer/SDKs/MacOSX10.3.9.sdk/usr/include/gcc/darwin/3.3/c++/ppc-darwin -isystem /Developer/SDKs/MacOSX10.3.9.sdk/usr/include"

rm -f libjpeg_ppc.a
rm -f libjpeg_i386.a
rm -f libjpeg.a
make clean

make
mv -f libjpeg.a libjpeg_ppc.a

make clean

export PATH=/usr/local/bin:$PATH
export CC=/usr/bin/gcc-4.0;export CXX=/usr/bin/g++-4.0
export LDFLAGS=""
export CPPFLAGS=""
export CFLAGS=""
./configure --disable-shared --host=i386

export LDFLAGS="-isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch i386"
export CPPFLAGS="-isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch i386"

make -e
if [  $? -ne 0 ]; then exit 1; fi
mv libjpeg.a libjpeg_i386.a
lipo -create libjpeg_i386.a libjpeg_ppc.a -output libjpeg.a

export CC="";export CXX=""
export LDFLAGS=""
export CPPFLAGS=""
