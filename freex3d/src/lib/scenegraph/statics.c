/*
=INSERT_TEMPLATE_HERE=

$Id$

large constant strings; used for rendering.

*/

#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"


/* BOX */

/* faces are quads, 4 vertices; order: front, back, top, down, right, left. */

GLfloat boxnorms[] ={0,0,1, 0,0,1, 0,0,1, 0,0,1,
			0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1,
			0,1,0, 0,1,0, 0,1,0, 0,1,0,
			0,-1,0, 0,-1,0, 0,-1,0, 0,-1,0,
			1,0,0, 1,0,0, 1,0,0, 1,0,0,
			-1,0,0, -1,0,0, -1,0,0, -1,0,0};

/* box texture coordinates */
GLfloat boxtex[] ={ 1,1, 0,1, 0,0, 1,0,
		    0,0, 1,0, 1,1, 0,1,
		    0,0, 1,0, 1,1, 0,1,
		    0,0, 1,0, 1,1, 0,1,
		    0,0, 1,0, 1,1, 0,1,
		    1,0, 1,1, 0,1, 0,0};

/* Background and TextureBackground */
/* faces are quads, 4 vertices; order: front, back, top, down, right, left. */
GLfloat Backnorms[] ={0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1,
			0,0,1, 0,0,1, 0,0,1, 0,0,1,
			0,1,0, 0,1,0, 0,1,0, 0,1,0,
			0,-1,0, 0,-1,0, 0,-1,0, 0,-1,0,
			1,0,0, 1,0,0, 1,0,0, 1,0,0,
			-1,0,0, -1,0,0, -1,0,0, -1,0,0};

GLfloat Backtex[]= {0.99,0.99, 0.01,0.99, 0.01,0.01, 0.99,0.01,
		    0.99,0.01, 0.99,0.99, 0.01,0.99, 0.01,0.01,
		    0.99,0.99, 0.01,0.99, 0.01,0.01, 0.99,0.01,
		    0.99,0.99, 0.01,0.99, 0.01,0.01, 0.99,0.01,
		    0.99,0.99, 0.01,0.99, 0.01,0.01, 0.99,0.01,
		    0.99,0.99, 0.01,0.99, 0.01,0.01, 0.99,0.01};


GLfloat BackgroundVert[] =  {
		/* front */
		0.15,0.15,-0.15, -0.15,0.15,-0.15, -0.15,-0.15,-0.15, 0.15,-0.15,-0.15,
		/* back */
		-0.15, -0.15, 0.15, -0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, -0.15, 0.15,
		/* top */
		0.15,0.15,0.15, -0.15,0.15,0.15, -0.15,0.15,-0.15, 0.15,0.15,-0.15,
		/* bottom */
		0.15,-0.15,-0.15, -0.15,-0.15,-0.15, -0.15,-0.15,0.15, 0.15,-0.15,0.15,
		/* right */
		0.15,0.15,0.15, 0.15,0.15,-0.15, 0.15,-0.15,-0.15, 0.15,-0.15,0.15,
		/* left */
		-0.15, 0.15, -0.15, -0.15, 0.15, 0.15, -0.15, -0.15, 0.15, -0.15, -0.15, -0.15
	};



/*  CYLINDER*/

/*  simple generation; normals point outwards from each vertex.*/
/* for (i=0; i<=20; i++) {*/
/* 	a1 = PI * 2 * (i-0.5) / 20.0;*/
/* 	printf ("%4.3f,0.0,%4.3f, ",sin(a1),  cos(a1));*/
/* 	printf ("%4.3f,0.0,%4.3f,\n",sin(a1), cos(a1));*/
/* }*/
GLfloat cylnorms[] = { -0.156,0.0,0.988, -0.156,0.0,0.988,
	0.156,0.0,0.988, 0.156,0.0,0.988, 0.454,0.0,0.891, 0.454,0.0,0.891,
	0.707,0.0,0.707, 0.707,0.0,0.707, 0.891,0.0,0.454, 0.891,0.0,0.454,
	0.988,0.0,0.156, 0.988,0.0,0.156, 0.988,0.0,-0.156, 0.988,0.0,-0.156,
	0.891,0.0,-0.454, 0.891,0.0,-0.454, 0.707,0.0,-0.707, 0.707,0.0,-0.707,
	0.454,0.0,-0.891, 0.454,0.0,-0.891, 0.156,0.0,-0.988, 0.156,0.0,-0.988,
	-0.156,0.0,-0.988, -0.156,0.0,-0.988, -0.454,0.0,-0.891, -0.454,0.0,-0.891,
	-0.707,0.0,-0.707, -0.707,0.0,-0.707, -0.891,0.0,-0.454, -0.891,0.0,-0.454,
	-0.988,0.0,-0.156, -0.988,0.0,-0.156, -0.988,0.0,0.156, -0.988,0.0,0.156,
	-0.891,0.0,0.454, -0.891,0.0,0.454, -0.707,0.0,0.707, -0.707,0.0,0.707,
	-0.454,0.0,0.891, -0.454,0.0,0.891, -0.156,0.0,0.988, -0.156,0.0,0.988};

/*  top index into the __points generated array*/
unsigned char cyltopindx[] = {
	42,0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40};

/*  bottom index into the __points generated array*/
unsigned char cylbotindx[] = {43,41,39,37,35,33,31,29,27,25,23,21,
	19,17,15,13,11,9,7,5,3,1};

/*  side textures; simply 20 slices of the texture; 2 coords per slice.*/
/*  for (i=0; i<=20; i++) {*/
/* 	printf ("%4.3f,1.0, %4.3f,0.0,\n",(float)(i+10.0)/20.0, (float)(i+10.0)/20.0);*/
/*  }*/
GLfloat cylsidetex[] = { 0.500,1.0, 0.500,0.0, 0.550,1.0, 0.550,0.0, 0.600,1.0, 0.600,0.0,
	0.650,1.0, 0.650,0.0, 0.700,1.0, 0.700,0.0, 0.750,1.0, 0.750,0.0, 0.800,1.0, 0.800,0.0,
	0.850,1.0, 0.850,0.0, 0.900,1.0, 0.900,0.0, 0.950,1.0, 0.950,0.0, 1.000,1.0, 1.000,0.0,
	1.050,1.0, 1.050,0.0, 1.100,1.0, 1.100,0.0, 1.150,1.0, 1.150,0.0, 1.200,1.0, 1.200,0.0,
	1.250,1.0, 1.250,0.0, 1.300,1.0, 1.300,0.0, 1.350,1.0, 1.350,0.0, 1.400,1.0, 1.400,0.0,
	1.450,1.0, 1.450,0.0, 1.500,1.0, 1.500,0.0, };


/*  end textures; interleaved top and bottom; cyltopindx and cylbotindx are the*/
/*  indicies. Generated with:*/
/*  for (i=0; i<20; i++) {*/
/*          a1 = PI * 2 * (i) / 20.0;*/
/*          a2 = PI * 2 * (i+1) / 20.0;*/
/*          printf ("%4.3f, %4.3f, ",0.5+0.5*sin(a1),  0.5+0.5*cos(a1+PI));*/
/*          printf ("%4.3f, %4.3f, ",0.5+0.5*sin(a1),  0.5+0.5*cos(a1));*/
/*          printf ("\n");*/
/*  } printf ("0.5,0.5,0.5,0.5\n");*/
GLfloat cylendtex[] = {
		0.500, 0.000, 0.500, 1.000,
		0.655, 0.024, 0.655, 0.976,
		0.794, 0.095, 0.794, 0.905,
		0.905, 0.206, 0.905, 0.794,
		0.976, 0.345, 0.976, 0.655,
		1.000, 0.500, 1.000, 0.500,
		0.976, 0.655, 0.976, 0.345,
		0.905, 0.794, 0.905, 0.206,
		0.794, 0.905, 0.794, 0.095,
		0.655, 0.976, 0.655, 0.024,
		0.500, 1.000, 0.500, 0.000,
		0.345, 0.976, 0.345, 0.024,
		0.206, 0.905, 0.206, 0.095,
		0.095, 0.794, 0.095, 0.206,
		0.024, 0.655, 0.024, 0.345,
		0.000, 0.500, 0.000, 0.500,
		0.024, 0.345, 0.024, 0.655,
		0.095, 0.206, 0.095, 0.794,
		0.206, 0.095, 0.206, 0.905,
		0.345, 0.024, 0.345, 0.976,
		0.500, 0.000, 0.500, 1.000,
		0.5,0.5,0.5,0.5
};


/*  CONE*/
/* indexes for arrays for bottom of cone */
unsigned char tribotindx[] = {21,20,19,18,17,16,15,14,13,12,11,
			10,9,8,7,6,5,4,3,2,1,22};

/* texture mapping indexes for bottom of cone */
/* generated with:
	printf ("0.0, 0.0");
	for (i=1; i<=20; i++) {
	        a1 = PI * 2 * (i) / 20.0;
	        printf ("%4.3f, %4.3f, ",0.5+0.5*sin(a1),  0.5+0.5*cos(a1));
	}
	printf ("0.5,0.5,0.5,1.0\n");
*/
float tribottex[] = { 0,0, 0.655, 0.976, 0.794, 0.905, 0.905, 0.794,
		0.976, 0.655, 1.000, 0.500, 0.976, 0.345, 0.905, 0.206,
		0.794, 0.095, 0.655, 0.024, 0.500, 0.000, 0.345, 0.024,
		0.206, 0.095, 0.095, 0.206, 0.024, 0.345, 0.000, 0.500,
		0.024, 0.655, 0.095, 0.794, 0.206, 0.905, 0.345, 0.976,
		0.500, 1.000, 0.5,0.5,0.5,1.0 };

float trisidtex[] = {
			0.575,1.0, 0.55,0.0, 0.60,0.0, /* 12*/
			0.625,1.0, 0.60,0.0, 0.65,0.0, /* 13*/
			0.675,1.0, 0.65,0.0, 0.70,0.0, /* 14*/
			0.725,1.0, 0.70,0.0, 0.75,0.0, /* 15*/
			0.775,1.0, 0.75,0.0, 0.80,0.0, /* 16*/
			0.825,1.0, 0.80,0.0, 0.85,0.0, /* 17*/
			0.875,1.0, 0.85,0.0, 0.90,0.0, /* 18*/
			0.925,1.0, 0.90,0.0, 0.95,0.0, /* 19*/
			0.975,1.0, 0.95,0.0, 1.0,0.0,   /* 20*/
			0.025,1.0, 0.00,0.0, 0.05,0.0, /* 1*/
			0.075,1.0, 0.05,0.0, 0.10,0.0, /* 2*/
			0.125,1.0, 0.10,0.0, 0.15,0.0, /* 3*/
			0.175,1.0, 0.15,0.0, 0.20,0.0, /* 4*/
			0.225,1.0, 0.20,0.0, 0.25,0.0, /* 5*/
			0.275,1.0, 0.25,0.0, 0.30,0.0, /* 6*/
			0.325,1.0, 0.30,0.0, 0.35,0.0, /* 7*/
			0.375,1.0, 0.35,0.0, 0.40,0.0, /* 8*/
			0.425,1.0, 0.40,0.0, 0.45,0.0, /* 9*/
			0.475,1.0, 0.45,0.0, 0.50,0.0, /* 10*/
			0.525,1.0, 0.50,0.0, 0.55,0.0 /* 11*/
		};


/*  SPHERE*/

float spheretex[] = {
		0.000,0.100, 0.000,0.000,
		0.050,0.100, 0.050,0.000,
		0.100,0.100, 0.100,0.000,
		0.150,0.100, 0.150,0.000,
		0.200,0.100, 0.200,0.000,
		0.250,0.100, 0.250,0.000,
		0.300,0.100, 0.300,0.000,
		0.350,0.100, 0.350,0.000,
		0.400,0.100, 0.400,0.000,
		0.450,0.100, 0.450,0.000,
		0.500,0.100, 0.500,0.000,
		0.550,0.100, 0.550,0.000,
		0.600,0.100, 0.600,0.000,
		0.650,0.100, 0.650,0.000,
		0.700,0.100, 0.700,0.000,
		0.750,0.100, 0.750,0.000,
		0.800,0.100, 0.800,0.000,
		0.850,0.100, 0.850,0.000,
		0.900,0.100, 0.900,0.000,
		0.950,0.100, 0.950,0.000,
		1.000,0.100, 1.000,0.000,
		0.000,0.200, 0.000,0.100,
		0.050,0.200, 0.050,0.100,
		0.100,0.200, 0.100,0.100,
		0.150,0.200, 0.150,0.100,
		0.200,0.200, 0.200,0.100,
		0.250,0.200, 0.250,0.100,
		0.300,0.200, 0.300,0.100,
		0.350,0.200, 0.350,0.100,
		0.400,0.200, 0.400,0.100,
		0.450,0.200, 0.450,0.100,
		0.500,0.200, 0.500,0.100,
		0.550,0.200, 0.550,0.100,
		0.600,0.200, 0.600,0.100,
		0.650,0.200, 0.650,0.100,
		0.700,0.200, 0.700,0.100,
		0.750,0.200, 0.750,0.100,
		0.800,0.200, 0.800,0.100,
		0.850,0.200, 0.850,0.100,
		0.900,0.200, 0.900,0.100,
		0.950,0.200, 0.950,0.100,
		1.000,0.200, 1.000,0.100,
		0.000,0.300, 0.000,0.200,
		0.050,0.300, 0.050,0.200,
		0.100,0.300, 0.100,0.200,
		0.150,0.300, 0.150,0.200,
		0.200,0.300, 0.200,0.200,
		0.250,0.300, 0.250,0.200,
		0.300,0.300, 0.300,0.200,
		0.350,0.300, 0.350,0.200,
		0.400,0.300, 0.400,0.200,
		0.450,0.300, 0.450,0.200,
		0.500,0.300, 0.500,0.200,
		0.550,0.300, 0.550,0.200,
		0.600,0.300, 0.600,0.200,
		0.650,0.300, 0.650,0.200,
		0.700,0.300, 0.700,0.200,
		0.750,0.300, 0.750,0.200,
		0.800,0.300, 0.800,0.200,
		0.850,0.300, 0.850,0.200,
		0.900,0.300, 0.900,0.200,
		0.950,0.300, 0.950,0.200,
		1.000,0.300, 1.000,0.200,
		0.000,0.400, 0.000,0.300,
		0.050,0.400, 0.050,0.300,
		0.100,0.400, 0.100,0.300,
		0.150,0.400, 0.150,0.300,
		0.200,0.400, 0.200,0.300,
		0.250,0.400, 0.250,0.300,
		0.300,0.400, 0.300,0.300,
		0.350,0.400, 0.350,0.300,
		0.400,0.400, 0.400,0.300,
		0.450,0.400, 0.450,0.300,
		0.500,0.400, 0.500,0.300,
		0.550,0.400, 0.550,0.300,
		0.600,0.400, 0.600,0.300,
		0.650,0.400, 0.650,0.300,
		0.700,0.400, 0.700,0.300,
		0.750,0.400, 0.750,0.300,
		0.800,0.400, 0.800,0.300,
		0.850,0.400, 0.850,0.300,
		0.900,0.400, 0.900,0.300,
		0.950,0.400, 0.950,0.300,
		1.000,0.400, 1.000,0.300,
		0.000,0.500, 0.000,0.400,
		0.050,0.500, 0.050,0.400,
		0.100,0.500, 0.100,0.400,
		0.150,0.500, 0.150,0.400,
		0.200,0.500, 0.200,0.400,
		0.250,0.500, 0.250,0.400,
		0.300,0.500, 0.300,0.400,
		0.350,0.500, 0.350,0.400,
		0.400,0.500, 0.400,0.400,
		0.450,0.500, 0.450,0.400,
		0.500,0.500, 0.500,0.400,
		0.550,0.500, 0.550,0.400,
		0.600,0.500, 0.600,0.400,
		0.650,0.500, 0.650,0.400,
		0.700,0.500, 0.700,0.400,
		0.750,0.500, 0.750,0.400,
		0.800,0.500, 0.800,0.400,
		0.850,0.500, 0.850,0.400,
		0.900,0.500, 0.900,0.400,
		0.950,0.500, 0.950,0.400,
		1.000,0.500, 1.000,0.400,
		0.000,0.600, 0.000,0.500,
		0.050,0.600, 0.050,0.500,
		0.100,0.600, 0.100,0.500,
		0.150,0.600, 0.150,0.500,
		0.200,0.600, 0.200,0.500,
		0.250,0.600, 0.250,0.500,
		0.300,0.600, 0.300,0.500,
		0.350,0.600, 0.350,0.500,
		0.400,0.600, 0.400,0.500,
		0.450,0.600, 0.450,0.500,
		0.500,0.600, 0.500,0.500,
		0.550,0.600, 0.550,0.500,
		0.600,0.600, 0.600,0.500,
		0.650,0.600, 0.650,0.500,
		0.700,0.600, 0.700,0.500,
		0.750,0.600, 0.750,0.500,
		0.800,0.600, 0.800,0.500,
		0.850,0.600, 0.850,0.500,
		0.900,0.600, 0.900,0.500,
		0.950,0.600, 0.950,0.500,
		1.000,0.600, 1.000,0.500,
		0.000,0.700, 0.000,0.600,
		0.050,0.700, 0.050,0.600,
		0.100,0.700, 0.100,0.600,
		0.150,0.700, 0.150,0.600,
		0.200,0.700, 0.200,0.600,
		0.250,0.700, 0.250,0.600,
		0.300,0.700, 0.300,0.600,
		0.350,0.700, 0.350,0.600,
		0.400,0.700, 0.400,0.600,
		0.450,0.700, 0.450,0.600,
		0.500,0.700, 0.500,0.600,
		0.550,0.700, 0.550,0.600,
		0.600,0.700, 0.600,0.600,
		0.650,0.700, 0.650,0.600,
		0.700,0.700, 0.700,0.600,
		0.750,0.700, 0.750,0.600,
		0.800,0.700, 0.800,0.600,
		0.850,0.700, 0.850,0.600,
		0.900,0.700, 0.900,0.600,
		0.950,0.700, 0.950,0.600,
		1.000,0.700, 1.000,0.600,
		0.000,0.800, 0.000,0.700,
		0.050,0.800, 0.050,0.700,
		0.100,0.800, 0.100,0.700,
		0.150,0.800, 0.150,0.700,
		0.200,0.800, 0.200,0.700,
		0.250,0.800, 0.250,0.700,
		0.300,0.800, 0.300,0.700,
		0.350,0.800, 0.350,0.700,
		0.400,0.800, 0.400,0.700,
		0.450,0.800, 0.450,0.700,
		0.500,0.800, 0.500,0.700,
		0.550,0.800, 0.550,0.700,
		0.600,0.800, 0.600,0.700,
		0.650,0.800, 0.650,0.700,
		0.700,0.800, 0.700,0.700,
		0.750,0.800, 0.750,0.700,
		0.800,0.800, 0.800,0.700,
		0.850,0.800, 0.850,0.700,
		0.900,0.800, 0.900,0.700,
		0.950,0.800, 0.950,0.700,
		1.000,0.800, 1.000,0.700,
		0.000,0.900, 0.000,0.800,
		0.050,0.900, 0.050,0.800,
		0.100,0.900, 0.100,0.800,
		0.150,0.900, 0.150,0.800,
		0.200,0.900, 0.200,0.800,
		0.250,0.900, 0.250,0.800,
		0.300,0.900, 0.300,0.800,
		0.350,0.900, 0.350,0.800,
		0.400,0.900, 0.400,0.800,
		0.450,0.900, 0.450,0.800,
		0.500,0.900, 0.500,0.800,
		0.550,0.900, 0.550,0.800,
		0.600,0.900, 0.600,0.800,
		0.650,0.900, 0.650,0.800,
		0.700,0.900, 0.700,0.800,
		0.750,0.900, 0.750,0.800,
		0.800,0.900, 0.800,0.800,
		0.850,0.900, 0.850,0.800,
		0.900,0.900, 0.900,0.800,
		0.950,0.900, 0.950,0.800,
		1.000,0.900, 1.000,0.800,
		0.000,1.000, 0.000,0.900,
		0.050,1.000, 0.050,0.900,
		0.100,1.000, 0.100,0.900,
		0.150,1.000, 0.150,0.900,
		0.200,1.000, 0.200,0.900,
		0.250,1.000, 0.250,0.900,
		0.300,1.000, 0.300,0.900,
		0.350,1.000, 0.350,0.900,
		0.400,1.000, 0.400,0.900,
		0.450,1.000, 0.450,0.900,
		0.500,1.000, 0.500,0.900,
		0.550,1.000, 0.550,0.900,
		0.600,1.000, 0.600,0.900,
		0.650,1.000, 0.650,0.900,
		0.700,1.000, 0.700,0.900,
		0.750,1.000, 0.750,0.900,
		0.800,1.000, 0.800,0.900,
		0.850,1.000, 0.850,0.900,
		0.900,1.000, 0.900,0.900,
		0.950,1.000, 0.950,0.900,
		1.000,1.000, 1.000,0.900,
		0.000,1.100, 0.000,1.000,
		0.050,1.100, 0.050,1.000,
		0.100,1.100, 0.100,1.000,
		0.150,1.100, 0.150,1.000,
		0.200,1.100, 0.200,1.000,
		0.250,1.100, 0.250,1.000,
		0.300,1.100, 0.300,1.000,
		0.350,1.100, 0.350,1.000,
		0.400,1.100, 0.400,1.000,
		0.450,1.100, 0.450,1.000,
		0.500,1.100, 0.500,1.000,
		0.550,1.100, 0.550,1.000,
		0.600,1.100, 0.600,1.000,
		0.650,1.100, 0.650,1.000,
		0.700,1.100, 0.700,1.000,
		0.750,1.100, 0.750,1.000,
		0.800,1.100, 0.800,1.000,
		0.850,1.100, 0.850,1.000,
		0.900,1.100, 0.900,1.000,
		0.950,1.100, 0.950,1.000,
		1.000,1.100, 1.000,1.000,
		0.000,1.200, 0.000,1.100,
		0.050,1.200, 0.050,1.100,
		0.100,1.200, 0.100,1.100,
		0.150,1.200, 0.150,1.100,
		0.200,1.200, 0.200,1.100,
		0.250,1.200, 0.250,1.100,
		0.300,1.200, 0.300,1.100,
		0.350,1.200, 0.350,1.100,
		0.400,1.200, 0.400,1.100,
		0.450,1.200, 0.450,1.100,
		0.500,1.200, 0.500,1.100,
		0.550,1.200, 0.550,1.100,
		0.600,1.200, 0.600,1.100,
		0.650,1.200, 0.650,1.100,
		0.700,1.200, 0.700,1.100,
		0.750,1.200, 0.750,1.100,
		0.800,1.200, 0.800,1.100,
		0.850,1.200, 0.850,1.100,
		0.900,1.200, 0.900,1.100,
		0.950,1.200, 0.950,1.100,
		1.000,1.200, 1.000,1.100,
		0.000,1.300, 0.000,1.200,
		0.050,1.300, 0.050,1.200,
		0.100,1.300, 0.100,1.200,
		0.150,1.300, 0.150,1.200,
		0.200,1.300, 0.200,1.200,
		0.250,1.300, 0.250,1.200,
		0.300,1.300, 0.300,1.200,
		0.350,1.300, 0.350,1.200,
		0.400,1.300, 0.400,1.200,
		0.450,1.300, 0.450,1.200,
		0.500,1.300, 0.500,1.200,
		0.550,1.300, 0.550,1.200,
		0.600,1.300, 0.600,1.200,
		0.650,1.300, 0.650,1.200,
		0.700,1.300, 0.700,1.200,
		0.750,1.300, 0.750,1.200,
		0.800,1.300, 0.800,1.200,
		0.850,1.300, 0.850,1.200,
		0.900,1.300, 0.900,1.200,
		0.950,1.300, 0.950,1.200,
		1.000,1.300, 1.000,1.200,
		0.000,1.400, 0.000,1.300,
		0.050,1.400, 0.050,1.300,
		0.100,1.400, 0.100,1.300,
		0.150,1.400, 0.150,1.300,
		0.200,1.400, 0.200,1.300,
		0.250,1.400, 0.250,1.300,
		0.300,1.400, 0.300,1.300,
		0.350,1.400, 0.350,1.300,
		0.400,1.400, 0.400,1.300,
		0.450,1.400, 0.450,1.300,
		0.500,1.400, 0.500,1.300,
		0.550,1.400, 0.550,1.300,
		0.600,1.400, 0.600,1.300,
		0.650,1.400, 0.650,1.300,
		0.700,1.400, 0.700,1.300,
		0.750,1.400, 0.750,1.300,
		0.800,1.400, 0.800,1.300,
		0.850,1.400, 0.850,1.300,
		0.900,1.400, 0.900,1.300,
		0.950,1.400, 0.950,1.300,
		1.000,1.400, 1.000,1.300,
		0.000,1.500, 0.000,1.400,
		0.050,1.500, 0.050,1.400,
		0.100,1.500, 0.100,1.400,
		0.150,1.500, 0.150,1.400,
		0.200,1.500, 0.200,1.400,
		0.250,1.500, 0.250,1.400,
		0.300,1.500, 0.300,1.400,
		0.350,1.500, 0.350,1.400,
		0.400,1.500, 0.400,1.400,
		0.450,1.500, 0.450,1.400,
		0.500,1.500, 0.500,1.400,
		0.550,1.500, 0.550,1.400,
		0.600,1.500, 0.600,1.400,
		0.650,1.500, 0.650,1.400,
		0.700,1.500, 0.700,1.400,
		0.750,1.500, 0.750,1.400,
		0.800,1.500, 0.800,1.400,
		0.850,1.500, 0.850,1.400,
		0.900,1.500, 0.900,1.400,
		0.950,1.500, 0.950,1.400,
		1.000,1.500, 1.000,1.400,
		0.000,1.600, 0.000,1.500,
		0.050,1.600, 0.050,1.500,
		0.100,1.600, 0.100,1.500,
		0.150,1.600, 0.150,1.500,
		0.200,1.600, 0.200,1.500,
		0.250,1.600, 0.250,1.500,
		0.300,1.600, 0.300,1.500,
		0.350,1.600, 0.350,1.500,
		0.400,1.600, 0.400,1.500,
		0.450,1.600, 0.450,1.500,
		0.500,1.600, 0.500,1.500,
		0.550,1.600, 0.550,1.500,
		0.600,1.600, 0.600,1.500,
		0.650,1.600, 0.650,1.500,
		0.700,1.600, 0.700,1.500,
		0.750,1.600, 0.750,1.500,
		0.800,1.600, 0.800,1.500,
		0.850,1.600, 0.850,1.500,
		0.900,1.600, 0.900,1.500,
		0.950,1.600, 0.950,1.500,
		1.000,1.600, 1.000,1.500,
		0.000,1.700, 0.000,1.600,
		0.050,1.700, 0.050,1.600,
		0.100,1.700, 0.100,1.600,
		0.150,1.700, 0.150,1.600,
		0.200,1.700, 0.200,1.600,
		0.250,1.700, 0.250,1.600,
		0.300,1.700, 0.300,1.600,
		0.350,1.700, 0.350,1.600,
		0.400,1.700, 0.400,1.600,
		0.450,1.700, 0.450,1.600,
		0.500,1.700, 0.500,1.600,
		0.550,1.700, 0.550,1.600,
		0.600,1.700, 0.600,1.600,
		0.650,1.700, 0.650,1.600,
		0.700,1.700, 0.700,1.600,
		0.750,1.700, 0.750,1.600,
		0.800,1.700, 0.800,1.600,
		0.850,1.700, 0.850,1.600,
		0.900,1.700, 0.900,1.600,
		0.950,1.700, 0.950,1.600,
		1.000,1.700, 1.000,1.600,
		0.000,1.800, 0.000,1.700,
		0.050,1.800, 0.050,1.700,
		0.100,1.800, 0.100,1.700,
		0.150,1.800, 0.150,1.700,
		0.200,1.800, 0.200,1.700,
		0.250,1.800, 0.250,1.700,
		0.300,1.800, 0.300,1.700,
		0.350,1.800, 0.350,1.700,
		0.400,1.800, 0.400,1.700,
		0.450,1.800, 0.450,1.700,
		0.500,1.800, 0.500,1.700,
		0.550,1.800, 0.550,1.700,
		0.600,1.800, 0.600,1.700,
		0.650,1.800, 0.650,1.700,
		0.700,1.800, 0.700,1.700,
		0.750,1.800, 0.750,1.700,
		0.800,1.800, 0.800,1.700,
		0.850,1.800, 0.850,1.700,
		0.900,1.800, 0.900,1.700,
		0.950,1.800, 0.950,1.700,
		1.000,1.800, 1.000,1.700,
		0.000,1.900, 0.000,1.800,
		0.050,1.900, 0.050,1.800,
		0.100,1.900, 0.100,1.800,
		0.150,1.900, 0.150,1.800,
		0.200,1.900, 0.200,1.800,
		0.250,1.900, 0.250,1.800,
		0.300,1.900, 0.300,1.800,
		0.350,1.900, 0.350,1.800,
		0.400,1.900, 0.400,1.800,
		0.450,1.900, 0.450,1.800,
		0.500,1.900, 0.500,1.800,
		0.550,1.900, 0.550,1.800,
		0.600,1.900, 0.600,1.800,
		0.650,1.900, 0.650,1.800,
		0.700,1.900, 0.700,1.800,
		0.750,1.900, 0.750,1.800,
		0.800,1.900, 0.800,1.800,
		0.850,1.900, 0.850,1.800,
		0.900,1.900, 0.900,1.800,
		0.950,1.900, 0.950,1.800,
		1.000,1.900, 1.000,1.800,
		0.000,2.000, 0.000,1.900,
		0.050,2.000, 0.050,1.900,
		0.100,2.000, 0.100,1.900,
		0.150,2.000, 0.150,1.900,
		0.200,2.000, 0.200,1.900,
		0.250,2.000, 0.250,1.900,
		0.300,2.000, 0.300,1.900,
		0.350,2.000, 0.350,1.900,
		0.400,2.000, 0.400,1.900,
		0.450,2.000, 0.450,1.900,
		0.500,2.000, 0.500,1.900,
		0.550,2.000, 0.550,1.900,
		0.600,2.000, 0.600,1.900,
		0.650,2.000, 0.650,1.900,
		0.700,2.000, 0.700,1.900,
		0.750,2.000, 0.750,1.900,
		0.800,2.000, 0.800,1.900,
		0.850,2.000, 0.850,1.900,
		0.900,2.000, 0.900,1.900,
		0.950,2.000, 0.950,1.900,
		1.000,2.000, 1.000,1.900,
};

GLfloat spherenorms[] = {
		0.000,-0.951,-0.309, 0.000,-1.000,-0.000,
		-0.095,-0.951,-0.294, -0.000,-1.000,-0.000,
		-0.182,-0.951,-0.250, -0.000,-1.000,-0.000,
		-0.250,-0.951,-0.182, -0.000,-1.000,-0.000,
		-0.294,-0.951,-0.095, -0.000,-1.000,-0.000,
		-0.309,-0.951,0.000, -0.000,-1.000,0.000,
		-0.294,-0.951,0.095, -0.000,-1.000,0.000,
		-0.250,-0.951,0.182, -0.000,-1.000,0.000,
		-0.182,-0.951,0.250, -0.000,-1.000,0.000,
		-0.095,-0.951,0.294, -0.000,-1.000,0.000,
		0.000,-0.951,0.309, 0.000,-1.000,0.000,
		0.095,-0.951,0.294, 0.000,-1.000,0.000,
		0.182,-0.951,0.250, 0.000,-1.000,0.000,
		0.250,-0.951,0.182, 0.000,-1.000,0.000,
		0.294,-0.951,0.095, 0.000,-1.000,0.000,
		0.309,-0.951,-0.000, 0.000,-1.000,-0.000,
		0.294,-0.951,-0.095, 0.000,-1.000,-0.000,
		0.250,-0.951,-0.182, 0.000,-1.000,-0.000,
		0.182,-0.951,-0.250, 0.000,-1.000,-0.000,
		0.095,-0.951,-0.294, 0.000,-1.000,-0.000,
		-0.000,-0.951,-0.309, -0.000,-1.000,-0.000,
		0.000,-0.809,-0.588, 0.000,-0.951,-0.309,
		-0.182,-0.809,-0.559, -0.095,-0.951,-0.294,
		-0.345,-0.809,-0.476, -0.182,-0.951,-0.250,
		-0.476,-0.809,-0.345, -0.250,-0.951,-0.182,
		-0.559,-0.809,-0.182, -0.294,-0.951,-0.095,
		-0.588,-0.809,0.000, -0.309,-0.951,0.000,
		-0.559,-0.809,0.182, -0.294,-0.951,0.095,
		-0.476,-0.809,0.345, -0.250,-0.951,0.182,
		-0.345,-0.809,0.476, -0.182,-0.951,0.250,
		-0.182,-0.809,0.559, -0.095,-0.951,0.294,
		0.000,-0.809,0.588, 0.000,-0.951,0.309,
		0.182,-0.809,0.559, 0.095,-0.951,0.294,
		0.345,-0.809,0.476, 0.182,-0.951,0.250,
		0.476,-0.809,0.345, 0.250,-0.951,0.182,
		0.559,-0.809,0.182, 0.294,-0.951,0.095,
		0.588,-0.809,-0.000, 0.309,-0.951,-0.000,
		0.559,-0.809,-0.182, 0.294,-0.951,-0.095,
		0.476,-0.809,-0.345, 0.250,-0.951,-0.182,
		0.345,-0.809,-0.476, 0.182,-0.951,-0.250,
		0.182,-0.809,-0.559, 0.095,-0.951,-0.294,
		-0.000,-0.809,-0.588, -0.000,-0.951,-0.309,
		0.000,-0.588,-0.809, 0.000,-0.809,-0.588,
		-0.250,-0.588,-0.769, -0.182,-0.809,-0.559,
		-0.476,-0.588,-0.655, -0.345,-0.809,-0.476,
		-0.655,-0.588,-0.476, -0.476,-0.809,-0.345,
		-0.769,-0.588,-0.250, -0.559,-0.809,-0.182,
		-0.809,-0.588,0.000, -0.588,-0.809,0.000,
		-0.769,-0.588,0.250, -0.559,-0.809,0.182,
		-0.655,-0.588,0.476, -0.476,-0.809,0.345,
		-0.476,-0.588,0.655, -0.345,-0.809,0.476,
		-0.250,-0.588,0.769, -0.182,-0.809,0.559,
		0.000,-0.588,0.809, 0.000,-0.809,0.588,
		0.250,-0.588,0.769, 0.182,-0.809,0.559,
		0.476,-0.588,0.655, 0.345,-0.809,0.476,
		0.655,-0.588,0.476, 0.476,-0.809,0.345,
		0.769,-0.588,0.250, 0.559,-0.809,0.182,
		0.809,-0.588,-0.000, 0.588,-0.809,-0.000,
		0.769,-0.588,-0.250, 0.559,-0.809,-0.182,
		0.655,-0.588,-0.476, 0.476,-0.809,-0.345,
		0.476,-0.588,-0.655, 0.345,-0.809,-0.476,
		0.250,-0.588,-0.769, 0.182,-0.809,-0.559,
		-0.000,-0.588,-0.809, -0.000,-0.809,-0.588,
		0.000,-0.309,-0.951, 0.000,-0.588,-0.809,
		-0.294,-0.309,-0.905, -0.250,-0.588,-0.769,
		-0.559,-0.309,-0.769, -0.476,-0.588,-0.655,
		-0.769,-0.309,-0.559, -0.655,-0.588,-0.476,
		-0.905,-0.309,-0.294, -0.769,-0.588,-0.250,
		-0.951,-0.309,0.000, -0.809,-0.588,0.000,
		-0.905,-0.309,0.294, -0.769,-0.588,0.250,
		-0.769,-0.309,0.559, -0.655,-0.588,0.476,
		-0.559,-0.309,0.769, -0.476,-0.588,0.655,
		-0.294,-0.309,0.905, -0.250,-0.588,0.769,
		0.000,-0.309,0.951, 0.000,-0.588,0.809,
		0.294,-0.309,0.905, 0.250,-0.588,0.769,
		0.559,-0.309,0.769, 0.476,-0.588,0.655,
		0.769,-0.309,0.559, 0.655,-0.588,0.476,
		0.905,-0.309,0.294, 0.769,-0.588,0.250,
		0.951,-0.309,-0.000, 0.809,-0.588,-0.000,
		0.905,-0.309,-0.294, 0.769,-0.588,-0.250,
		0.769,-0.309,-0.559, 0.655,-0.588,-0.476,
		0.559,-0.309,-0.769, 0.476,-0.588,-0.655,
		0.294,-0.309,-0.905, 0.250,-0.588,-0.769,
		-0.000,-0.309,-0.951, -0.000,-0.588,-0.809,
		0.000,0.000,-1.000, 0.000,-0.309,-0.951,
		-0.309,0.000,-0.951, -0.294,-0.309,-0.905,
		-0.588,0.000,-0.809, -0.559,-0.309,-0.769,
		-0.809,0.000,-0.588, -0.769,-0.309,-0.559,
		-0.951,0.000,-0.309, -0.905,-0.309,-0.294,
		-1.000,0.000,0.000, -0.951,-0.309,0.000,
		-0.951,0.000,0.309, -0.905,-0.309,0.294,
		-0.809,0.000,0.588, -0.769,-0.309,0.559,
		-0.588,0.000,0.809, -0.559,-0.309,0.769,
		-0.309,0.000,0.951, -0.294,-0.309,0.905,
		0.000,0.000,1.000, 0.000,-0.309,0.951,
		0.309,0.000,0.951, 0.294,-0.309,0.905,
		0.588,0.000,0.809, 0.559,-0.309,0.769,
		0.809,0.000,0.588, 0.769,-0.309,0.559,
		0.951,0.000,0.309, 0.905,-0.309,0.294,
		1.000,0.000,-0.000, 0.951,-0.309,-0.000,
		0.951,0.000,-0.309, 0.905,-0.309,-0.294,
		0.809,0.000,-0.588, 0.769,-0.309,-0.559,
		0.588,0.000,-0.809, 0.559,-0.309,-0.769,
		0.309,0.000,-0.951, 0.294,-0.309,-0.905,
		-0.000,0.000,-1.000, -0.000,-0.309,-0.951,
		0.000,0.309,-0.951, 0.000,0.000,-1.000,
		-0.294,0.309,-0.905, -0.309,0.000,-0.951,
		-0.559,0.309,-0.769, -0.588,0.000,-0.809,
		-0.769,0.309,-0.559, -0.809,0.000,-0.588,
		-0.905,0.309,-0.294, -0.951,0.000,-0.309,
		-0.951,0.309,0.000, -1.000,0.000,0.000,
		-0.905,0.309,0.294, -0.951,0.000,0.309,
		-0.769,0.309,0.559, -0.809,0.000,0.588,
		-0.559,0.309,0.769, -0.588,0.000,0.809,
		-0.294,0.309,0.905, -0.309,0.000,0.951,
		0.000,0.309,0.951, 0.000,0.000,1.000,
		0.294,0.309,0.905, 0.309,0.000,0.951,
		0.559,0.309,0.769, 0.588,0.000,0.809,
		0.769,0.309,0.559, 0.809,0.000,0.588,
		0.905,0.309,0.294, 0.951,0.000,0.309,
		0.951,0.309,-0.000, 1.000,0.000,-0.000,
		0.905,0.309,-0.294, 0.951,0.000,-0.309,
		0.769,0.309,-0.559, 0.809,0.000,-0.588,
		0.559,0.309,-0.769, 0.588,0.000,-0.809,
		0.294,0.309,-0.905, 0.309,0.000,-0.951,
		-0.000,0.309,-0.951, -0.000,0.000,-1.000,
		0.000,0.588,-0.809, 0.000,0.309,-0.951,
		-0.250,0.588,-0.769, -0.294,0.309,-0.905,
		-0.476,0.588,-0.655, -0.559,0.309,-0.769,
		-0.655,0.588,-0.476, -0.769,0.309,-0.559,
		-0.769,0.588,-0.250, -0.905,0.309,-0.294,
		-0.809,0.588,0.000, -0.951,0.309,0.000,
		-0.769,0.588,0.250, -0.905,0.309,0.294,
		-0.655,0.588,0.476, -0.769,0.309,0.559,
		-0.476,0.588,0.655, -0.559,0.309,0.769,
		-0.250,0.588,0.769, -0.294,0.309,0.905,
		0.000,0.588,0.809, 0.000,0.309,0.951,
		0.250,0.588,0.769, 0.294,0.309,0.905,
		0.476,0.588,0.655, 0.559,0.309,0.769,
		0.655,0.588,0.476, 0.769,0.309,0.559,
		0.769,0.588,0.250, 0.905,0.309,0.294,
		0.809,0.588,-0.000, 0.951,0.309,-0.000,
		0.769,0.588,-0.250, 0.905,0.309,-0.294,
		0.655,0.588,-0.476, 0.769,0.309,-0.559,
		0.476,0.588,-0.655, 0.559,0.309,-0.769,
		0.250,0.588,-0.769, 0.294,0.309,-0.905,
		-0.000,0.588,-0.809, -0.000,0.309,-0.951,
		0.000,0.809,-0.588, 0.000,0.588,-0.809,
		-0.182,0.809,-0.559, -0.250,0.588,-0.769,
		-0.345,0.809,-0.476, -0.476,0.588,-0.655,
		-0.476,0.809,-0.345, -0.655,0.588,-0.476,
		-0.559,0.809,-0.182, -0.769,0.588,-0.250,
		-0.588,0.809,0.000, -0.809,0.588,0.000,
		-0.559,0.809,0.182, -0.769,0.588,0.250,
		-0.476,0.809,0.345, -0.655,0.588,0.476,
		-0.345,0.809,0.476, -0.476,0.588,0.655,
		-0.182,0.809,0.559, -0.250,0.588,0.769,
		0.000,0.809,0.588, 0.000,0.588,0.809,
		0.182,0.809,0.559, 0.250,0.588,0.769,
		0.345,0.809,0.476, 0.476,0.588,0.655,
		0.476,0.809,0.345, 0.655,0.588,0.476,
		0.559,0.809,0.182, 0.769,0.588,0.250,
		0.588,0.809,-0.000, 0.809,0.588,-0.000,
		0.559,0.809,-0.182, 0.769,0.588,-0.250,
		0.476,0.809,-0.345, 0.655,0.588,-0.476,
		0.345,0.809,-0.476, 0.476,0.588,-0.655,
		0.182,0.809,-0.559, 0.250,0.588,-0.769,
		-0.000,0.809,-0.588, -0.000,0.588,-0.809,
		0.000,0.951,-0.309, 0.000,0.809,-0.588,
		-0.095,0.951,-0.294, -0.182,0.809,-0.559,
		-0.182,0.951,-0.250, -0.345,0.809,-0.476,
		-0.250,0.951,-0.182, -0.476,0.809,-0.345,
		-0.294,0.951,-0.095, -0.559,0.809,-0.182,
		-0.309,0.951,0.000, -0.588,0.809,0.000,
		-0.294,0.951,0.095, -0.559,0.809,0.182,
		-0.250,0.951,0.182, -0.476,0.809,0.345,
		-0.182,0.951,0.250, -0.345,0.809,0.476,
		-0.095,0.951,0.294, -0.182,0.809,0.559,
		0.000,0.951,0.309, 0.000,0.809,0.588,
		0.095,0.951,0.294, 0.182,0.809,0.559,
		0.182,0.951,0.250, 0.345,0.809,0.476,
		0.250,0.951,0.182, 0.476,0.809,0.345,
		0.294,0.951,0.095, 0.559,0.809,0.182,
		0.309,0.951,-0.000, 0.588,0.809,-0.000,
		0.294,0.951,-0.095, 0.559,0.809,-0.182,
		0.250,0.951,-0.182, 0.476,0.809,-0.345,
		0.182,0.951,-0.250, 0.345,0.809,-0.476,
		0.095,0.951,-0.294, 0.182,0.809,-0.559,
		-0.000,0.951,-0.309, -0.000,0.809,-0.588,
		-0.000,1.000,0.000, 0.000,0.951,-0.309,
		0.000,1.000,0.000, -0.095,0.951,-0.294,
		0.000,1.000,0.000, -0.182,0.951,-0.250,
		0.000,1.000,0.000, -0.250,0.951,-0.182,
		0.000,1.000,0.000, -0.294,0.951,-0.095,
		0.000,1.000,-0.000, -0.309,0.951,0.000,
		0.000,1.000,-0.000, -0.294,0.951,0.095,
		0.000,1.000,-0.000, -0.250,0.951,0.182,
		0.000,1.000,-0.000, -0.182,0.951,0.250,
		0.000,1.000,-0.000, -0.095,0.951,0.294,
		-0.000,1.000,-0.000, 0.000,0.951,0.309,
		-0.000,1.000,-0.000, 0.095,0.951,0.294,
		-0.000,1.000,-0.000, 0.182,0.951,0.250,
		-0.000,1.000,-0.000, 0.250,0.951,0.182,
		-0.000,1.000,-0.000, 0.294,0.951,0.095,
		-0.000,1.000,0.000, 0.309,0.951,-0.000,
		-0.000,1.000,0.000, 0.294,0.951,-0.095,
		-0.000,1.000,0.000, 0.250,0.951,-0.182,
		-0.000,1.000,0.000, 0.182,0.951,-0.250,
		-0.000,1.000,0.000, 0.095,0.951,-0.294,
		0.000,1.000,0.000, -0.000,0.951,-0.309,
		-0.000,0.951,0.309, -0.000,1.000,0.000,
		0.095,0.951,0.294, 0.000,1.000,0.000,
		0.182,0.951,0.250, 0.000,1.000,0.000,
		0.250,0.951,0.182, 0.000,1.000,0.000,
		0.294,0.951,0.095, 0.000,1.000,0.000,
		0.309,0.951,-0.000, 0.000,1.000,-0.000,
		0.294,0.951,-0.095, 0.000,1.000,-0.000,
		0.250,0.951,-0.182, 0.000,1.000,-0.000,
		0.182,0.951,-0.250, 0.000,1.000,-0.000,
		0.095,0.951,-0.294, 0.000,1.000,-0.000,
		-0.000,0.951,-0.309, -0.000,1.000,-0.000,
		-0.095,0.951,-0.294, -0.000,1.000,-0.000,
		-0.182,0.951,-0.250, -0.000,1.000,-0.000,
		-0.250,0.951,-0.182, -0.000,1.000,-0.000,
		-0.294,0.951,-0.095, -0.000,1.000,-0.000,
		-0.309,0.951,0.000, -0.000,1.000,0.000,
		-0.294,0.951,0.095, -0.000,1.000,0.000,
		-0.250,0.951,0.182, -0.000,1.000,0.000,
		-0.182,0.951,0.250, -0.000,1.000,0.000,
		-0.095,0.951,0.294, -0.000,1.000,0.000,
		0.000,0.951,0.309, 0.000,1.000,0.000,
		-0.000,0.809,0.588, -0.000,0.951,0.309,
		0.182,0.809,0.559, 0.095,0.951,0.294,
		0.345,0.809,0.476, 0.182,0.951,0.250,
		0.476,0.809,0.345, 0.250,0.951,0.182,
		0.559,0.809,0.182, 0.294,0.951,0.095,
		0.588,0.809,-0.000, 0.309,0.951,-0.000,
		0.559,0.809,-0.182, 0.294,0.951,-0.095,
		0.476,0.809,-0.345, 0.250,0.951,-0.182,
		0.345,0.809,-0.476, 0.182,0.951,-0.250,
		0.182,0.809,-0.559, 0.095,0.951,-0.294,
		-0.000,0.809,-0.588, -0.000,0.951,-0.309,
		-0.182,0.809,-0.559, -0.095,0.951,-0.294,
		-0.345,0.809,-0.476, -0.182,0.951,-0.250,
		-0.476,0.809,-0.345, -0.250,0.951,-0.182,
		-0.559,0.809,-0.182, -0.294,0.951,-0.095,
		-0.588,0.809,0.000, -0.309,0.951,0.000,
		-0.559,0.809,0.182, -0.294,0.951,0.095,
		-0.476,0.809,0.345, -0.250,0.951,0.182,
		-0.345,0.809,0.476, -0.182,0.951,0.250,
		-0.182,0.809,0.559, -0.095,0.951,0.294,
		0.000,0.809,0.588, 0.000,0.951,0.309,
		-0.000,0.588,0.809, -0.000,0.809,0.588,
		0.250,0.588,0.769, 0.182,0.809,0.559,
		0.476,0.588,0.655, 0.345,0.809,0.476,
		0.655,0.588,0.476, 0.476,0.809,0.345,
		0.769,0.588,0.250, 0.559,0.809,0.182,
		0.809,0.588,-0.000, 0.588,0.809,-0.000,
		0.769,0.588,-0.250, 0.559,0.809,-0.182,
		0.655,0.588,-0.476, 0.476,0.809,-0.345,
		0.476,0.588,-0.655, 0.345,0.809,-0.476,
		0.250,0.588,-0.769, 0.182,0.809,-0.559,
		-0.000,0.588,-0.809, -0.000,0.809,-0.588,
		-0.250,0.588,-0.769, -0.182,0.809,-0.559,
		-0.476,0.588,-0.655, -0.345,0.809,-0.476,
		-0.655,0.588,-0.476, -0.476,0.809,-0.345,
		-0.769,0.588,-0.250, -0.559,0.809,-0.182,
		-0.809,0.588,0.000, -0.588,0.809,0.000,
		-0.769,0.588,0.250, -0.559,0.809,0.182,
		-0.655,0.588,0.476, -0.476,0.809,0.345,
		-0.476,0.588,0.655, -0.345,0.809,0.476,
		-0.250,0.588,0.769, -0.182,0.809,0.559,
		0.000,0.588,0.809, 0.000,0.809,0.588,
		-0.000,0.309,0.951, -0.000,0.588,0.809,
		0.294,0.309,0.905, 0.250,0.588,0.769,
		0.559,0.309,0.769, 0.476,0.588,0.655,
		0.769,0.309,0.559, 0.655,0.588,0.476,
		0.905,0.309,0.294, 0.769,0.588,0.250,
		0.951,0.309,-0.000, 0.809,0.588,-0.000,
		0.905,0.309,-0.294, 0.769,0.588,-0.250,
		0.769,0.309,-0.559, 0.655,0.588,-0.476,
		0.559,0.309,-0.769, 0.476,0.588,-0.655,
		0.294,0.309,-0.905, 0.250,0.588,-0.769,
		-0.000,0.309,-0.951, -0.000,0.588,-0.809,
		-0.294,0.309,-0.905, -0.250,0.588,-0.769,
		-0.559,0.309,-0.769, -0.476,0.588,-0.655,
		-0.769,0.309,-0.559, -0.655,0.588,-0.476,
		-0.905,0.309,-0.294, -0.769,0.588,-0.250,
		-0.951,0.309,0.000, -0.809,0.588,0.000,
		-0.905,0.309,0.294, -0.769,0.588,0.250,
		-0.769,0.309,0.559, -0.655,0.588,0.476,
		-0.559,0.309,0.769, -0.476,0.588,0.655,
		-0.294,0.309,0.905, -0.250,0.588,0.769,
		0.000,0.309,0.951, 0.000,0.588,0.809,
		-0.000,-0.000,1.000, -0.000,0.309,0.951,
		0.309,-0.000,0.951, 0.294,0.309,0.905,
		0.588,-0.000,0.809, 0.559,0.309,0.769,
		0.809,-0.000,0.588, 0.769,0.309,0.559,
		0.951,-0.000,0.309, 0.905,0.309,0.294,
		1.000,-0.000,-0.000, 0.951,0.309,-0.000,
		0.951,-0.000,-0.309, 0.905,0.309,-0.294,
		0.809,-0.000,-0.588, 0.769,0.309,-0.559,
		0.588,-0.000,-0.809, 0.559,0.309,-0.769,
		0.309,-0.000,-0.951, 0.294,0.309,-0.905,
		-0.000,-0.000,-1.000, -0.000,0.309,-0.951,
		-0.309,-0.000,-0.951, -0.294,0.309,-0.905,
		-0.588,-0.000,-0.809, -0.559,0.309,-0.769,
		-0.809,-0.000,-0.588, -0.769,0.309,-0.559,
		-0.951,-0.000,-0.309, -0.905,0.309,-0.294,
		-1.000,-0.000,0.000, -0.951,0.309,0.000,
		-0.951,-0.000,0.309, -0.905,0.309,0.294,
		-0.809,-0.000,0.588, -0.769,0.309,0.559,
		-0.588,-0.000,0.809, -0.559,0.309,0.769,
		-0.309,-0.000,0.951, -0.294,0.309,0.905,
		0.000,-0.000,1.000, 0.000,0.309,0.951,
		-0.000,-0.309,0.951, -0.000,-0.000,1.000,
		0.294,-0.309,0.905, 0.309,-0.000,0.951,
		0.559,-0.309,0.769, 0.588,-0.000,0.809,
		0.769,-0.309,0.559, 0.809,-0.000,0.588,
		0.905,-0.309,0.294, 0.951,-0.000,0.309,
		0.951,-0.309,-0.000, 1.000,-0.000,-0.000,
		0.905,-0.309,-0.294, 0.951,-0.000,-0.309,
		0.769,-0.309,-0.559, 0.809,-0.000,-0.588,
		0.559,-0.309,-0.769, 0.588,-0.000,-0.809,
		0.294,-0.309,-0.905, 0.309,-0.000,-0.951,
		-0.000,-0.309,-0.951, -0.000,-0.000,-1.000,
		-0.294,-0.309,-0.905, -0.309,-0.000,-0.951,
		-0.559,-0.309,-0.769, -0.588,-0.000,-0.809,
		-0.769,-0.309,-0.559, -0.809,-0.000,-0.588,
		-0.905,-0.309,-0.294, -0.951,-0.000,-0.309,
		-0.951,-0.309,0.000, -1.000,-0.000,0.000,
		-0.905,-0.309,0.294, -0.951,-0.000,0.309,
		-0.769,-0.309,0.559, -0.809,-0.000,0.588,
		-0.559,-0.309,0.769, -0.588,-0.000,0.809,
		-0.294,-0.309,0.905, -0.309,-0.000,0.951,
		0.000,-0.309,0.951, 0.000,-0.000,1.000,
		-0.000,-0.588,0.809, -0.000,-0.309,0.951,
		0.250,-0.588,0.769, 0.294,-0.309,0.905,
		0.476,-0.588,0.655, 0.559,-0.309,0.769,
		0.655,-0.588,0.476, 0.769,-0.309,0.559,
		0.769,-0.588,0.250, 0.905,-0.309,0.294,
		0.809,-0.588,-0.000, 0.951,-0.309,-0.000,
		0.769,-0.588,-0.250, 0.905,-0.309,-0.294,
		0.655,-0.588,-0.476, 0.769,-0.309,-0.559,
		0.476,-0.588,-0.655, 0.559,-0.309,-0.769,
		0.250,-0.588,-0.769, 0.294,-0.309,-0.905,
		-0.000,-0.588,-0.809, -0.000,-0.309,-0.951,
		-0.250,-0.588,-0.769, -0.294,-0.309,-0.905,
		-0.476,-0.588,-0.655, -0.559,-0.309,-0.769,
		-0.655,-0.588,-0.476, -0.769,-0.309,-0.559,
		-0.769,-0.588,-0.250, -0.905,-0.309,-0.294,
		-0.809,-0.588,0.000, -0.951,-0.309,0.000,
		-0.769,-0.588,0.250, -0.905,-0.309,0.294,
		-0.655,-0.588,0.476, -0.769,-0.309,0.559,
		-0.476,-0.588,0.655, -0.559,-0.309,0.769,
		-0.250,-0.588,0.769, -0.294,-0.309,0.905,
		0.000,-0.588,0.809, 0.000,-0.309,0.951,
		-0.000,-0.809,0.588, -0.000,-0.588,0.809,
		0.182,-0.809,0.559, 0.250,-0.588,0.769,
		0.345,-0.809,0.476, 0.476,-0.588,0.655,
		0.476,-0.809,0.345, 0.655,-0.588,0.476,
		0.559,-0.809,0.182, 0.769,-0.588,0.250,
		0.588,-0.809,-0.000, 0.809,-0.588,-0.000,
		0.559,-0.809,-0.182, 0.769,-0.588,-0.250,
		0.476,-0.809,-0.345, 0.655,-0.588,-0.476,
		0.345,-0.809,-0.476, 0.476,-0.588,-0.655,
		0.182,-0.809,-0.559, 0.250,-0.588,-0.769,
		-0.000,-0.809,-0.588, -0.000,-0.588,-0.809,
		-0.182,-0.809,-0.559, -0.250,-0.588,-0.769,
		-0.345,-0.809,-0.476, -0.476,-0.588,-0.655,
		-0.476,-0.809,-0.345, -0.655,-0.588,-0.476,
		-0.559,-0.809,-0.182, -0.769,-0.588,-0.250,
		-0.588,-0.809,0.000, -0.809,-0.588,0.000,
		-0.559,-0.809,0.182, -0.769,-0.588,0.250,
		-0.476,-0.809,0.345, -0.655,-0.588,0.476,
		-0.345,-0.809,0.476, -0.476,-0.588,0.655,
		-0.182,-0.809,0.559, -0.250,-0.588,0.769,
		0.000,-0.809,0.588, 0.000,-0.588,0.809,
		-0.000,-0.951,0.309, -0.000,-0.809,0.588,
		0.095,-0.951,0.294, 0.182,-0.809,0.559,
		0.182,-0.951,0.250, 0.345,-0.809,0.476,
		0.250,-0.951,0.182, 0.476,-0.809,0.345,
		0.294,-0.951,0.095, 0.559,-0.809,0.182,
		0.309,-0.951,-0.000, 0.588,-0.809,-0.000,
		0.294,-0.951,-0.095, 0.559,-0.809,-0.182,
		0.250,-0.951,-0.182, 0.476,-0.809,-0.345,
		0.182,-0.951,-0.250, 0.345,-0.809,-0.476,
		0.095,-0.951,-0.294, 0.182,-0.809,-0.559,
		-0.000,-0.951,-0.309, -0.000,-0.809,-0.588,
		-0.095,-0.951,-0.294, -0.182,-0.809,-0.559,
		-0.182,-0.951,-0.250, -0.345,-0.809,-0.476,
		-0.250,-0.951,-0.182, -0.476,-0.809,-0.345,
		-0.294,-0.951,-0.095, -0.559,-0.809,-0.182,
		-0.309,-0.951,0.000, -0.588,-0.809,0.000,
		-0.294,-0.951,0.095, -0.559,-0.809,0.182,
		-0.250,-0.951,0.182, -0.476,-0.809,0.345,
		-0.182,-0.951,0.250, -0.345,-0.809,0.476,
		-0.095,-0.951,0.294, -0.182,-0.809,0.559,
		0.000,-0.951,0.309, 0.000,-0.809,0.588,
		0.000,-1.000,-0.000, -0.000,-0.951,0.309,
		-0.000,-1.000,-0.000, 0.095,-0.951,0.294,
		-0.000,-1.000,-0.000, 0.182,-0.951,0.250,
		-0.000,-1.000,-0.000, 0.250,-0.951,0.182,
		-0.000,-1.000,-0.000, 0.294,-0.951,0.095,
		-0.000,-1.000,0.000, 0.309,-0.951,-0.000,
		-0.000,-1.000,0.000, 0.294,-0.951,-0.095,
		-0.000,-1.000,0.000, 0.250,-0.951,-0.182,
		-0.000,-1.000,0.000, 0.182,-0.951,-0.250,
		-0.000,-1.000,0.000, 0.095,-0.951,-0.294,
		0.000,-1.000,0.000, -0.000,-0.951,-0.309,
		0.000,-1.000,0.000, -0.095,-0.951,-0.294,
		0.000,-1.000,0.000, -0.182,-0.951,-0.250,
		0.000,-1.000,0.000, -0.250,-0.951,-0.182,
		0.000,-1.000,0.000, -0.294,-0.951,-0.095,
		0.000,-1.000,-0.000, -0.309,-0.951,0.000,
		0.000,-1.000,-0.000, -0.294,-0.951,0.095,
		0.000,-1.000,-0.000, -0.250,-0.951,0.182,
		0.000,-1.000,-0.000, -0.182,-0.951,0.250,
		0.000,-1.000,-0.000, -0.095,-0.951,0.294,
		-0.000,-1.000,-0.000, 0.000,-0.951,0.309,
};
