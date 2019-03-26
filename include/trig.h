#ifndef TRIG_H
#define TRIG_H

typedef struct trig_s {
	float sin;
	float cos;
	float tan;
} trig_t;

extern trig_t TRIGTABLE[];

#define SIN(angle) (TRIGTABLE[angle].sin)
#define COS(angle) (TRIGTABLE[angle].cos)
#define TAN(angle) (TRIGTABLE[angle].tan)

#endif
