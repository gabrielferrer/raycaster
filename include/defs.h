/*
	project: raycaster
	file: defs.h
	programming: Gabriel Ferrer
	date: 16-08-2012
*/

#ifndef DEFS_H
#define DEFS_H

#define false 0x00
#define true 0xff

/* Walls per cell */
#define WALLCOUNT 4

/* Wall flags */
#define WALLTRANSPARENT		// transparent
#define WALLTHROUGH			// can go through the wall

typedef unsigned char bool_t;

/* A point into XY plane. */
typedef struct point_s {
	float x;
	float y;
} point_t, vector_t, vertex_t;

/* A wall. A texture index and wall flags */
typedef struct wall_s {
	int index;
	int flags;
} wall_t;

/* Wall names */
typedef enum {W_SOUTH, W_WEST, W_NORTH, W_EAST} wallname_t;

/* A cell into the map. Contains a ceil and floor index and optionally
	the four wall's texture indexes */
typedef struct cell_s {
	int ceilindex;
	int floorindex;
	wall_t *walls;
} cell_t;

#endif
