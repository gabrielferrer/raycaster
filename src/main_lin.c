/*
	project: raycaster
	file: main_lin.c
	programming: Gabriel Ferrer
	date: 16-08-2012
*/

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include "defs.h"
#include "state.h"
#include "render.h"
#include "main.h"

Display* Dsp;
Window Wnd;
int Scr;
GC Gc;
XImage* Image;
Visual* Vsl;
XVisualInfo VisualInfo;
XSetWindowAttributes Attribs;

/*
	Returns time tick in microseconds.
*/
long M_GetTick () {
    struct timeval timeVal;
    gettimeofday (&timeVal, NULL);
    return (long) (timeVal.tv_sec * 1000000 + timeVal.tv_usec);
}

void M_HandleKeyPress (unsigned int keycode) {
	switch (XKeycodeToKeysym(Dsp, keycode, 0)) {
		case XK_Left:
			ST_CalcDirection (-1.0);
			break;
		case XK_Right:
			ST_CalcDirection (1.0);
			break;
		case XK_Up:
			ST_CalcPosition (1.0);
			break;
		case XK_Down:
			ST_CalcPosition (-1.0);
			break;
		default:
			break;
	}
}

bool_t M_Init () {
	unsigned long attribmask, valuemask;
	long infomask;
	Window rootwnd;
	XGCValues gcvalues;

	if ((Dsp = (Display*) XOpenDisplay (NULL)) == NULL)
		return false;

	Scr = XDefaultScreen (Dsp);
	rootwnd = XRootWindow (Dsp, Scr);
	
	if (!XMatchVisualInfo (Dsp, Scr, DEPTH, TrueColor, &VisualInfo))
		return false;

	Vsl = VisualInfo.visual;
	Vsl = (Visual*) XDefaultVisual (Dsp, Scr);

	attribmask = CWEventMask | CWBorderPixel;
	Attribs.border_pixel = 0;
	Attribs.event_mask = KeyPressMask | ExposureMask;

	if ((Wnd = XCreateWindow (Dsp, rootwnd, 0, 0, WINDOWWIDTH, WINDOWHEIGHT, 0, DEPTH, InputOutput, Vsl, attribmask, &Attribs)) == NULL)
		return false;

	valuemask = GCGraphicsExposures;
	gcvalues.graphics_exposures = True;

	Gc = (GC) XCreateGC (Dsp, Wnd, valuemask, &gcvalues);

	if ((Image = (XImage*) XCreateImage(Dsp, Vsl, DEPTH, ZPixmap, NULL, NULL, WINDOWWIDTH, WINDOWHEIGHT, QUANTUM, 0)) == NULL)
		return false;

	if ((Image->data = (char*) malloc (Image->bytes_per_line * Image->height)) == NULL)
		return false;

	memset (Image->data, 0, Image->bytes_per_line * Image->height);

	ST_Init ();
	ST_SetWindowWidth (WINDOWWIDTH);
	ST_SetWindowHeight (WINDOWHEIGHT);
	ST_SetImageBpl (Image->bytes_per_line);

	/* Pass image data pointer to render */
	R_Init (Image->data);

	XMapWindow (Dsp, Wnd);

	return true;
}

void M_Terminate () {
	XCloseDisplay (Dsp);

	if (Image)
		/* This destroys Imaga->data too because was XCreateImage() was used */
		XDestroyImage (Image);
}

void M_Draw () {
#ifdef DEBUG
	R_RendTextures ();
#else
	R_Rend ();
#endif
	XPutImage (Dsp, Wnd, Gc, Image, 0, 0, 0, 0, Image->width, Image->height);
}

void M_ProcessEvents () {
	XEvent ev;

	XNextEvent (Dsp, &ev);
	switch (ev.type) {
		case KeyPress:
			M_HandleKeyPress (ev.xkey.keycode);
			break;
		case Expose:
			break;
		default:
			break;
	}
}

void M_MainLoop () {
	int exit = 0;
	long prevTick, tick, elapsed;

	prevTick = M_GetTick ();
	while (!exit) {
		M_ProcessEvents ();
		M_Draw ();
		tick = M_GetTick ();

		/* Manage wraparound */
		if (prevTick > tick)
			elapsed = 0xffffffff - prevTick + tick;
		else
			elapsed = tick - prevTick;

		prevTick = tick;
	}
}

int main(int argc, char** argv) {
	if (!M_Init ())
		return -1;

	M_MainLoop ();
	M_Terminate ();

	return 0;
}
