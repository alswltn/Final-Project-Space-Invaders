// ADC.c
// Runs on LM4F120/TM4C123
// Provide functions that initialize ADC0
// Student names: change this to your names or look very silly
// Last modification date: change this to the last modification date or look very silly

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"

// ADC initialization function 
// Input: none
// Output: none
// measures from PD2, analog channel 5
void ADC_Init(void){ 
SYSCTL_RCGCGPIO_R |= 0x08;
	while ((SYSCTL_PRGPIO_R&0x08) == 0){};
		GPIO_PORTD_DIR_R &= ~0x04;	//make PD2 input
		GPIO_PORTD_AFSEL_R |= 0x04;
		GPIO_PORTD_DEN_R &= ~0x04;
		GPIO_PORTD_AMSEL_R |= 0x04;	
		SYSCTL_RCGCADC_R |= 0x01;	//activate ADC0
		volatile int nop = 0;
		nop++;
		nop++;
		nop++;
		nop++;
		ADC0_PC_R = 0x01;	//speed = 125k
		ADC0_SSPRI_R = 0x0123;
		ADC0_ACTSS_R &= ~0x0008;	//disable sample sequencer 3
		ADC0_EMUX_R &= ~0xF000;	//software trigger
		ADC0_SSMUX3_R = (ADC0_SSMUX3_R&0xFFFFFFF0)+5;	// Ain5
		ADC0_SSCTL3_R = 0x0006;
		ADC0_IM_R &= ~0x0008;
		ADC0_ACTSS_R |= 0x0008;

}

//------------ADC_In------------
// Busy-wait Analog to digital conversion
// Input: none
// Output: 12-bit result of ADC conversion
// measures from PD2, analog channel 5
uint32_t ADC_In(void){  
	uint32_t data;
	ADC0_PSSI_R = 0x0008;
	while((ADC0_RIS_R&0x08)==0){};	//start sequencer 3 for sampling
	data = ADC0_SSFIFO3_R&0xFFF;
	ADC0_ISC_R = 0x0008;	//clear flag
		return data;
}


