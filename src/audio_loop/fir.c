/*
 * fir.c
 *
 *  Created on: 12 ott 2017
 *      Author: Tiziano
 */


// FIR filter implemented using linear buffer and floating point arithmetic
// x: pointer to input buffer
// h: pointer to filter coefficients
// r: pointer to output buffer
// dbuffer: pointer to internal delay line (must be at least nh samples)
// nx: number of samples in input buffer
// nh: number of filter coefficients



void fir_linear_f(int *x, float *h, float *r, float *dbuffer, unsigned short nx, unsigned short nh) //all data are floating point except for input that is always integer
{
	int i,j;
	float acc;

	for (i = 0; i < nx; i++) // filter implementation pag. 21
	{
		// load new sample in delay line
		dbuffer[0] = x[i];
		// initialize accumulator with first tap product
		acc = dbuffer[0] * h[0];
		// rest of cycle
		for (j = 1; j < nh; j++)
		{
			acc += dbuffer[j] * h[j];
		}
		for (j = nh-2; j >= 0; j--)
		{
			// shift delay line
			dbuffer[j+1] = dbuffer[j];
		}
		// write output
		r[i] = acc;
	}
}


// FIR filter implemented using circular buffer
// x: pointer to input buffer
// h: pointer to filter coefficients
// r: poiter to output buffer
// dbuffer: pointer to internal circular buffer (must be at least nh+1 samples)
//		dbuffer[0...nh-1]: stores actual circular buffer
//		dbuffer[nh]: store value of circular pointer
// nx: number of samples in input buffer
// nh: number of filter coefficients


void fir_circular_f(int *x, float *h, float *r, float *dbuffer, unsigned short nx, unsigned short nh) // implementazione pag. 24
{
	int i,j,k,P;
	float acc;

	// recover initial value of circular pointer
	// this implementation assumes that dbuffer has one extra location (nh + 1 locations)
	// the last location is used to store the value of the circular pointer
	P = dbuffer[nh];
	for (i = 0; i < nx; i++)
	{
		// implement here FIR filter
		// load new sample in delay buffer
		dbuffer[P] = x[i];
		// initialize accumulator with first tap product
		acc = dbuffer[P] * h[0];
		// rest of cycle
		for (j = 1, k=P+1; k<nh; j++, k++)
		{
		acc += dbuffer[k] * h[j];
		}
		for (k=0; k<P; j++, k++)
		{
			acc+=dbuffer[k]*h[j];
		}
			if (--P<0) P=nh-1;
		// shift delay buffer
		// write output
		r[i] = acc;
	}
	// store last value of circular pointer
	dbuffer[nh] = P;
}


void fir_linear_fixed(int *x, int *h, int *r, int *dbuffer, unsigned short nx, unsigned short nh) //all data are floating point except for input that is always integer
{
	int i,j;
	long acc;

	for (i = 0; i < nx; i++) // filter implementation pag. 21
	{
		// load new sample in delay line
		dbuffer[0] = x[i];
		// initialize accumulator with first tap product
		acc = (long)dbuffer[0] * h[0];
		// rest of cycle
		for (j = 1; j < nh; j++)
		{
			acc += (long)dbuffer[j] * h[j];
		}
		for (j = nh-2; j >= 0; j--)
		{
			// shift delay line
			dbuffer[j+1] = dbuffer[j];
		}
		// write output
		acc=acc+(1<<14);
		r[i] = (int)(acc>>15);
	}
}


void fir_circular_fixed(int *x, int *h, int *r, int *dbuffer, unsigned short nx, unsigned short nh) // implementazione pag. 24
{
	int i,j,k,P;
	long acc;

	// recover initial value of circular pointer
	// this implementation assumes that dbuffer has one extra location (nh + 1 locations)
	// the last location is used to store the value of the circular pointer
	P = dbuffer[nh];
	for (i = 0; i < nx; i++)
	{
		// implement here FIR filter
		// load new sample in delay buffer
		dbuffer[P] = x[i];
		// initialize accumulator with first tap product
		acc = (long)dbuffer[P] * h[0];
		// rest of cycle
		for (j = 1, k=P+1; k<nh; j++, k++)
		{
		acc += (long)dbuffer[k] * h[j];
		}
		for (k=0; k<P; j++, k++)
		{
			acc+=(long)dbuffer[k]*h[j];
		}
			if (--P<0) P=nh-1;
		// shift delay buffer
		// write output
		acc=acc+(1<<14);
		r[i] = (int)(acc>>15);
	}
	// store last value of circular pointer

	dbuffer[nh] = P;
}
