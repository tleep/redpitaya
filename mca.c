/* Analog event aquisisition for AirTED 
 * written by Tristen Lee 2022
 * 
 * Linux compile command:
 * gcc -o MCA MCA.c fpga_osc.c -g -std=gnu99 -Wall -Werror -DVERSION=0.00-0000 -DREVISION=devbuild -lm -lpthread
 */
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stddef.h>
#include <sys/param.h>
#include <time.h>
#include "fpga_osc.h"


/*  initialization */

const int BUF = 16*1024;		// Buffer size
const int N = 255; 				// desired length of trace (1,..., 16383)
const int decimation = 1;		// decimation: [1;8;64;1024;8192;65536]

int main(void) {
	
	// Initialization and check
	
	int start = osc_fpga_init();
	
	if(start) {
		
		printf("FPGA didn't initialize,check osc_fpga_init, retval = %d",start);
		
		return -1;
		
	}
	
	//file stuff
	FILE * fp;
	time_t rawtime;
	char buffer[N];
	time(&rawtime);
	// name data file as 'rp-xxxxxx-data.txt'
	sprintf(buffer, "/MCA/rp-f097dd-data.txt");
	
	fp = fopen (buffer, "a");
	
	/* AQUISITION SETUP */
	
	int * cha_signal;			// Initialize signal variables
	int * chb_signal;       
	
	osc_fpga_set_trigger_delay(N); // Set acquisition parameters
	g_osc_fpga_reg_mem->data_dec = decimation;
	
	osc_fpga_reset();			// Reset sampling engine 
	
	// Define initial parameters
	
	int trig_ptr; 			// Trigger pointer shows memory address where trigger was met
	int trig_test;			// Trigger test checks if writing the trace has completed yet
	int trigger_voltage = 0.0;	// Trigger voltage in [V] as parameter [1V...~600 RP units]
	g_osc_fpga_reg_mem->cha_thr = osc_fpga_cnv_v_to_cnt(trigger_voltage)/500; //sets trigger voltage
	
	


	// Set trigger, and begin acquisition when trigger is activated
	
	osc_fpga_arm_trigger();		// Start acquiring, incrementing write pointer
	
	// Choose trigger method. High gain channel should be chosen.
	
	osc_fpga_set_trigger(2);
	
	/*  *     0 - end of acquisition/no acquisition
		*     1 - trig immediately
		*     2 - ChA positive edge 
		*     3 - ChA negative edge
		*     4 - ChB positive edge 
		*     5 - ChB negative edge
		*     6 - External trigger 0
		*     7 - External trigger 1*/
	
	// Trigger always changes to 0 after acquisition is completed
	// Write pointer stops incrementing
	// Reference -> fpga.osc.h l66
	
	
	// Wait for the acquisition to finish = trigger is set to 0*/
	
	trig_test=(g_osc_fpga_reg_mem->trig_source); // Gets the above trigger value
	
	
	// Now loop that wait for aquisition is done before moving on.
	
	while (trig_test!=0) {
		
		trig_test=(g_osc_fpga_reg_mem->trig_source);	//->fpga_osc.c l366
		
	}		
	
	// Point to memory address where trigger was activated
	
	trig_ptr = g_osc_fpga_reg_mem->wr_ptr_trigger;		//->fpga_osc.c l283 
	osc_fpga_get_sig_ptr(&cha_signal, &chb_signal);		//->fpga_osc.c l378
	// Read N voltage samples from the trigger pointer location
	
	int i;
	int ptr_a;
	int ptr_b;
	int volts_a[N]; 	//channel A
	int volts_b[N]; 	//channel B
	
	for (i=0; i < N; i++) {
		
		ptr_a = (trig_ptr+i)%BUF;				// Pointer location
	
		if (cha_signal[ptr_a]>=8192) {	    // Fix negative values
			
		volts_a[i] = cha_signal[ptr_a]-16384;
		
			}
		
		else {
		
		volts_a[i] = cha_signal[ptr_a];
		
		}  
	}
	
	for (i=0; i < N; i++) {

		ptr_b = (trig_ptr+i)%BUF;                         // Pointer location

		if (chb_signal[ptr_b]>=8192) {                    // Fix negative values

		volts_b[i] = chb_signal[ptr_b]-16384;

		}

		else {

		volts_b[i] = chb_signal[ptr_b];

		}
	}
	
	// Parse volts_a[ ] to find max_a pulse height 
	
	int scale;               
	int max_a; 
	int max_b;
	int j;
	max_a = volts_a[0];			// Set max_a to first voltage value
	max_b = volts_b[0];
	scale = 1;				// Scales pulses based on output range of shaping amp
							// set to 1 for 1-10V output range
							// set to 2 for 1-5V output range
	
	for (j = 0; j < N; j++) {
		
		if (volts_a[j] > max_a) {
		
		max_a = scale*volts_a[j];
		
		}
		
	}
	
	for (j = 0; j < N; j++) {

		if (volts_b[j] > max_b) {

		max_b = scale*volts_b[j];

		}

	}     
	
	running = false; // AQUISITION loop Terminated
	
	// Print channel A, channel B, time of event
	fprintf(fp, "%d\t %d\t %s", max_a, max_b, ctime(&rawtime));
	
	
	


	
	// Close file, reset for next aquisition
	fclose(fp);
	osc_fpga_exit();
	
	return 0;

}