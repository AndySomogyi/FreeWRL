# -*- Mode: makefile -*-
#
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is Mozilla Communicator client code, released
# March 31, 1998.
#
# The Initial Developer of the Original Code is
# Netscape Communications Corporation.
# Portions created by the Initial Developer are Copyright (C) 1998
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Steve Zellers (zellers@apple.com)
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

#
# Config for Mac OS X as of PR3
# Just ripped from Linux config
#

CC = gcc-4.0 
CCC = g++
CFLAGS +=  -Wno-format -MMD -arch ppc -isysroot /Developer/SDKs/MacOSX10.4u.sdk -no_compact_linkedit
OS_CFLAGS = -DXP_UNIX -DSVR4 -DSYSV -D_BSD_SOURCE -DPOSIX_SOURCE -DDARWIN -DHAVE_VA_COPY -DVA_COPY=va_copy
LDFLAGS += -arch ppc 

RANLIB = ranlib
MKSHLIB = $(CC) -dynamiclib $(XMKSHLIBOPTS) -framework System  -arch ppc

SO_SUFFIX = dylib

#.c.o:
#      $(CC) -c -MD $*.d $(CFLAGS) $<

CPU_ARCH = $(shell uname -m)
ifeq (86,$(findstring 86,$(CPU_ARCH)))
CPU_ARCH = x86
OS_CFLAGS+= -DX86_LINUX
endif
GFX_ARCH = x

OS_LIBS = -lc -framework System

ASFLAGS += -x assembler-with-cpp

ifeq ($(CPU_ARCH),alpha)

# Ask the C compiler on alpha linux to let us work with denormalized
# double values, which are required by the ECMA spec.

OS_CFLAGS += -mieee
endif

# Use the editline library to provide line-editing support.
JS_EDITLINE = 1

# Don't allow Makefile.ref to use libmath
NO_LIBM = 1

