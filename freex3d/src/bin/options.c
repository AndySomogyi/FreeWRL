/*

  FreeWRL command line arguments.

*/

/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/


#include <config.h>
#include <system.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "main.h"
#include "options.h"

#if HAVE_GETOPT_H
#include <getopt.h>
#endif


void fv_print_version()
{
    const char *libver, *progver;
    
    libver = libFreeWRL_get_version();
    progver = freewrl_get_version();
    
    printf("Program version: %s\nLibrary version: %s\n", progver, libver);
    printf("\nFreeWRL VRML/X3D browser from CRC Canada (http://www.crc.ca)\n");
    printf("   type \"man freewrl\" to view man pages\n\n");
}

void fv_usage()
{
    printf( "usage: freewrl [options] <VRML or X3D file|URL>\n\n"
	    "  -h|--help               This help.\n"
	    "  -v|--version            Print version.\n"
	    "\nWindow options:\n"
	    "  -c|--fullscreen         Set window fullscreen\n"
	    "  -g|--geometry <WxH>     Set window geometry (W width, H height).\n"
	    "  -b|--big                Set window size to 800x600.\n"
	    "\nGeneral options:\n"
	    "  -e|--eai                Enable EAI.\n"
	    "  -f|--fast               Set global texture size to -256 (fast).\n"
	    "  -W|--linewidth <float>  Set line width.\n"
	    "  -Q|--nocollision        Disable collision management.\n"
	    "\nSnapshot options:\n"
	    "  -p|--gif                Set file format to GIF (default is PNG).\n"
	    "  -n|--snapfile <string>  Set output file name pattern with <string>,\n"
	    "                          (use %%n for iteration number).\n"
	    "  -o|--snaptmp <string>   Set output directory for snap files.\n"
#if defined(DOSNAPSEQUENCE)
	    "\nSnapshot sequence options:\n"
	    "  -l|--seq                Set snapshot sequence mode.\n"
	    "  -m|--seqfile <string>   Set sequence file name pattern.\n"
	    "  -q|--maximg <number>    Set maximum number of files in sequence.\n"
#endif
	    "\nMisc options:\n"
	    "  -V|--eaiverbose         Set EAI subsystem messages.\n"
	    "  -r|--screendist <float> Set screen distance.\n"
	    "  -y|--eyedist <float>    Set eye distance.\n"
	    "  -u|--shutter            Set shutter glasses.\n"
	    "  -t|--stereo <float>     Set stereo parameter (angle factor).\n"
	    "  -A|--anaglyph <string>  Set anaglyph color pair ie: RB for left red, right blue. any of RGBCAM.\n"
	    "  -B|--sidebyside         Set side-by-side stereo.\n"
	    "  -U|--updown			   Set updown stereo.\n"
	    "  -K|--keypress <string>  Set immediate key pressed when ready.\n"
		"  -R|--record             Record to /recording/<scene>.fwplay.\n"
		"  -F|--fixture            Playback from /recording/<scene>.fwplay to /fixture.\n"
		"  -P|--playback           Playback from /recording/<scene>.fwplay to /playback\n"
		"  -N|--nametest <string>  Set name of .fwplay test file\n"
		"  -G|--colorscheme <string>  UI colorscheme by builtin name: {original,angry,\n"
		"       aqua,favicon,midnight,neon:lime,neon:yellow,neon:cyan,neon:pink}\n"
		"  -H|--colors <string>    UI colorscheme by 4 html colors in order: \n"
		"    panel,menuIcon,statusText,messageText ie \"#3D4557,#00FFFF,#00FFFF.#00FFFF\" \n"
		"  -I|--pin TF             Pin statusbar(T/F) menubar(T/F)\n"				
		"  -N|--nametest <string>  Set name of .fwplay test file\n"
	    "\nInternal options:\n"
	    "  -i|--plugin <string>    Called from plugin.\n"
	    "  -j|--fd <number>        Pipe to command the program.\n"
	    "  -k|--instance <number>  Instance of plugin.\n"
	    "  -L|--logfile <filename> Log file where all messages should go.\n"
#ifdef HAVE_LIBCURL
	    "  -C|--curl               Use libcurl instead of wget.\n"
#endif
	    ""
	);
}

const char * fv_validate_string_arg(const char *optarg)
{
    return NULL; /* TODO: implement validate_* functions */
}
    static struct option long_options[] = {

/* { const char *name, int has_arg, int *flag, int val }, */

	{"help", no_argument, 0, 'h'},
	{"version", no_argument, 0, 'v'},

	{"fullscreen", no_argument, 0, 'c'},
	{"pin", required_argument, 0, 'I'},
	{"geometry", required_argument, 0, 'g'},
	{"big", no_argument, 0, 'b'},

	{"eai", no_argument, 0, 'e'},
	{"fast", no_argument, 0, 'f'},
	{"linewidth", required_argument, 0, 'W'},
	{"nocollision", no_argument, 0, 'Q'},

	{"gif", no_argument, 0, 'p'},
	{"snapfile", required_argument, 0, 'n'},
	{"snaptmp", required_argument, 0, 'o'},

#if defined(DOSNAPSEQUENCE)
	{"seq", no_argument, 0, 'l'},
	{"seqfile", required_argument, 0, 'm'},
	{"maximg", required_argument, 0, 'q'},
#endif

	{"eaiverbose", no_argument, 0, 'V'},
	{"screendist", required_argument, 0, 'r'},
	{"eyedist", required_argument, 0, 'y'},
	{"shutter", no_argument, 0, 'u'},
	{"stereo", required_argument, 0, 't'},
	{"anaglyph", required_argument, 0, 'A'},
	{"sidebyside", no_argument, 0, 'B'},
	{"updown", no_argument, 0, 'U'},
	{"keypress", required_argument, 0, 'K'},
	{"plugin", required_argument, 0, 'i'},
	{"fd", required_argument, 0, 'j'},
	{"instance", required_argument, 0, 'k'},
	{"logfile", required_argument, 0, 'L'},

	{"curl", no_argument, 0, 'C'},

	{"display", required_argument, 0, 'd'}, /* Roberto Gerson */
	{"record", no_argument, 0, 'R'},
	{"fixture", no_argument, 0, 'F'},
	{"playback", no_argument, 0, 'P'},
	{"nametest", required_argument, 0, 'N'},
	{"colorscheme", required_argument, 0, 'G'},
	{"colors", required_argument, 0, 'H'},
	{0, 0, 0, 0}
    };

int fv_find_opt_for_optopt(char c) {
	int i;
	struct option *p;

	/* initialization */
	i = 0;
	p = &(long_options[i]);

	while (p->name) {
	    if (!p->flag) {
		if (p->val == c) {
		    return i;
		}
	    }
	    p = &(long_options[++i]);
	}
	return -1;
}

//freewrl_params_t *fv_params = NULL;

int fv_parseCommandLine (int argc, char **argv, freewrl_params_t *fv_params)
{
    int c;
    float ftmp;
    long int ldtmp;
    int option_index = 0;
    int real_option_index;
    const char *real_option_name;
    //char *logFileName = NULL;
    //FILE *fp;

#if defined(DOSNAPSEQUENCE)
	static const char optstring[] = "efg:hi:j:k:vVlpq:m:n:o:bsQW:K:Xcr:y:utCL:d:RFPN:";
#else
	static const char optstring[] = "efg:hi:j:k:vVpn:o:bsQW:K:Xcr:y:utCL:d:RFPN:"; //':' means the preceding option requires an arguement
#endif



    while (1) {

	/* Do we want getopt to print errors by itself ? */
	opterr = 0;

# if HAVE_GETOPT_LONG

#if defined(_MSC_VER)
#define strncasecmp _strnicmp
	for(c=0;c<argc;c++)
	{
		printf("argv[%d]=%s\n",c,argv[c]);
	}
	c =	_getopt_internal (argc, argv, optstring, long_options, &option_index, 0);
#else
	c = getopt_long(argc, argv, optstring, long_options, &option_index);
#endif

# else
	c = getopt(argc, argv, optstring);
# endif

	if (c == -1)
	    break;

	if ((c == '?')) {
	    real_option_index = fv_find_opt_for_optopt(optopt);
	} else {
	    real_option_index = fv_find_opt_for_optopt(c);
	}
	if (real_option_index < 0) {
	    real_option_name = argv[optind-1];
	} else {
	    real_option_name = long_options[real_option_index].name;
	}
	DEBUG_ARGS("option_index=%d optopt=%c option=%s\n", real_option_index, c,
		  real_option_name);

	switch (c) {

	    /* Error handling */

	case '?': /* getopt error: unknown option or missing argument */
	    ERROR_MSG("ERROR: unknown option or missing argument to option: %c (%s)\n", 
		     c, real_option_name);
	    fwExit(1);
	    break;

	    /* Options handling */

	case 'h': /* --help, no argument */
	    fv_usage();
	    fwExit(0);
	    break;

	case 'v': /* --version, no argument */
	    fv_print_version();
	    fwExit(0);
	    break;

/* Window options */

	case 'c': /* --fullscreen, no argument */

#if !defined(TARGET_AQUA) 
#ifdef _MSC_VER
		fv_params->fullscreen = TRUE; //win32 will look at this in its internal code
#else
#if defined(HAVE_XF86_VMODE)
	    fv_params->fullscreen = TRUE;
#else
	    printf("\nFullscreen mode is only available when xf86vmode extension is\n"
		  "supported by your X11 server: i.e. XFree86 version 4 or later,\n"
		   "Xorg version 1.0 or later.\n"
		   "Configure should autodetect it for you. If not please report"
		   "this problem to\n\t " PACKAGE_BUGREPORT "\n");
	    fv_params->fullscreen = FALSE;
#endif /* HAVE_XF86_VMODE */
#endif
#endif /* TARGET_AQUA */


	    break;

	case 'g': /* --geometry, required argument: string (ex: 1024x768+100+50) */
	    if (!optarg) {
		ERROR_MSG("Argument missing for option -g/--geometry\n");
		fwExit(1);
	    } else {
		    if (!fwl_parse_geometry_string(optarg, 
						   &fv_params->width, &fv_params->height,
						   &fv_params->xpos, &fv_params->ypos)) {
			    ERROR_MSG("Malformed geometry string: %s\n", optarg);
			    return FALSE;
		    }
	    }
	    break;

	case 'b': /* --big, no argument */
		fv_params->width = 800;
		fv_params->height = 600;
		break;

	case 'd': /* --display, required argument int */
		printf ("Parameter --display = %s\n", optarg);
		sscanf(optarg,"%ld", (long int *)&ldtmp);
		fv_params->winToEmbedInto = ldtmp;
		break;



/* General options */

	case 'e': /* --eai, no argument */
	    fv_params->enableEAI = TRUE;
	    break;

	case 'f': /* --fast, no argument */
		/* does nothing right now */
	    break;

	case 'W': /* --linewidth, required argument: float */
	    sscanf(optarg,"%g", &ftmp);
	    fwl_set_LineWidth(ftmp);
	    break;

	case 'Q': /* --nocollision, no argument */
	    //fv_params->collision = FALSE; //this is the default
	    ConsoleMessage ("ignoring collision off mode on command line");
	    break;

/* Snapshot options */

	case 'p': /* --gif, no argument */
	    fwl_init_SnapGif();
	    break;

	case 'n': /* --snapfile, required argument: string */
	    fwl_set_SnapFile(optarg);
	    break;

	case 'o': /* --snaptmp, required argument: string */
	    fwl_set_SnapTmp(optarg);
	    break;

/* Snapshot sequence options */

#if defined(DOSNAPSEQUENCE)
	case 'l': /* --seq, no argument */
	    fwl_init_SnapSeq();
	    break;

	case 'm': /* --seqfile, required argument: string */
	    fwl_set_SeqFile(optarg);
	    break;

	case 'q': /* --maximg, required argument: number */
	    sscanf(optarg,"%d",&maxSnapImages);
	    fwl_set_MaxImages(maxSnapImages);
	    break;
#endif

/* Misc options */

	case 'V': /* --eaiverbose, no argument */
	    fwl_init_EaiVerbose();
	    fv_params->verbose = TRUE;
	    break;

	case 'r': /* --screendist, required argument: float */
	    fwl_set_ScreenDist(optarg);
	    break;

	case 'y': /* --eyedist, required argument: float */
	    fwl_set_EyeDist(optarg);
	    break;

	case 'u': /* --shutter, no argument */
	    fwl_init_Shutter();
	    /*setXEventStereo();*/
	    break;

	case 't': /* --stereo, required argument: float */
	    fwl_set_StereoParameter(optarg);
	    break;
	case 'A': /* --anaglyph, required argument: string */
	    fwl_set_AnaglyphParameter(optarg);
	    break;

	case 'B': /* --sidebyside, no argument */
	    fwl_init_SideBySide();
	    break;
	case 'U': /* --updown, no argument */
	    fwl_init_UpDown();
	    break;

	case 'K': /* --keypress, required argument: string */
	    /* initial string of keypresses once main url is loaded */
		fwl_set_KeyString(optarg);
	    break;

	case 'G': /* --colorscheme string */
		fwl_set_ui_colorscheme(optarg);
		break;
	case 'H': /* --colors string */
		fwl_set_ui_colors(optarg);
		break;

	case 'I': /* --pin TF */
		fwl_set_sbh_pin_option(optarg);
		break;

/* Internal options */

	case 'i': /* --plugin, required argument: number */
	    sscanf(optarg,"pipe:%d",&_fw_pipe);
	    isBrowserPlugin = TRUE;
	    break;

	case 'j': /* --fd, required argument: number */
	    sscanf(optarg,"%d",&_fw_browser_plugin);
	    break;

	case 'k': /* --instance, required argument: number */
	    sscanf(optarg,"%u",(unsigned int *)(void *)(&_fw_instance));
	    break;

	case 'L': /* --logfile, required argument: log filename */
	    if (optarg) {
		//logFileName = strdup(optarg);
			 fwl_set_logfile(optarg);
	    } else {
		ERROR_MSG("Option -L|--logfile: log filename required\n");
		return FALSE;
	    }
	    break;

	case 'R': /* --record, no arg */
		fwl_set_modeRecord();
		break;
	case 'F': /* --fixture, no arg */
		fwl_set_modeFixture();
		break;
	case 'P': /* --playback, no arg */
		fwl_set_modePlayback();
		break;
	case 'N': /* --nametest, required arguement: "name_of_fwplay"*/
		fwl_set_nameTest(optarg);
		break;


#ifdef HAVE_LIBCURL
	case 'C': /* --curl, no argument */
	    with_libcurl = TRUE;
	    break;
#endif

	default:
	    ERROR_MSG("ERROR: getopt returned character code 0%o, unknown error.\n", c);
	    fwExit(1);
	    break;
	}
    }

 //   moved to fwl_set_logfile(char*)/* Quick hack: redirect stdout and stderr to logFileName if supplied */
 //   if (logFileName) {
	//if (strncasecmp(logFileName, "-", 1) == 0) {
	//    printf("FreeWRL: output to stdout/stderr\n");
	//} else {
	//    printf ("FreeWRL: redirect stdout and stderr to %s\n", logFileName);	
	//    fp = freopen(logFileName, "a", stdout);
	//    if (NULL == fp) {
	//	WARN_MSG("WARNING: Unable to reopen stdout to %s\n", logFileName) ;
	//    }
	//    fp = freopen(logFileName, "a", stderr);
	//    if (NULL == fp) {
	//	WARN_MSG("WARNING: Unable to reopen stderr to %s\n", logFileName) ;
	//    }
	//}
 //   }

    if (optind < argc) {
	if (optind != (argc-1)) {
		ERROR_MSG("FreeWRL accepts only one argument: we have %d\n", (argc-optind));
		return FALSE;
	}
	DEBUG_MSG("Start url: %s\n", argv[optind]);
	start_url = STRDUP(argv[optind]);
    }

    return TRUE;
}

void fv_parseEnvVars()
{
	/* Check environment */
	fwl_set_strictParsing		(getenv("FREEWRL_STRICT_PARSING") != NULL);
	fwl_set_plugin_print		(getenv("FREEWRL_DO_PLUGIN_PRINT") != NULL);
	fwl_set_occlusion_disable	(getenv("FREEWRL_NO_GL_ARB_OCCLUSION_QUERY") != NULL);
	fwl_set_print_opengl_errors	(getenv("FREEWRL_PRINT_OPENGL_ERRORS") != NULL);
	fwl_set_trace_threads		(getenv("FREEWRL_TRACE_THREADS") != NULL);
	{
		char *env_texture_size = getenv("FREEWRL_TEXTURE_SIZE");
		if (env_texture_size) {
			unsigned int local_texture_size ;
			sscanf(env_texture_size, "%u", &local_texture_size);
			TRACE_MSG("Env: TEXTURE SIZE %u.\n", local_texture_size);
			fwl_set_texture_size(local_texture_size);
		}
	}
}
