/* specific for iPhone build */

#define FRONTEND_HANDLES_DISPLAY_THREAD 1
#define HAVE_MEMCPY 1
#define HAVE_CTYPE_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H 1
#define HAVE_MATH_H 1
#define HAVE_STDARG_H 1
#define HAVE_ERRNO_H 1
#define HAVE_GETTIMEOFDAY 1
#define XP_UNIX 1
#define X_DISPLAY_MISSING 1
#define TARGET_AQUA 1
#define _REENTRANT 1
#define SHADERS_2011 1
#define IPHONE -DAQUA 1


#ifndef AQUA
#define AQUA
#endif
#define INSTALLDIR "/usr/share/freewrl"
#define FONTS_DIR "/usr/share/freewrl/fonts"
#define HAVE_STDINT_H 1
#define HAVE_STDBOOL_H 1
#define HAVE_UNISTD_H 1
#define HAVE_SYS_WAIT_H 1
#define HAVE_PTHREAD 1
#define HAVE_CONFIG_H 1
#define BUILDDIR "/FreeWRL/freewrl/freex3d/appleOSX"
#define FWVER "1.22.10"
#define FREEX3D_MESSAGE_WRAPPER "/usr/bin/say"
#define FREEWRL_MESSAGE_WRAPPER "/usr/bin/say"
#define REWIRE_SERVER "/usr/bin/freewrlReWireServer"
#define SOUNDSERVERBINARY "/usr/bin/FreeWRL_SoundServer"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <fcntl.h>
#ifndef IPHONE
#include <expat.h>
#endif
#include <internal.h>
#define HAVE_STDINT_H 1
#define HAVE_STDBOOL_H 1
#define HAVE_UNISTD_H 1
#define HAVE_SYS_WAIT_H 1
#define HAVE_PTHREAD 1
#define WGET "/usr/bin/curl"
#define BROWSER "/usr/bin/open"