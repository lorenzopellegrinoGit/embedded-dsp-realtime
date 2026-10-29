/*
 * rice_codec.c
 *
 *  Created on: 29 set 2016
 *      Author: Tiziano
 */

/*
 * define macros for efficiency
 * code is substituted when compiling instead of generating function call
 */

#define PACK_BITS(value, nbits, bufptr, packed_bits, bits_left) \
{ \
	bits_left -= nbits; \
	packed_bits |= (unsigned long)(value) << bits_left; \
	if (bits_left <= 16) \
	{ \
		unsigned long tmp; \
		tmp = packed_bits >> 16; \
		*bufptr++ = tmp; \
		packed_bits <<= 16; \
		bits_left += 16; \
	} \
}


#define UNPACK_BITS(value, nbits, bufptr, packed_bits, bits_left) \
{ \
	value = packed_bits >> (32 - nbits); \
	packed_bits <<= nbits; \
	bits_left -= nbits; \
	if (bits_left <= 16) \
	{ \
		unsigned long tmp = *bufptr++; \
		packed_bits |= tmp << (16 - bits_left); \
		bits_left += 16; \
	} \
}




/*
 * compute Rice-Golomb encoding of samples in data
 * data: pointer to data buffer
 * stream_out: pointer to encoded data (assume enough space is allocated)
 * n_data: number of samples to encode
 * 	k_rice: k parameter of Rice code
 * 	WARNING: in C55xx a byte is 16 bits!
 */
long rice_encoder(int *data, unsigned int *stream_out, int n_data, int k_rice)
{
	unsigned long pad = 0;
	int free = 32;
	unsigned long encoded_bits = 0;
	int i=0,s,q,r, L, sample;


	int mask = 0xffff >> (16-k_rice);
	while (i < n_data)
	{
		sample = data[i];

		if (sample >= 0)
			s = 0;
		else
			{s=1;
			sample=-sample;
			}

		q = sample >> k_rice;
		r = sample & mask;

		PACK_BITS(s, 1, stream_out, pad, free)
		encoded_bits ++;

		L = q+1;
		while(L>16)
		{
			PACK_BITS(0,16, stream_out, pad, free);
			L -= 16;
			encoded_bits += 16;
		}
		PACK_BITS(1, L, stream_out,pad,free);
		encoded_bits += L;
		PACK_BITS(r, k_rice, stream_out, pad, free);
		encoded_bits += k_rice;
		i++;
	}
	PACK_BITS(0,16, stream_out, pad, free); // dopo aver codificato 8192 campioni aggiungo 16 bit extra
	return encoded_bits;
}


void rice_decoder(unsigned int *stream_in, int *data, int n_data, int k_rice)
{
	unsigned long pad = 0;
	int free = 32;
	int i=0,s,q,r,bit;

	// initialize pad register with first 32 bits in stream_in
	pad  = *stream_in++;
	pad <<= 16;
	pad |= (unsigned long)*stream_in++;

	while (i < n_data)
	{
		// decode sign bit
		UNPACK_BITS(bit, 1, stream_in, pad, free)
		s = (bit) ? -1 : 1;
		// decode unary code
		q = -1;
		bit = 0;
		while (bit == 0)
		{
			UNPACK_BITS(bit, 1, stream_in, pad, free)
			q++;
		}
		// decode last k_rice bits
		UNPACK_BITS(r, k_rice, stream_in, pad, free)
		// reconstruct sample
		data[i] = s*((q << k_rice)+r);
		i++;
	}
}


/*
 * compute optimal parameter k for Golomb-Rice code
 * use k \approx \log_2 ( Mean[Abs[data]] )
 */
unsigned int rice_parameter(int *data, int n_data)
{
	int i;
	unsigned long acc = 0;
	unsigned int M,k;

	for (i = 0; i < n_data; i++)
	{
		acc += (data[i] >= 0) ? data[i] : -data[i];
	}

	M = acc/n_data;
	k = 15;
	while (!(M & 0x8000) && k > 0)
	{
		k--;
		M <<= 1;
	}
	return k;
}

