/*
 * iir.c
 *
 *  Created on: 14 ott 2017
 *      Author: Tiziano
 */



// IIR filter implemented using direct I form biquads floating point arithmetic
// x: pointer to input buffer
// b: pointer to filter coefficients (numerator), size = 3*nq, organization: [b01,b11,b21, b02,b12,b22, ..., b0nq,b1nq,b2nq]
// a: pointer to filter coefficients (denominator), size = 2*nq, organization: [a11,a21, a12,a22, ..., a1nq,a2nq]
// r: poiter to output buffer
// dbuf1: pointer to input delay line (must be at least 3*nq+1 samples)
// dbuf2: pointer to output delay line (must be at least 2*nq samples)
// nx: number of samples in input buffer
// nq: number of biquads



void iir_biquad1_f(int *x, float *b, float *a, float *r, float *dbuf1, float *dbuf2, unsigned short nx, unsigned short nq)
{
	// direct form I equation
	// y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]

	int i, k;
	float acc;

	for (i = 0; i < nx; i++)
	{
		// load input sample
		dbuf1[0] = x[i];
		for (k = 0; k < nq; k++)
		{
			// compute biquad
			acc = dbuf1[k*3+0] * b[k*3+0];
			acc += dbuf1[k*3+1] * b[k*3+1];
			acc += dbuf1[k*3+2] * b[k*3+2];
			acc -= dbuf2[k*2+0] * a[k*2+0];
			acc -= dbuf2[k*2+1] * a[k*2+1];
			// shift input delay line
			dbuf1[k*3+2] = dbuf1[k*3+1];
			dbuf1[k*3+1] = dbuf1[k*3+0];
			// shift output delay line
			dbuf2[k*2+1] = dbuf2[k*2+0];
			// load new output in delay line
			dbuf2[k*2+0] = acc;
			// load new output as input of next biquad
			dbuf1[k*3+3] = acc;
		}
		// write output
		r[i] = acc;
	}
}


// IIR filter implemented using direct II form biquads floating point arithmetic
// x: pointer to input buffer
// b: pointer to filter coefficients (numerator), size = 3*nq, organization: [b01,b11,b21, b02,b12,b22, ..., b0nq,b1nq,b2nq]
// a: pointer to filter coefficients (denominator), size = 2*nq, organization: [a11,a21, a12,a22, ..., a1nq,a2nq]
// r: poiter to output buffer
// dbuf: pointer to internal delay line (must be at least 3*nq samples)
// nx: number of samples in input buffer
// nq: number of biquads



void iir_biquad2_f(int *x, float *b, float *a, float *r, float *dbuf, unsigned short nx, unsigned short nq)
{
	// direct form II equations
	// d[n] = x[n] - a1*d[n-1] - a2*d[n-2]
	// y[n] = b0*d[n] + b1*d[n-1] + b2*d[n-2]

	int i, k;
	float acc;

	for (i = 0; i < nx; i++)
	{
		// implement here direct form II biquad
		// load input sample
		acc = x[i];
		for (k = 0; k < nq; k++)
		{
		// compute internal state (backward part)
		acc -= dbuf[k*3+1] * a[k*2+0];
		acc -= dbuf[k*3+2] * a[k*2+1];
		// load new state in delay line
		dbuf[k*3+0] = acc;
		// compute output (forward part)
		acc = dbuf[k*3+0] * b[k*3+0];
		acc += dbuf[k*3+1] * b[k*3+1];
		acc += dbuf[k*3+2] * b[k*3+2];
		// shift delay line
		dbuf[k*3+2] = dbuf[k*3+1];
		dbuf[k*3+1] = dbuf[k*3+0];
		}
		// write output
		r[i] = acc;
	}
}

void iir_biquad1_fixed(int *x, int *b, int *a, int *r, int *dbuf1, int *dbuf2, unsigned short nx, unsigned short nq)
{
	// direct form I equation
	// y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]

	int i, k;
	long acc;

	for (i = 0; i < nx; i++)
	{
		// load input sample
		dbuf1[0] = x[i];
		for (k = 0; k < nq; k++)
		{
			// compute biquad
			acc = (long)dbuf1[k*3+0] * b[k*3+0];
			acc += (long)dbuf1[k*3+1] * b[k*3+1];
			acc += (long)dbuf1[k*3+2] * b[k*3+2];
			acc -= (long)dbuf2[k*2+0] * a[k*2+0];
			acc -= (long)dbuf2[k*2+1] * a[k*2+1];
			// shift input delay line
			dbuf1[k*3+2] = dbuf1[k*3+1];
			dbuf1[k*3+1] = dbuf1[k*3+0];
			// shift output delay line
			dbuf2[k*2+1] = dbuf2[k*2+0];
			// load new output in delay line
			acc=acc+(1<<(14-1)); // Acc is at 32 bits
			dbuf2[k*2+0] = (int)(acc>>14);
			// load new output as input of next biquad
			dbuf1[k*3+3] = (int)(acc>>14);
		}
		// write output
		r[i] = (int)(acc>>14);
	}
}

void iir_biquad2_fixed(int *x, int *b, int *a, int *r, int *dbuf, unsigned short nx, unsigned short nq)
{
	// direct form II equations
	// d[n] = x[n] - a1*d[n-1] - a2*d[n-2]
	// y[n] = b0*d[n] + b1*d[n-1] + b2*d[n-2]

	int i, k;
	long acc;

	for (i = 0; i < nx; i++)
	{
		// implement here direct form II biquad
		// load input sample
		acc = (long) x[i]*(1<<14);
		for (k = 0; k < nq; k++)
		{
		// compute internal state (backward part)
		acc -= (long)dbuf[k*3+1] * a[k*2+0];
		acc -= (long)dbuf[k*3+2] * a[k*2+1];
		// load new state in delay line
		acc=acc+(1<<(14-1));
		dbuf[k*3+0] = (int)(acc>>14); // we introduce a rounding error in this line
		// compute output (forward part)
		acc =(long) dbuf[k*3+0] * b[k*3+0];
		acc +=(long) dbuf[k*3+1] * b[k*3+1];
		acc +=(long) dbuf[k*3+2] * b[k*3+2];
		// shift delay line
		dbuf[k*3+2] = dbuf[k*3+1];
		dbuf[k*3+1] = dbuf[k*3+0];
		}
		// write output
		acc=acc+(1<<(14-1));
		r[i] = (int)(acc>>14);
	}
}
