/*
	project: raycaster
	file: state.h
	programming: Gabriel Ferrer
	date: 16-08-2012
*/

#ifndef STATE_H
#define STATE_H

#include "defs.h"
#include "bitmap.h"

/* Cell's width and height */
#define CELLSIZE 64
/* Because cell size is 64x64. 64 = 2^6 */
#define CELLSHIFT 6
/* Number of textures */
#define MAXTEXTURES 13

typedef struct state_s {
	/* Map's rows and columns */
	int rows;
	int cols;
	/* Maximum coordinates X and Y. It depends on map size */
	float maxX;
	float maxY;
	/* Indicates where the player is pointing. It is measured in tenths of a
	degree (35 degrees = 350 tenth of a degree) */
	int directorAngle;
	/* 	Player position in XY plane */
	point_t position;
	/* Player's speed */
	float speed;
	/* Distance amount the player moves. It varies with speed */
	float deltaDistance;
	/* Amount added to the angle when casting rays in the rendering scan. It
	must to be a float because if it were an integer accumulated errors will
	ocurr */
	float deltaAngle;
	/* Field of view (all angles in tenths of degree */
	int fov;
	/* fov/2 */
	int fov2;
	/* Window width */
	int windowWidth;
	/* Window height */
	int windowHeight;
	/* Image buffer bytes per line */
	int imageBpl;
	/*
		A map is represented by cells.
		A cell with a value greater than zero is a wall and the value indicates
		the wall's color (later it will be a texture).
		Later there may be added texture mapping so the cell value will mean an
		index into a texture buffer.
		A cell with a value equal to zero means empty space
	*/
	cell_t* map;
	/* Texture with index zero doesn't exist. That's because index zero
		indicate not a wall */
	bitmap_t *textures[MAXTEXTURES];
} state_t;

void ST_Init ();
void ST_SetWindowWidth (int windowWidth);
void ST_SetWindowHeight (int windowHeight);
cell_t* ST_GetCell (int row, int col);
void ST_CalcPosition (float advance);
void ST_CalcDirection (int direction);
void ST_SetImageBpl (int bpl);

#endif
