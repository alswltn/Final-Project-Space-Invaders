// SpaceInvaders.c
// Runs on LM4F120/TM4C123
// Jonathan Valvano and Daniel Valvano
// This is a starter project for the EE319K Lab 10

// Last Modified: 1/17/2020 
// http://www.spaceinvaders.de/
// sounds at http://www.classicgaming.cc/classics/spaceinvaders/sounds.php
// http://www.classicgaming.cc/classics/spaceinvaders/playguide.php
/* This example accompanies the books
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2019
   "Embedded Systems: Introduction to Arm Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2019
 Copyright 2019 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
// ******* Possible Hardware I/O connections*******************
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PD2/AIN5
// Slide pot pin 3 connected to +3.3V 
// fire button connected to PE0
// special weapon fire button connected to PE1
// 8*R resistor DAC bit 0 on PB0 (least significant bit)
// 4*R resistor DAC bit 1 on PB1
// 2*R resistor DAC bit 2 on PB2
// 1*R resistor DAC bit 3 on PB3 (most significant bit)
// LED on PB4
// LED on PB5

// Backlight (pin 10) connected to +3.3 V
// MISO (pin 9) unconnected
// SCK (pin 8) connected to PA2 (SSI0Clk)
// MOSI (pin 7) connected to PA5 (SSI0Tx)
// TFT_CS (pin 6) connected to PA3 (SSI0Fss)
// CARD_CS (pin 5) unconnected
// Data/Command (pin 4) connected to PA6 (GPIO), high for data, low for command
// RESET (pin 3) connected to PA7 (GPIO)
// VCC (pin 2) connected to +3.3 V
// Gnd (pin 1) connected to ground

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "ST7735.h"
#include "Print.h"
#include "Random.h"
#include "PLL.h"
#include "ADC.h"
#include "Images.h"
#include "Sound.h"
#include "Timer0.h"
#include "Timer1.h"

int ADCMail = 0;
int ADCStatus = 0;
int32_t Position;
int8_t portfdone = 0;
int8_t English0Spanish1 = 0;
int8_t LanguageFlag = 0;
int8_t j = 19;
int32_t Score = 0;
int8_t Paused = 0;
int8_t PlayerLife = 3;
int8_t LvlCount = 1;
uint32_t last, now;

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts

void Delay100ms(uint32_t count); // time delay in 0.1 seconds

	void SysTick_Init(void){
	NVIC_ST_CTRL_R = 0;    					// disable SysTick during setup
  NVIC_ST_CTRL_R = 0x07;
  NVIC_ST_RELOAD_R = 2666666;			// reload value --> 30 Hz
  NVIC_ST_CURRENT_R = 0;      		// any write to current clears it
	NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x20000000; // priority 1
	NVIC_ST_CTRL_R = 0x07; //enable interrupts
}

void SysTick_Handler(void){
	//toggle heartbeat
	//sample ADC
	//set ADC flag
	GPIO_PORTF_DATA_R |= 0x0E;
	GPIO_PORTF_DATA_R &=  ~0x0E;
	ADCMail = ADC_In();	
	ADCStatus = 1;
}

void PortEF_Init(void){volatile long delay;                            
  SYSCTL_RCGCGPIO_R |= 0x20;           // activate port F
  while((SYSCTL_PRGPIO_R&0x20)==0){}; // allow time for clock to start
  delay = 100;                  //    allow time to finish activating
  GPIO_PORTF_LOCK_R = 0x4C4F434B;   // 2) unlock GPIO Port F
  GPIO_PORTF_CR_R = 0x1F;           // allow changes to PF4-0                              // 2) GPIO Port F needs to be unlocked
  GPIO_PORTF_AMSEL_R &= ~0x11;  // 3) disable analog on PF4,0
                                // 4) configure PF4,0 as GPIO
  GPIO_PORTF_PCTL_R &= ~0x000F000F;
  GPIO_PORTF_DIR_R &= ~0x11;    // 5) make PF4,0 in
  GPIO_PORTF_AFSEL_R &= ~0x11;  // 6) disable alt funct on PF4,0
	GPIO_PORTF_PUR_R |= 0x11;     
  GPIO_PORTF_DEN_R |= 0x11;     // 7) enable digital I/O on PF4,0
  GPIO_PORTF_IS_R &= ~0x11;     //    PF4,0 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x11;    //    PF4,0 is not both edges
  GPIO_PORTF_IEV_R &= ~0x11;     //    PF4,0 falling edge event (Neg logic)
  GPIO_PORTF_ICR_R = 0x11;      //    clear flag1-0
  GPIO_PORTF_IM_R |= 0x11;      // 8) arm interrupt on PF4,0
                                // 9) GPIOF priority 2
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF0FFFFF)|0x00400000;
  NVIC_EN0_R = 1<<30;   // 10)enable interrupt 30 in NVIC
}

uint32_t Convert(uint32_t input){
	return (input/42);
 //return (((160*input)/4096-19));
}



typedef enum { dead, alive} status_t;
struct sprite {
	int32_t x;	//x coordinate
	int32_t y;	//y coordinate
	int32_t vx, vy;	//pixels/30Hz
	const unsigned short *image;	//pointer to image
	const unsigned short *black;
	status_t life;	//dead or alive
	uint32_t w;	//width
	uint32_t h;	//height
	uint32_t needDraw;	//true(1) if need to draw false(0) if not
};

typedef struct sprite sprite_t;
sprite_t Enemy[100]; // enemies are 0-18, missiles are 19-35
int Flag;
int Anyalive;

void GameInit(void) {
	int i;
	int ii = 0;
	int iii = 0;
	for (i = 0; i < 18; i++){
			Enemy[i].x = 20*i;	//space them out across x coordinate
			Enemy[i].y = 20;
			Enemy[i].vx = 0;	//not moving
			Enemy[i].vy = 1;
			Enemy[i].image = SmallEnemy10pointA;
			Enemy[i].black = BlackEnemy;
			Enemy[i].life = alive;
			Enemy[i].w = 16;
			Enemy[i].h = 10; 
			Enemy[i].needDraw = 1;
	}
	for(i = 6;i < 12;i++){
		Enemy[i].life = dead;
		Enemy[i].y = 30;
		Enemy[i].x = 20*ii;
		ii++;
	}
	for(i = 12;i < 18;i++){
		Enemy[i].life = dead;
		Enemy[i].y = 40;
		Enemy[i].x = 20*iii;
		iii++;
	}
	for(i = 0;i < 18;i = i + 2){
		Enemy[i].vx = -1;
	}
	for(i = 1; i < 18;i = i + 2){
		Enemy[i].vx = 1;
	}
	for (i = 19; i < 100; i++){ //missiles
		Enemy[i].x = 0;
		Enemy[i].y = 140;
		Enemy[i].vx = 0;
		Enemy[i].vy = -2;
		Enemy[i].image = Missiles;
		Enemy[i].black = BlackMissile;
		Enemy[i].life = dead;
		Enemy[i].w = 5;
		Enemy[i].h = 9;
		Enemy[i].needDraw = 0;
	}
}

void GameMove(void){ int i;
	Anyalive = 0;
	//for(i = 0;i < 18;i++){				//randomizes sprite movement for left and right
		//int8_t m = (Random())%60;
		//if(m < 29){
			//Enemy[i].vx = -1;
		//}else{
			//Enemy[i].vx = 1;
		//}
	//}
	for(i = 0;i < 6;i++){
		if(Enemy[i].vx == -1){
			Enemy[i].vx = 1;
		}else{
			Enemy[i].vx = -1;
		}
	}
	for(i = 0;i < 18;i++){				//checks if enemy has been hit by missile
		for(int8_t ii = 19;ii < 100;ii++){
			if(Enemy[ii].life == alive){
			if(Enemy[i].life == alive){
			if(((Enemy[ii].y) - 3 <= (Enemy[i].y))&&((Enemy[i].y) <= (Enemy[ii].y + 3))){
				if(((Enemy[ii].x) - 7 <= (Enemy[i].x))&&((Enemy[i].x) <= (Enemy[ii].x + 8))){
					Enemy[i].life = dead;
					Enemy[ii].life = dead;
					Enemy[i].needDraw = 1;
					Enemy[ii].needDraw = 1;
					Score = Score + 100;
					Sound_Killed();
					}
				}
			}
		}
	}
}
	for(i = 0;i < 100;i++){
		if(Enemy[i].life == alive){
			Enemy[i].needDraw = 1;
			Anyalive = 1;
			if(Enemy[i].y > 140){  //if enemy hits bottom of screen
				Enemy[i].life = dead;
				PlayerLife--;
			}else{
				if(Enemy[i].y < 10){  //if enemy hits the top of screen
					Enemy[i].life = dead;
				}else{
					if(Enemy[i].x < 0){  //if enemy hits the left of screen
					Enemy[i].x = Enemy[i].x + 1;
				}else{
					if(Enemy[i].x > 102){  //if enemy hits the right of screen
					Enemy[i].x = Enemy[i].x - 1;
				}else{
					Enemy[i].x += Enemy[i].vx;
					Enemy[i].y += Enemy[i].vy;
				}			
			}				
		}
	}		
}
}
	for(i = 0;i < 18;i++){				//checks if sprites are going to run into each other
		for(int8_t ii = 0;ii < 18;ii++){
			if(i == ii){
				ii++;
			}
			if(((Enemy[i].x + 16) > Enemy[ii].x)&(Enemy[i].x < (Enemy[ii].x - 16))){
				Enemy[i].vx = -1;
			}
			if(((Enemy[i].x - 16) > Enemy[ii].x)&(Enemy[i].x < (Enemy[ii].x + 16))){
				Enemy[i].vx = 1;
			}
		}
	}
	if(Anyalive == 0){
	if(PlayerLife > 0){
		LvlCount++;
		GameInit();
		if(LvlCount == 2){
			for(i = 0;i < 12;i++){
				Enemy[i].life = alive;
			}
		}
		if(LvlCount > 2){
			for(i = 0;i < 18;i++){
				Enemy[i].life = alive;
			}
		}
		ST7735_FillScreen(0x0000);
		ST7735_SetCursor(6, 7);
		ST7735_OutString("Level");
		ST7735_SetCursor(13, 7);
		LCD_OutDec(LvlCount);

		int32_t wastetime = (727240*25);  // 0.1sec at 80 MHz
    while(wastetime){
	  	wastetime--;
		}
		ST7735_FillScreen(0x0000);
	}
	}
}
void GameDraw(void){
		//if(English0Spanish1 == 0){
		//ST7735_SetCursor(1,1);
		//ST7735_OutString("Score:");
		//ST7735_SetCursor(7,1);
		//LCD_OutDec(Score); //display score
	//}else{
		//ST7735_SetCursor(1,1);
		//ST7735_OutString("Puntaje:");
		//ST7735_SetCursor(9,1);
		//LCD_OutDec(Score); //display score
	//}
	
	int i;
	for(i = 0;i < 100;i++){ //moves enemy sprites
		if(Enemy[i].needDraw){
			if(Enemy[i].life == alive){
				ST7735_DrawBitmap(Enemy[i].x,Enemy[i].y,
				Enemy[i].image,Enemy[i].w,Enemy[i].h);
			}else{
				ST7735_DrawBitmap(Enemy[i].x,Enemy[i].y,
				Enemy[i].black,Enemy[i].w,Enemy[i].h);
			}
			Enemy[i].needDraw = 0;
		}
	}
	ST7735_DrawBitmap(0, 159, ClearPlayerShip, 128, 8);
	ST7735_DrawBitmap(Position, 159, PlayerShip0, 18,8); //moves player ship
	
}

void GameTask(void){
	GameMove();
	//Flag = 1;
	//while(ADCStatus == 0){}
	ADCMail = ADC_In();
	Position = Convert(ADCMail);
	ADCStatus = 0;
}

void GPIOPortF_Handler(void){
	if ((GPIO_PORTF_RIS_R & 0x01)==0x01){ //PF0
		if(LanguageFlag == 0){
			English0Spanish1 = 1;
		}else{
		Sound_Shoot();
		Enemy[j].life = alive;
		Enemy[j].x = Position + 6;
		j++;
		if(j == 99){
			j = 19;
		}
		//playsound(Fire); //edit this line to fit our game
		}
	}
	if ((GPIO_PORTF_RIS_R & 0x10)==0x10){ //PF4
		if(LanguageFlag == 0){
			English0Spanish1 = 0;
		}
		if(LanguageFlag == 1){
		if(Paused == 0){
		ST7735_FillScreen(0x0000);
		ST7735_SetCursor(8, 7);
		ST7735_OutString("Paused");
		Paused ^= 0x01;
		while(Paused == 1){
			while((GPIO_PORTF_DATA_R & 0x10)==0x10){}
			while((GPIO_PORTF_DATA_R & 0x10)==0x00){}
			while((GPIO_PORTF_DATA_R & 0x10)==0x10){}
			while((GPIO_PORTF_DATA_R & 0x10)==0x00){}
			if((GPIO_PORTF_DATA_R & 0x10)==0x10){
				ST7735_FillScreen(0x0000);
				Paused ^= 0x01;
			}
		}
		}
	}
}
  GPIO_PORTF_ICR_R = 0x11;      // acknowledge flag4
	portfdone = 1;
}

uint32_t PortF_Input(){
	return GPIO_PORTF_DATA_R&0x01;
}

int main(void){

  DisableInterrupts();
  PLL_Init(Bus80MHz);       // Bus clock is 80 MHz 
	ADC_Init();
	PortEF_Init();
	Sound_Init();
  Random_Init(1);
	Random_Init(NVIC_ST_CURRENT_R);

  Output_Init();
	GameInit();
  ST7735_FillScreen(0x0000);            // set screen to black
	ST7735_SetCursor(3, 6);
	ST7735_OutString("Choose Language:");
	ST7735_SetCursor(1, 7);
	ST7735_OutString("Press A for English");
	ST7735_SetCursor(1, 8);
	ST7735_OutString("Press B for Spanish");
	EnableInterrupts();
	while(portfdone == 0){}; // wait for response from user
	

	
	ST7735_FillScreen(0x0000);
	if(English0Spanish1 == 0){
		ST7735_SetCursor(7, 7);
		ST7735_OutString("Welcome");
		LanguageFlag = 1;
	}else{
		ST7735_SetCursor(5, 7);
		ST7735_OutString("Bienvenidos");
		LanguageFlag = 1;
	}
	
	int32_t wastetime = (727240*25);  // 0.1sec at 80 MHz
    while(wastetime){
	  	wastetime--;
    }
		
	ST7735_FillScreen(0x0000);
  
  //ST7735_DrawBitmap(22, 159, PlayerShip0, 18,8); // player ship bottom
  //ST7735_DrawBitmap(53, 151, Bunker0, 18,5);
  //ST7735_DrawBitmap(42, 159, PlayerShip1, 18,8); // player ship bottom
  //ST7735_DrawBitmap(62, 159, PlayerShip2, 18,8); // player ship bottom
  //ST7735_DrawBitmap(82, 159, PlayerShip3, 18,8); // player ship bottom
	Timer1_Init(&GameTask,80000000/30);
	do{
		//while(Flag == 0){
			Flag = 0;
			GameDraw();
		//}
		}while(PlayerLife > 0);

  //ST7735_DrawBitmap(0, 9, SmallEnemy10pointA, 16,10);
  //ST7735_DrawBitmap(20,9, SmallEnemy10pointB, 16,10);
  //ST7735_DrawBitmap(40, 9, SmallEnemy20pointA, 16,10);
  //ST7735_DrawBitmap(60, 9, SmallEnemy20pointB, 16,10);
  //ST7735_DrawBitmap(80, 9, SmallEnemy30pointA, 16,10);
  //ST7735_DrawBitmap(100, 9, SmallEnemy30pointB, 16,10);

  //Delay100ms(50);              // delay 5 sec at 80 MHz

	if(English0Spanish1 == 0){
		ST7735_FillScreen(0x0000);   // set screen to black
		ST7735_SetCursor(6, 5);
		ST7735_OutString("GAME OVER");
		ST7735_SetCursor(6, 6);
		ST7735_OutString("Nice try,");
		ST7735_SetCursor(6, 7);
		ST7735_OutString("Earthling!");
		ST7735_SetCursor(8, 8);
		LCD_OutDec(Score);
	}else{
		ST7735_FillScreen(0x0000);   // set screen to black
		ST7735_SetCursor(3, 5);
		ST7735_OutString("JUEGO TERMINADO");
		ST7735_SetCursor(4, 6);
		ST7735_OutString("Buen intento,");
		ST7735_SetCursor(5, 7);
		ST7735_OutString("Terr¨ªcola!");
		ST7735_SetCursor(8, 8);
		LCD_OutDec(Score);
	}
}


// You can't use this timer, it is here for starter code only 
// you must use interrupts to perform delays
void Delay100ms(uint32_t count){uint32_t volatile time;
  while(count>0){
    time = 727240;  // 0.1sec at 80 MHz
    while(time){
	  	time--;
    }
    count--;
  }
}
