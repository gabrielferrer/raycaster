/*
	project: raycaster
	file: render.c
	programming: Gabriel Ferrer
	date: 16-08-2012
*/

#include <math.h>
#include <float.h>
#include <stddef.h>
#include "sys.h"
#include "state.h"
#include "trig.h"
#include "render.h"
#include "bitmap.h"

/* Distance to projection plane */
#define PROJPLANESHIFT 9	// 2^9 = 512 (projection plane distance)
/* Wall wallheight */
#define WALLHEIGHT 64
/* Wall shift */
#define WALLHEIGHTSHIFT 6	// 2^6 = 64 (wall height)
/* Avoid a product */
#define PROJWALL (1 << PROJPLANESHIFT + WALLHEIGHTSHIFT)
/* To how many units light changes */
#define ATTENUATIONDECAYUNITSSHIFT 1

extern state_t State;
int* VidBuffer;

/*
	Attenuates a color passed as parameter.

	[IN]

		initialColor: color to attenuate.
		finalColor: attenuated color.
		attenuate: attenuation quantity.
*/
int R_Attenuate (int initialColor, int attenuation) {
	int finalColor = 0;
	int channel = initialColor & 0x000000ff;

	if (attenuation <= channel) {
		finalColor ^= channel - attenuation;
	}

	attenuation <<= 8;
	channel = initialColor & 0x0000ff00;

	if (attenuation <= channel) {
		finalColor ^= channel - attenuation;
	}

	attenuation <<= 8;
	channel = initialColor & 0x00ff0000;

	if (attenuation <= channel) {
		finalColor ^= channel - attenuation;
	}

	return finalColor;
}

/* Keep angle between right range */
int R_KeepRange (int angle) {
	if (angle < 0) {
		return angle + 3600;
	} else if (angle > 3599) {
		return angle - 3600;
	}
}

int R_Round (double x) {
	double i, f;

	f = modf (x, &i);

	if (f > .5) {
		return (int) i + 1;
	} else {
		return (int) i;
	}
}

/*
	Reder initialization.

	[IN]

		imageBuffer: pointer to image data to render.
*/
void R_Init (char* imageBuffer) {
	VidBuffer = (int*) imageBuffer;
}

#ifdef DEBUG
void R_RendTexture (bitmap_t* texture, int x, int y) {
	int i, j, vidbase, texbase;

	vidbase = y * State.windowWidth + x;
	for (i = 0, texbase = 0; i < texture->height; i++) {
		for (j = 0; j < texture->width; j++) {
			VidBuffer[vidbase + j] = texture->data[texbase + j];
		}
		vidbase += State.windowWidth;
		texbase += texture->width;
	}
}

/*
	Renders all textues for debug purposes.
*/
void R_RendTextures () {
	int i, x, y;

	for (i = 1, x = y = 0; i < MAXTEXTURES; i++) {
		R_RendTexture(&State.textures[i], x, y);

		if (x + State.textures[i]->width > State.windowWidth) {
			x = 0;
			y += State.textures[i]->height;
		} else {
			x += State.textures[i]->width;
		}
	}
}
#endif

/*
	Renders an scene by scanning all the window columns one by one.
	At each column, a ray is casted from player's position until it hits a wall.
	The distance from player's position to the hit wall is calculated and then a
	wall slice is draw depending on this distance.
	Bigger the distance, smaller the wall slice will be. Smallest the distance,
	bigger the slice wall.
*/
void R_Rend () {
	int angleint, row, col, rowS, colS, wallheight, xint, yint, ywallstart,
		corrangleint, i, texheight, acc, texindex, texrow, texcol, texcolS,
		texwidth, texs, texi, vids, vidi, midwallpixels, playerindex, cellindex,
		inc, wallindex, wallindexS, x, y, pjh, ph, sd, color, attenuation;
	float dx, dy, mindist, mindistS, delta, offset, angle, limit, intersec,
		cathetus, corrangle, ddist, xacc, yacc;
	cell_t* cell;

	/* Angle for scan begining (in tenth of degree) */
	angle = (float) (State.directorAngle - State.fov2);
	
	/* For wall distance correction avoiding eye-fish efect */
	corrangle = (float) - State.fov2;

	/* Scan every window column casting a ray from player's position until a wall is hit */
	for (xint = 0; xint < State.windowWidth; xint++) {

		mindist = mindistS = FLT_MAX;
		/* Angle rounded to the nearest available tenth of degree */
		angleint = R_Round (angle);
		corrangleint = R_Round (corrangle);

		angleint = R_KeepRange (angleint);
		corrangleint = R_KeepRange (corrangleint);

		if (COS (angleint) != 0.0) {

			/* Player's position column. */
			playerindex = (int) State.position.x >> CELLSHIFT;

			/* Check player heading left/right */
			if (COS (angleint) > 0.0) {
				/* Heading right, the limit will be between player's column and the
					one to the riht */
				limit = (float) (playerindex + 1 << CELLSHIFT);
				/* Column next to the player heading in right direction */
				col = playerindex + 1;
				/* Offset between limits is constant */
				offset = CELLSIZE;
				/* Calculate the next column. Sum one to current column */
				inc = 1;
				/* Heading a western wall */
				wallindex = W_WEST;
			} else if (COS (angleint) < 0.0) {
				/* Heading left, same as heading to the right but change the
					signs */
				limit = playerindex << CELLSHIFT;
				col = playerindex - 1;
				offset = -CELLSIZE;
				inc = -1;
				/* Heading a eastern wall */
				wallindex = W_EAST;
			}

			/* Special cases angleint=0, 180 degrees */
			if (fabs (COS (angleint)) == 1.0) {
				row = (int) State.position.y >> CELLSHIFT;

				while (mindist == FLT_MAX) {
					cell = ST_GetCell (row, col);

					if (cell->walls != NULL) {
						mindist = fabs (State.position.x - limit);
					} else {
						limit += offset;
						col += inc;
					}
				}
			} else {
				/* Distance from player 'x' coordinate to the limit between
					player's column and next column. Note cathetus can be
					negative */
				cathetus = limit - State.position.x;
				/* Intersection at 'y' coordinate and the previous limit */
				intersec = State.position.y + cathetus * TAN (angleint);
				/* Row and column where to check for a collision with a wall */
				row = (int) intersec >> CELLSHIFT;
				/* Next intersection will be at constant rate */
				delta = offset * TAN (angleint);
				/* Continue checking cells until a wall is hit */

				while (mindist == FLT_MAX) {
					/* Advance column by column, left or right. So, calculated
						row can be outside map */
					if (row < 0 || row >= State.rows) {
						break;
					}

					/* Check for a collision */
					cell = ST_GetCell (row, col);
					if (cell->walls != NULL) {
						dx = State.position.x - limit;
						dy = State.position.y - intersec;
						texcol = (int) intersec % CELLSIZE;
						/* Where a wall is hit calculate the distance from
							player's position to that wall */
						mindist = sqrt(dx * dx + dy * dy);
					} else {
						/* Go to next two columns limit */
						limit += offset;
						/* Next column */
						col += inc;
						/* Next intersection */
						intersec += delta;
						/* Next cell for collision with wall */
						row = (int) intersec >> CELLSHIFT;
					}
				}
			}
		}

		if (SIN (angleint) != 0.0) {
			/* Player's position row */
			playerindex = (int) State.position.y >> CELLSHIFT;

			/* Check player heading up/down */
			if (SIN (angleint) > 0.0) {
				/* Heading up, the limit will be between player's row and the one
					above it */
				limit = (float) (playerindex + 1 << CELLSHIFT);
				offset = CELLSIZE;
				/* Row next to the player heading up */
				rowS = playerindex + 1;
				/* Calculate the next row. Sum one to current column */
				inc = 1;
				/* Heading a northern wall */
				wallindexS = W_NORTH;
			} else if (SIN (angleint) < 0.0) {
				/* Heading down, same as heading up but change the signs */
				limit = playerindex << CELLSHIFT;
				offset = -CELLSIZE;
				rowS = playerindex - 1;
				inc = -1;
				/* Heading a southern wall */
				wallindexS = W_SOUTH;
			}

			if (fabs (SIN (angleint)) == 1.0) {
				colS = (int) State.position.x >> CELLSHIFT;

				while (mindistS == FLT_MAX) {
					cell = ST_GetCell (rowS, colS);

					if (cell->walls != NULL) {
						mindistS = fabs (State.position.y - limit);
					} else {
						limit += offset;
						rowS += inc;
					}
				}
			} else {
				/* Distance from player 'y' coordinate to the limit between
					player's row and next row. Note cathetus can be negative */
				cathetus = limit - State.position.y;
				/* Intersection at 'x' coordinate and the previous limit */
				intersec = State.position.x + cathetus / TAN(angleint);
				colS = (int) intersec >> CELLSHIFT;
				/* Now calculate distance at 'y' steps */
				delta = offset / TAN (angleint);

				while (mindistS == FLT_MAX) {
					/* Advance row by row, up or down. So, calculated column can be outside map */
					if (colS < 0 || colS >= State.cols) {
						break;
					}

					cell = ST_GetCell (rowS, colS);
					if (cell->walls != NULL) {
						dx = State.position.x - intersec;
						dy = State.position.y - limit;
						texcolS = (int) intersec % CELLSIZE;
						mindistS = sqrt (dx * dx + dy * dy);
					} else {
						/* Go to next two rows limit */
						limit += offset;
						/* Next column */
						rowS += inc;
						/* Next intersection */
						intersec += delta;
						/* Next cell for collision with wall */
						colS = (int) intersec >> CELLSHIFT;
					}
				}
			}
		}

		/* Get minimum distance, row and column coincident with this distance
		*/
		if (mindistS < mindist) {
			mindist = mindistS;
			texcol = texcolS;
			row = rowS;
			col = colS;
			wallindex = wallindexS;
		}

		cell = ST_GetCell (row, col);
		/* Texture index, width and height */
		texindex = cell->walls[wallindex].index;
		texwidth = State.textures[texindex]->width;
		texheight = State.textures[texindex]->height;

		/* Get wall slice height. Multiply by cos(angle) avoiding eye-fish efect */
		wallheight = PROJWALL / R_Round(mindist * fabs (COS (corrangleint)));

		/* Make sure 'wallheight' is even. This asures the same pixel amount for ceil and floor */
		if (wallheight & 0x00000001 != 0) {
			wallheight++;
		}

		/* Check if wall slice wallheight is bigger than window height */
		if (wallheight > State.windowHeight) {
			ywallstart = 0;
			/* Half window height */
			midwallpixels = State.windowHeight >> 1;
		} else {
			ywallstart = State.windowHeight - wallheight >> 1;
			/* Half wall height */
			midwallpixels = wallheight >> 1;
		}

		// Calculate attenuation.
		attenuation = (int) mindist >> ATTENUATIONDECAYUNITSSHIFT;

		// Attenuation can't be greater than the maximum RGB channel value.
		if (attenuation > 255) {
			attenuation = 255;
		}

		/* Set 'vidi' and 'vids' at window midpoint, 'texi' and 'texs' at
			texture midpoint.
			'vidi' and 'texi' covers window and texture lower half repectively,
			'vids' and 'texs' covers window and texture upper half
			repectively */
		vidi = (State.windowHeight >> 1) * State.windowWidth + xint;
		vids = vidi - State.windowWidth;
		texi = (texheight >> 1) * texwidth + texcol;
		texs = texi - texwidth;
		/* Draw textured walls */
		if (texheight > wallheight) {
			acc = texheight;

			for (i = 0; i < midwallpixels; acc += wallheight) {
				if (acc >= texheight) {
					VidBuffer[vids] = R_Attenuate (State.textures[texindex]->data[texs], attenuation);
					VidBuffer[vidi] = R_Attenuate (State.textures[texindex]->data[texi], attenuation);
					vids -= State.windowWidth;
					vidi += State.windowWidth;
					acc -= texheight;
					i++;
				}
				texs -= texwidth;
				texi += texwidth;
			}

		} else if (wallheight > texheight) {
			acc = 0;

			/* 'acc' value depends on 'wallheight' > window height */
			for (i = 0; i < midwallpixels; i++, acc += texheight) {
				if (acc >= wallheight) {
					texs -= texwidth;
					texi += texwidth;
					acc -= wallheight;
				}
				VidBuffer[vids] = R_Attenuate (State.textures[texindex]->data[texs], attenuation);
				VidBuffer[vidi] = R_Attenuate (State.textures[texindex]->data[texi], attenuation);
				vids -= State.windowWidth;
				vidi += State.windowWidth;
			}

		} else
			for (i = 0; i < midwallpixels; i++) {
				VidBuffer[vids] = R_Attenuate (State.textures[texindex]->data[texs], attenuation);
				VidBuffer[vidi] = R_Attenuate (State.textures[texindex]->data[texi], attenuation);		
				vids -= State.windowWidth;
				vids += State.windowWidth;
				texs -= texwidth;
				texi += texwidth;
			}

		/* Draw ceil and floor in one pass */
		ph = WALLHEIGHT >> 1;
		vids = (ywallstart - 1) * State.windowWidth + xint;
		vidi = (ywallstart + wallheight) * State.windowWidth + xint;
		for (i = 0; i < ywallstart; i++) {
			pjh = midwallpixels + i + 1;
			sd = (ph << PROJPLANESHIFT) / pjh;
			ddist = (float) sd / fabs (COS (corrangleint));
			attenuation = (int) ddist >> ATTENUATIONDECAYUNITSSHIFT;
			if (attenuation > 255) {
				attenuation = 255;
			}
			dx = ddist * COS (angleint);
			dy = ddist * SIN (angleint);
			x = (int) (State.position.x + dx);
			y = (int) (State.position.y + dy);
			row = y >> CELLSHIFT;
			col = x >> CELLSHIFT;
			texcol = x % CELLSIZE;
			texrow = y % CELLSIZE;
			cell = ST_GetCell (row, col);
			texs = texrow * texwidth + texcol;
			VidBuffer[vids] = R_Attenuate (State.textures[cell->ceilindex]->data[texs], attenuation);
			VidBuffer[vidi] = R_Attenuate (State.textures[cell->floorindex]->data[texs], attenuation);
			vids -= State.windowWidth;
			vidi += State.windowWidth;
		}

		angle += State.deltaAngle;
		corrangle += State.deltaAngle;
	}
}
