//////////////////////////////////////////////////////////////////////////////
// * File name: main_bypass1.c
// *                                                                          
// * Description: This file includes main() and system initialization funcitons.
// *                                                                          
// * Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/ 
// *                                                                          
// *                                                                          
// *  Redistribution and use in source and binary forms, with or without      
// *  modification, are permitted provided that the following conditions      
// *  are met:                                                                
// *                                                                          
// *    Redistributions of source code must retain the above copyright        
// *    notice, this list of conditions and the following disclaimer.         
// *                                                                          
// *    Redistributions in binary form must reproduce the above copyright     
// *    notice, this list of conditions and the following disclaimer in the   
// *    documentation and/or other materials provided with the                
// *    distribution.                                                         
// *                                                                          
// *    Neither the name of Texas Instruments Incorporated nor the names of   
// *    its contributors may be used to endorse or promote products derived   
// *    from this software without specific prior written permission.         
// *                                                                          
// *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS     
// *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT       
// *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR   
// *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT    
// *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,   
// *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT        
// *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,   
// *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY   
// *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT     
// *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE   
// *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    
// *                                                                          
//////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "data_types.h"
#include "register_system.h"
#include "register_cpu.h"
#include "rtc.h"
#include "control.h"
#include "i2s_bypass1.h"
#include "dma_bypass1.h"
#include "ref_data_bypass.h"
#include "fir.h"
// #include "fir_coeffs.h"
#include "iir.h"
// #include "iir_coeffs.h"
#include "LP_coeffs2.h"


void InitSystem(void);
void ConfigPort(void);
void SYS_GlobalIntEnable(void);
void SYS_GlobalIntDisable(void);
void PLL_98MHz(void);
void buff_copy(int *input, int *output, unsigned short size);
void zero_out(int *output, unsigned short size);
void delay_subtract(int *x1, int *x2, int *r, int *dbuffer, unsigned short nx, unsigned short delay);

void switchLED(void);
//void turnOnLED(void);
//void turnOffLED(void);
//void BlinkLED(void);


// this flag is set by RTC interrupt routine
extern Uint16 Flag_RTC;


extern void AIC3254_init(void);

extern Uint16 CurrentRxL_DMAChannel;
extern Uint16 CurrentRxR_DMAChannel;
extern Uint16 CurrentTxL_DMAChannel;
extern Uint16 CurrentTxR_DMAChannel;
extern Uint16 RunFilterForL;
extern Uint16 RunFilterForR;


unsigned long samples_per_sec = 0;


#define MAX_DELAY 4096
#define BUFF_LEN 256



int enable_loop = 1;
int enable_delay = 0;
unsigned int delay = MAX_DELAY;
int delay_bufL[MAX_DELAY+1];
int delay_bufR[MAX_DELAY+1];

// allocate buffers here
int buffer_fir_R[FIR_LEN];
int buffer_fir_L[FIR_LEN];
int buffer_firc_R[FIR_LEN+1];
int buffer_firc_L[FIR_LEN+1];
int fixedbuffer2[3*BIQ_LEN+1];
int fixedbuffer1[3*BIQ_LEN+1];
int fixedbuffer3[3*BIQ_LEN+1];
int fixedbuffer4[3*BIQ_LEN+1];

// define pointers to buffers
int *pRcvL1 = (int *)&RcvL1[0];
int *pRcvL2 = (int *)&RcvL2[0];
int *pRcvR1 = (int *)&RcvR1[0];
int *pRcvR2 = (int *)&RcvR2[0];

int *pOutL1 = (int *)&OutL1[0];
int *pOutL2 = (int *)&OutL2[0];
int *pOutR1 = (int *)&OutR1[0];
int *pOutR2 = (int *)&OutR2[0];


void main(void)
{
    int i;
    unsigned long tmp = 0;
    

    InitSystem();
    ConfigPort();

    SYS_GlobalIntEnable();
	reset_RTC();
    
    IER0 = 0x0110;      // enable dma, timer int      
    IER1 = 0x0004;      // enable RTC int



    setDMA_address();
    // I2S is set as slave, AIC3254 is set as master
    set_i2s0_slave();
    AIC3254_init();


	enable_i2s0();
    enable_dma_int();
    set_dma0_ch0_i2s0_Lout();
    set_dma0_ch1_i2s0_Rout();
    set_dma0_ch2_i2s0_Lin();
	set_dma0_ch3_i2s0_Rin();
	enable_rtc_second_int();

	// zero out buffers
	for (i = 0; i < XMIT_BUFF_SIZE; i++) // Buffer is 256 long
	{
		OutL1[i] = 0;
		OutL2[i] = 0;
		OutR1[i] = 0;
		OutR2[i] = 0;
	}
	for (i = 0; i < MAX_DELAY; i++)
	{
		delay_bufL[i] = 0;
		delay_bufR[i] = 0; // initialize buffers here
	}
	for( i=0;i< FIR_LEN; i++)
	{
		buffer_fir_L[i]=0;
		buffer_fir_R[i]=0;
	}
	for( i=0;i< FIR_LEN+1; i++)
	{
		buffer_firc_L[i]=0;
		buffer_firc_R[i]=0;
	}
	for (i = 0; i < 3*BIQ_LEN+1; i++)
		{
			fixedbuffer1[i] = 0;
			fixedbuffer2[i] = 0;
			fixedbuffer3[i] = 0;
			fixedbuffer4[i] = 0;
		}


	i = 1;

    while(i)
    {
    	// process left channel
		if(RunFilterForL ==1)
		{
			RunFilterForL =0;
			// check which buffer is ready for processing
			if (CurrentRxL_DMAChannel ==2) //means that ch 1 is ready to be processed
			{
				// insert here the code to process RcvL1 buffer
				// the code below simply copies the content of RcvL1 buffer to OutL1 buffer
				if (enable_loop)
				{
					buff_copy(pRcvL1, pOutL1, XMIT_BUFF_SIZE);
				}
				else
				{
					//zero_out(pOutL1, XMIT_BUFF_SIZE);
					// fir_linear_fixed(pRcvL1,h,pOutL1,buffer_fir_L, XMIT_BUFF_SIZE, FIR_LEN);
					// fir_circular_fixed(pRcvL1,h,pOutL1,buffer_firc_L, XMIT_BUFF_SIZE , FIR_LEN);
		//	iir_biquad1_fixed(pRcvL1, b, a, pOutL1, fixedbuffer1, fixedbuffer2, BUFF_LEN, BIQ_LEN);
					iir_biquad2_fixed(pRcvL1, b, a, pOutL1, fixedbuffer1, BUFF_LEN, BIQ_LEN);
				}
				// if enable_delay is true, a delay copy of RcvL1 is subtracted from OutL1 and the result saved in OutL1
				if (enable_delay) delay_subtract(pOutL1, pRcvL1, pOutL1, delay_bufL, XMIT_BUFF_SIZE, delay);
			}
			else // if current DMA channel is 1, ch2 is ready to be processed
			{
				// insert here the code to process RcvL2 buffer
				// the code below simply copies the content of RcvL2 buffer to OutL2 buffer
				if (enable_loop)
				{
					buff_copy(pRcvL2, pOutL2, XMIT_BUFF_SIZE);
				}
				else
				{
					// zero_out(pOutL2, XMIT_BUFF_SIZE); // zero out the content of the output buffer, silence as output
					// fir_linear_fixed(pRcvL2,h,pOutL2,buffer_fir_L, XMIT_BUFF_SIZE, FIR_LEN);
					// fir_circular_fixed(pRcvL2,h,pOutL2,buffer_firc_L, XMIT_BUFF_SIZE , FIR_LEN);
		//	iir_biquad1_fixed(pRcvL2, b, a, pOutL2, fixedbuffer1, fixedbuffer2, BUFF_LEN, BIQ_LEN);
					iir_biquad2_fixed(pRcvL2, b, a, pOutL2, fixedbuffer1, BUFF_LEN, BIQ_LEN);
				}
				// if enable_delay is true, a delay copy of RcvL2 is subtracted from OutL2 and the result saved in OutL2
				if (enable_delay) delay_subtract(pOutL2, pRcvL2, pOutL2, delay_bufL, XMIT_BUFF_SIZE, delay);
			}
			// count number of samples processed
			tmp += XMIT_BUFF_SIZE;
		}
		// process right channel (same as before)
		if(RunFilterForR ==1)
		{
			RunFilterForR=0;
			// check which buffer is ready for processing
			if (CurrentRxR_DMAChannel ==2)
			{
				// insert here the code to process RcvR1 buffer
				// the code below simply copies the content of RcvR1 buffer to OutL1 buffer
				if (enable_loop)
				{
					buff_copy(pRcvR1, pOutR1, XMIT_BUFF_SIZE);
				}
				else
				{
					// zero_out(pOutR1, XMIT_BUFF_SIZE);
					// fir_linear_fixed(pRcvR1,h,pOutR1,buffer_fir_R, XMIT_BUFF_SIZE, FIR_LEN);
					//fir_circular_fixed(pRcvR1,h,pOutR1,buffer_firc_R, XMIT_BUFF_SIZE , FIR_LEN);
	//		iir_biquad1_fixed(pRcvR1, b, a, pOutR1, fixedbuffer3, fixedbuffer4, BUFF_LEN, BIQ_LEN);
					 iir_biquad2_fixed(pRcvR1, b, a, pOutR1, fixedbuffer2, BUFF_LEN, BIQ_LEN);
				}
				// if enable_delay is true, a delay copy of RcvR1 is subtracted from OutR1 and the result saved in OutR1
				if (enable_delay) delay_subtract(pOutR1, pRcvR1, pOutR1, delay_bufR, XMIT_BUFF_SIZE, delay);
			}
			else
			{
				// insert here the code to process RcvR2 buffer
				// the code below simply copies the content of RcvR2 buffer to OutR2 buffer
				if (enable_loop)
				{
					buff_copy(pRcvR2, pOutR2, XMIT_BUFF_SIZE);
				}
				else
				{
					// zero_out(pOutR2, XMIT_BUFF_SIZE);
					// fir_linear_fixed(pRcvR2,h,pOutR2,buffer_fir_R, XMIT_BUFF_SIZE, FIR_LEN);
					// fir_circular_fixed(pRcvR2,h,pOutR2,buffer_firc_R, XMIT_BUFF_SIZE , FIR_LEN);
		//	iir_biquad1_fixed(pRcvR2, b, a, pOutR2, fixedbuffer3, fixedbuffer4, BUFF_LEN, BIQ_LEN);
					 iir_biquad2_fixed(pRcvR2, b, a, pOutR2, fixedbuffer2, BUFF_LEN, BIQ_LEN);
				}
				// if enable_delay is true, a delay copy of RcvR2 is subtracted from OutR2 and the result saved in OutR2
				if (enable_delay) delay_subtract(pOutR2, pRcvR2, pOutR2, delay_bufR, XMIT_BUFF_SIZE, delay);
			}
			// count number of samples processed
			tmp += XMIT_BUFF_SIZE;
		}
		// execute when Flag_RTC is set. This is set by RTC interrupt any SEC_COUNT seconds
		// this real time counter is useful to set a watch on a variable i want to test and periodically observe the behavior of that variable
		// by default the interrupt will be set every 10 seconds.
		// the adc is at 24 khz, i will receive 24000 samples per second, with 2 channels it means that im receiving 48000 samples per second. tmp will be incremented by 48000 per second.
		if (Flag_RTC)
		{
			Flag_RTC = 0;
			// evaluate samples per second
			samples_per_sec = tmp/SEC_COUNT;
			tmp = 0;
			switchLED();
		}
	}
}



void InitSystem(void)
{
	Uint16 i;
// PLL set up from RTC
    // bypass PLL
    CONFIG_MSW = 0x0;

#if (PLL_100M ==1)
    PLL_CNTL2 = 0x8000;
    PLL_CNTL4 = 0x0000;
    PLL_CNTL3 = 0x0806;
    PLL_CNTL1 = 0x82FA;
    
#elif (PLL_12M ==1)
    PLL_CNTL2 = 0x8000;
    PLL_CNTL4 = 0x0200;
    PLL_CNTL3 = 0x0806;
    PLL_CNTL1 = 0x82ED;
#elif (PLL_98M ==1)    
    PLL_CNTL2 = 0x8000;
    PLL_CNTL4 = 0x0000;
    PLL_CNTL3 = 0x0806;
    PLL_CNTL1 = 0x82ED;

#elif (PLL_40M ==1)        
    PLL_CNTL2 = 0x8000;
    PLL_CNTL4 = 0x0300;
    PLL_CNTL3 = 0x0806;
    PLL_CNTL1 = 0x8262;    
#endif

    while ( (PLL_CNTL3 & 0x0008) == 0);
    // Switch to PLL clk
    CONFIG_MSW = 0x1;

// clock gating
// enable all clocks
    IDLE_PCGCR = 0x0;
    IDLE_PCGCR_MSW = 0xFF84;
    

// reset peripherals
    PER_RSTCOUNT = 0x02;
    PER_RESET = 0x00fb;    
    for (i=0; i< 0xFFF; i++);
}

void ConfigPort(void)
{
    Int16 i;
    //  configure ports
    PERIPHSEL0 = 0x6900;        // parallel port: mode 6, serial port1: mode 2 

    for (i=0; i< 0xFFF; i++);
}


void SYS_GlobalIntEnable(void)
{
    asm(" BIT (ST1, #ST1_INTM) = #0");
}

void SYS_GlobalIntDisable(void)
{
    asm(" BIT (ST1, #ST1_INTM) = #1");
}



void switchLED(void)
{
	Uint16 temp;

	temp = ST1_55;
	if((temp&0x2000) == 0)
	{
		// turn on LED
		temp |= 0x2000;
		ST1_55 =temp;
	}
	else
	{
		// turn off LED
		temp &=0xDFFF;
		ST1_55 =temp;
	}
}



#if 0
void turnOnLED(void)
{
    Uint16 temp;
    
    temp = ST1_55;
    if((temp&0x2000) == 0)
    {
        // turn on LED
        temp |= 0x2000;
        ST1_55 =temp;
    }
}


void turnOffLED(void)
{
    Uint16 temp;
    
    temp = ST1_55;
    if((temp&0x2000) != 0)
    {
        // turn off LED
        temp &=0xDFFF;
        ST1_55 =temp;
    }
}



void BlinkLED(void)
{
    Uint16 temp;

    if(fLedBlinkOn ==0)
        return;
            
    if(Conunt_RTC > 0)
    {
        Conunt_RTC =0;
        
        temp = ST1_55;
        if((temp&0x2000) == 0)
        {
            // turn on LED
            temp |= 0x2000;
        }
        else
        {
            // turn off LED
            temp &=0xDFFF;
        }
        ST1_55 = temp; 
    }
    
}
#endif

void buff_copy(int *input, int *output, unsigned short size)
{
	int i;
	
	for(i =0; i<size; i++)
	{
		*(output + i) = *(input +i);
	}
	
}


void zero_out(int *output, unsigned short size)
{
	int i;

	for(i =0; i<size; i++)
	{
		*(output + i) = 0;
	}

}


/*
 * subtract from x1 a delayed version of x2
 * x1: pointer to first input samples
 * x2: pointer to second input samples
 * r: pointer to output (can be x1)
 * dbuffer: pointer to delay line for x2 (at least delay+1 samples)
 * nx: size of input buffer
 * delay: delay
 */


void delay_subtract(int *x1, int *x2, int *r, int *dbuffer, unsigned short nx, unsigned short delay)
{
	int i,k;

	if (delay == 0) return;
	// get circular pointer, assume it stored in extra position of dbuffer
	k = (unsigned int)dbuffer[delay] % delay;
	for (i = 0; i < nx; i++)
	{
		// compute difference
		r[i] = x1[i] - dbuffer[k];
		// load new sample in delay line
		dbuffer[k] = x2[i];
		// update circular pointer;
		if (++k == delay) k = 0;
	}
	// save circular pointer
	dbuffer[delay] = k;
}



