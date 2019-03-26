#include <stdio.h>
#include <math.h>

#define FILENAME "trig.txt"
#define PI 3.14159265359
#define DELTAANGLE PI/1800.0

int main(int argc, char**argv) {
	FILE *f;
	int a;
	double angle;

	if ((f=fopen(FILENAME, "w"))==NULL)
		return -1;

	for (a=0, angle=0.0; a<3599; a++, angle+=DELTAANGLE)
		fprintf(f, "{%f, %f, %f},\n", sin(angle), cos(angle), tan(angle));
	fprintf(f, "{%f, %f, %f}\n", sin(angle), cos(angle), tan(angle));

	fclose(f);

	return 0;
}

