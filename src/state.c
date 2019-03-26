/*
	project: raycaster
	file: state.c
	programming: Gabriel Ferrer
	date: 16-08-2012
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys.h"
#include "bitmap.h"
#include "trig.h"
#include "state.h"
#include "errormsg.h"

/* How much to turn (left or right) when a turn key is pressed. Measured in
tenths of degree */
#define TURNANGLE 100
/* How much to advance */
#define ADVANCEAMOUNT 2.5f
/* Temporary buffer size */
#define SIZE 1024
/* Textures path */
#define TEXTURESPATH "textures"

const char* TextureFileNames[] = {
	"brick", "concrete", "crumble_brick", "dirt", "door", "floor", "metal",
	"metal_blackbarrel", "rock", "roof", "stone", "wallpaper", "wood"
};

/* Program state */
state_t State;

/*
	Load map helper function.
	Reads a string representing an integer from the map file and returns it
	as an integer value.
*/
int ST_ReadInt (FILE* mapfile) {
	char s[SIZE];

	if (fgets (s, SIZE, mapfile) == NULL) {
		SYS_Exit ("ST_ReadInt", READMAPERROR);
	}

	return atoi (s);
}

/*
	Load map helper function.
	Reads a string representing a float from the map file and returns it
	as a float value.
*/
float ST_ReadFloat (FILE* mapfile) {
	char s[SIZE];

	if (fgets (s, SIZE, mapfile) == NULL) {
		SYS_Exit ("ST_ReadFloat", READMAPERROR);
	}

	return atof (s);
}

char* ST_GetToken (char** line, bool_t* recordEnd) {
	char* p, * q = *line;

	*recordEnd = false;
	while (*q != '\0') {
		if (*q == ',' || *q == '}') {
			if (*q == '}') {
				*recordEnd = true;
			}
			*q = '\0';
			p = *line;
			*line = q + 1;
			return p;
		}
		q++;
	}

	SYS_Exit ("ST_GetToken", TOKENMAPERROR);
}

char* ST_GetRecordMark (char** line, char mark, char wrongMark, int row, int col) {
	char* q = *line;

	while (*q != '\0') {
		if (*q == wrongMark) {
			SYS_ExitF ("ST_GetRecordMark", RECORDMAPWRONGMARKERROR, mark, wrongMark, row, col);
		}

		if (*q == mark) {;
			*line = q + 1;
			return q;
		}
		q++;
	}

	SYS_ExitF ("ST_GetRecordMark", RECORDMAPNOTFOUNDERROR, mark);
}

void ST_ReadWalls (int index, char** line, int row, int col) {
	bool_t recordEnd;
	int i;

	if (ST_GetRecordMark (line, '{', '}', row, col) != NULL) {
		if ((State.map[index].walls = (wall_t*) malloc (WALLCOUNT * sizeof (wall_t))) == NULL) {
			SYS_Exit ("ST_ReadWalls", READMAPERROR);
		}

		for (i = 0; i < WALLCOUNT; i++) {
			State.map[index].walls[i].index = atoi (ST_GetToken (line, &recordEnd));
			State.map[index].walls[i].flags = 0;
		}
	}
}

void ST_ReadCell (int index, char** line, int row, int col) {
	bool_t recordEnd;

	ST_GetRecordMark (line, '{', '}', row, col);
	State.map[index].floorindex = atoi (ST_GetToken (line, &recordEnd));
	State.map[index].ceilindex = atoi (ST_GetToken (line, &recordEnd));

	if (!recordEnd) {
		ST_ReadWalls (index, line, row, col);
		ST_GetRecordMark (line, '}', '{', row, col);
	}
}

/*
	Load map helper function.
	Reads a map row form the map file and stores it in memory.
*/
void ST_ReadRow (int row, FILE* mapfile) {
	char s[SIZE], * line = s;
	int base, col;

	if (fgets (s, SIZE, mapfile) == NULL) {
		SYS_Exit ("ST_ReadRow", READMAPERROR);
	}

	for (base = row * State.cols, col = 0; col < State.cols; col++) {
		ST_ReadCell (base + col, &line, row, col);
	}
}

/*
	Loads a default map.
*/
void ST_LoadMap () {
	FILE* mapfile;
	int row;

	if ((mapfile = fopen ("map", "r")) == NULL) {
		SYS_Exit ("ST_LoadMap", OPENMAPERROR);
	}

	/* Map rows and columns */
	State.rows = ST_ReadInt (mapfile);
	State.cols = ST_ReadInt (mapfile);
	/* Maximum X and Y coordinates */
	State.maxX = CELLSIZE * State.cols;
	State.maxY = CELLSIZE * State.rows;
	/* Position depends on map definition. */
	State.position.x = ST_ReadFloat (mapfile);
	State.position.y = ST_ReadFloat (mapfile);

	if ((State.map = (cell_t*) malloc (State.rows * State.cols * sizeof (cell_t))) == NULL) {
		SYS_Exit ("ST_LoadMap", ALLOCERROR);
	}

	for (row = 0; row < State.rows; row++) {
		ST_ReadRow(row, mapfile);
	}

	fclose (mapfile);
}

#ifdef DEBUG
void ST_WriteMap () {
	FILE* file;
	int i, j;

	if ((file = fopen ("memory-map", "w")) == NULL) {
		return;
	}

	for (i = 0; i < State.rows; i++) {
		for (j = 0; j < State.cols - 1; j++) {
			fprintf (file, "%u,", State.map[i * State.cols + j]);
		}
		fprintf (file, "%u\n", State.map[i * State.cols + j]);
	}

	fclose (file);
}
#endif

/*
	Initializes program's state.
*/
void ST_Init () {
	int i;

	ST_LoadMap ();
#ifdef DEBUG
	ST_WriteMap ();
#endif
	for (i = 0; i < MAXTEXTURES; i++) {
		State.textures[i] = BMP_LoadBitmap (TEXTURESPATH, TextureFileNames[i]);
	}

	State.directorAngle = 0;
	State.speed = 0.0;
	State.deltaDistance = 0.5;
	State.fov = 450;
	State.fov2 = State.fov / 2;	
}

/*
	Changes the window width.
	
	[IN]
	
		windowWidth: the new width for the window.
*/
void ST_SetWindowWidth (int windowWidth) {
	State.windowWidth = windowWidth;
	/*
	The amount added to the angle when scanning depends on the window width.
	*/
	State.deltaAngle = (float) State.fov / (float) State.windowWidth;
}

/*
	Changes the window height.
	
	[IN]
	
		windowHeight: the new height for the window.
*/
void ST_SetWindowHeight (int windowHeight) {
	State.windowHeight = windowHeight;
}

cell_t* ST_GetCell (int row, int col) {
	return &State.map[row * State.cols + col];
}

/*
	Calculate new player's position.
	
	[IN]
	
		advance: a value that can be 1.0 (advance foward) or -1.0 (advance
			backward).
*/
void ST_CalcPosition (float advance) {
	float newx, newy;
	int row, col;
	cell_t* cell;

	/* New position is calculated along the director angle */
	newx = State.position.x + COS (State.directorAngle) * advance * ADVANCEAMOUNT;
	newy = State.position.y + SIN (State.directorAngle) * advance * ADVANCEAMOUNT;
	/* Current row and column */
	row = State.position.y / CELLSIZE;
	col = State.position.x / CELLSIZE;

	if (newx > 0 && newx < State.maxX) {
		col = (int) (newx / CELLSIZE);

		cell = ST_GetCell (row, col);
		if (cell->walls == NULL) {
			State.position.x = newx;
		}
	}

	if (newy > 0 && newy < State.maxY) {
		row = (int) (newy / CELLSIZE);

		cell = ST_GetCell (row, col);
		if (cell->walls == NULL) {
			State.position.y = newy;
		}
	}
}

/*
	Calculate new player's direction.
	
	[IN]
	
		direction: a value that can be 1 (turn right) or -1 (turn left).
*/
void ST_CalcDirection (int direction) {
	State.directorAngle += TURNANGLE * direction;

	if (State.directorAngle < 0) {
		State.directorAngle += 3600;
	}

	if (State.directorAngle > 3599) {
		State.directorAngle -= 3600;
	}
}

void ST_SetImageBpl (int bpl) {
	State.imageBpl = bpl;
}
