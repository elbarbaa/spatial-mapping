#include <stdint.h>
#include "PLL.h"
#include "SysTick.h"
#include "uart.h"
#include "onboardLEDs.h"
#include "tm4c1294ncpdt.h"
#include "VL53L1X_api.h"

#include <stdio.h>
#include <math.h>
double pi = 3.14159265358979323846;
int process = 0;


#define I2C_MCS_ACK             0x00000008  // Data Acknowledge Enable
#define I2C_MCS_DATACK          0x00000008  // Acknowledge Data
#define I2C_MCS_ADRACK          0x00000004  // Acknowledge Address
#define I2C_MCS_STOP            0x00000004  // Generate STOP
#define I2C_MCS_START           0x00000002  // Generate START
#define I2C_MCS_ERROR           0x00000002  // Error
#define I2C_MCS_RUN             0x00000001  // I2C Master Enable
#define I2C_MCS_BUSY            0x00000001  // I2C Busy
#define I2C_MCR_MFE             0x00000010  // I2C Master Function Enable
#define MAXRETRIES              5           // number of receive attempts before giving up



// Port H ---------------------------------------------------------------------------

void PortH_Init(void){
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R7;						// activate clock for Port
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R7) == 0){};		// allow time for clock to stabilize
	GPIO_PORTH_DIR_R |= 0xFF;        										//Make output
  GPIO_PORTH_DEN_R |= 0xFF;        										// enable digital IO																				    								
	return;
}
//----------------------------------------------------------------------------------------------------------

// Port N ----------------------------------------------------------------------------------

void PortN_Init(void){
SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R12; //activate the clock for Port F
while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R12) == 0){};//allow time for clock to stabilize
GPIO_PORTN_DIR_R=0b00000010; //Make PN1 output, to turn on LED's
GPIO_PORTN_DEN_R=0b00000010;
return;
}

// ---------------------------------------------------------------------------------------------------------

void PortF_Init(void){
SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5; //activate the clock for Port F
while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R5) == 0){};//allow time for clock to stabilize
GPIO_PORTN_DIR_R=0b00010000; //Make PF4 output, to turn on LED's
GPIO_PORTN_DEN_R=0b00010000;
return;
}

// Port M ---------------------

void PortM_Init(void){
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R11;                 // Activate the clock for Port M
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R11) == 0){};        // Allow time for clock to stabilize 
	GPIO_PORTM_DIR_R = 0b00000000;       								      // Make PM0 and PM1 inputs 
  GPIO_PORTM_DEN_R = 0b00000011;
	GPIO_PORTM_CR_R = 0x01;
	GPIO_PORTM_LOCK_R = 0x0;
	GPIO_PORTM_PUR_R = 0b00000011; 														// Enable the pull-up resistors for PM1:PM0
	return;
}




// I2C init---------------------------------------------------------------------------------------

void I2C_Init(void){
  SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0;           													// activate I2C0
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;          												// activate port B
  while((SYSCTL_PRGPIO_R&0x0002) == 0){};																		// ready?

    GPIO_PORTB_AFSEL_R |= 0x0C;           																	// 3) enable alt funct on PB2,3       0b00001100
    GPIO_PORTB_ODR_R |= 0x08;             																	// 4) enable open drain on PB3 only

    GPIO_PORTB_DEN_R |= 0x0C;             																	// 5) enable digital I/O on PB2,3
//    GPIO_PORTB_AMSEL_R &= ~0x0C;          																// 7) disable analog functionality on PB2,3

                                                                            // 6) configure PB2,3 as I2C
//  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00003300;
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00002200;    //TED
    I2C0_MCR_R = I2C_MCR_MFE;                      													// 9) master function enable
    I2C0_MTPR_R = 0b0000000000000101000000000111011;                       	// 8) configure for 100 kbps clock (added 8 clocks of glitch suppression ~50ns)
//    I2C0_MTPR_R = 0x3B;                                        						// 8) configure for 100 kbps clock
        
}


//----------------------------------------------------------------------------------------------

// Port G_Initlize -------------------------------------------------------------------------------
void PortG_Init(void){
    //Use PortG0
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R6;                // activate clock for Port N
    while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R6) == 0){};    		// allow time for clock to stabilize
    GPIO_PORTG_DIR_R &= 0x00;                               // make PG0 in (HiZ)
  GPIO_PORTG_AFSEL_R &= ~0x01;                              // disable alt funct on PG0
  GPIO_PORTG_DEN_R |= 0x01;                                 // enable digital I/O on PG0
                                                            // configure PG0 as GPIO
  //GPIO_PORTN_PCTL_R = (GPIO_PORTN_PCTL_R&0xFFFFFF00)+0x00000000;
  GPIO_PORTG_AMSEL_R &= ~0x01;                               // disable analog functionality on PN0

    return;
}
//-------------------------------------------------------------------------------------------------------

//XSHUT     This pin is an active-low shutdown input; 
//					the board pulls it up to VDD to enable the sensor by default. 
//					Driving this pin low puts the sensor into hardware standby. This input is not level-shifted.
void VL53L1X_XSHUT(void){
    GPIO_PORTG_DIR_R |= 0x01;                                        // make PG0 out
    GPIO_PORTG_DATA_R &= 0b11111110;                                 //PG0 = 0
    SysTick_Wait10ms(10);
    GPIO_PORTG_DIR_R &= ~0x01;                                            // make PG0 input (HiZ)
    
}

// Motor control function --------------------------------------------------------------------------------------

void step(void){
	
	int z = 2; 					// 1.40625 degrees per step. there is 512 steps in 360 degrees. so step size we need is = [512*(Degree we need for each step)]/360
	
	int wait = 1;
	
	// rotates clockwise for the step size designated by z		
	// if button press is ever encountered process breaks off
	for(int i=0; i<z; i++){
		GPIO_PORTH_DATA_R = 0b00001001;
		SysTick_Wait10ms(wait);
		GPIO_PORTH_DATA_R = 0b00000011;
		SysTick_Wait10ms(wait);
		GPIO_PORTH_DATA_R = 0b00000110;
		SysTick_Wait10ms(wait);
		GPIO_PORTH_DATA_R = 0b00001100;
		SysTick_Wait10ms(wait);
	}
}

//-------------------------------------------------------------------------------------------------------------

// Return to home -----------------------------------------------------------------------------------

void stepReturn(void){
	// Function will return home
	SysTick_Wait10ms(100);
	//int z=64; 			
	int z=512;				//Corresponds to a step size of 360 degrees	
	
	// systick delay is 12.5ns * wait 
	int wait = 1; //gives delay of 3ms
	GPIO_PORTN_DATA_R ^= 0b0000000010; //PN1 Toggle (and stay on for now)
	
	// rotates clockwise for the step size designated by z		
	// if button press is ever encountered process breaks off
	for(int i=0; i<z; i++){
		GPIO_PORTH_DATA_R = 0b00001100;
		SysTick_Wait10ms(wait);
		GPIO_PORTH_DATA_R = 0b00000110;
		SysTick_Wait10ms(wait);
		GPIO_PORTH_DATA_R = 0b00000011;
		SysTick_Wait10ms(wait);
		GPIO_PORTH_DATA_R = 0b00001001;
		SysTick_Wait10ms(wait);
	}
	SysTick_Wait10ms(10); //delay of 0.1s
	GPIO_PORTN_DATA_R ^= 0b000000010; //PN1 Toggle and stay off
}

//---------------------------------------------------------------------------------------------------------------


//***********					MAIN Function				*****************************************************************

uint16_t	dev = 0x29;			
int status=0;

int main(void) {
  uint8_t byteData, sensorState=0, myByteArray[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} , i=0;
  uint16_t wordData;
  uint16_t Distance;
  uint16_t SignalRate;
  uint16_t AmbientRate;
  uint16_t SpadNum; 
  uint8_t RangeStatus;
  uint8_t dataReady;

	//initialize
	PLL_Init();	
	SysTick_Init();
	onboardLEDs_Init();
	I2C_Init();
	UART_Init();
	PortH_Init();
	PortN_Init();
	PortM_Init();
	
	int program_run = 0;
	
	
	while(program_run == 0){
	if((GPIO_PORTM_DATA_R&0b00000001) == 0){
			while((GPIO_PORTM_DATA_R&0b00000001) == 0){}
				program_run ^= 1;
		}
	}
	SysTick_Wait10ms(10);
	
	// Indictaes program is working
	UART_printf("Program Begins\r\n");
	
// ----------------------------------------------------------------------------------------------------------
//sensor 
	while(sensorState==0){
		status = VL53L1X_BootState(dev, &sensorState);
		SysTick_Wait10ms(10);
  }
	UART_printf("ToF Chip Booted\r\n Please Wait...\r\n");
	
	status = VL53L1X_ClearInterrupt(dev); /* clear interrupt has to be called to enable next interrupt*/
	
  /* This function must to be called to initialize the sensor with the default setting  */
  status = VL53L1X_SensorInit(dev);
	Status_Check("SensorInit", status);
  status = VL53L1X_StartRanging(dev);   // This function has to be called to enable the ranging

	double degree = 90;
	double D;
	double y;
	double z;
	int x = 0;
	int new_plane = 0;
	
// ------------------------------------------------------------------------------
	while(program_run==1){
			
		if((GPIO_PORTM_DATA_R&0b00000010) == 0){
			while((GPIO_PORTM_DATA_R&0b00000010) == 0){}
				new_plane = 1;
		}
		
		if((GPIO_PORTM_DATA_R&0b00000001) == 0){
			while((GPIO_PORTM_DATA_R&0b00000001) == 0){}
				program_run ^= 1;
		}
		
		
	if(new_plane == 1){
	
	for(int i = 0; i < 256; i++) { //run it 256 times because z is 2 so 512/2 = 256
		
		if(i%32==0){ //if is divisble by 32,then blink LED. B/c number of times it will run divided by 8 -> 360/8 = 45 degrees. So 256/8 = 32
			GPIO_PORTF_DATA_R ^= 0b00010000;
			SysTick_Wait10ms(5);
			GPIO_PORTF_DATA_R ^= 0b00010000;
		}
			
//---------------------------------------------------------------------------------------
		//wait until the ToF sensor's data is ready
	  while (dataReady == 0){  //waits for the ToF sensor's data to be ready
          status = VL53L1X_CheckForDataReady(dev, &dataReady);
          VL53L1_WaitMs(dev, 5);
      }
        dataReady = 0;

        int max = 0;  //checks measurement to make sure it is accurate
        status = VL53L1X_GetRangeStatus(dev, &RangeStatus);
        while(RangeStatus!=0&&max<10){
            max++;
            status = VL53L1X_ClearInterrupt(dev);
            while (dataReady == 0){
                status = VL53L1X_CheckForDataReady(dev, &dataReady);
                        VL53L1_WaitMs(dev, 5);
      }
            dataReady = 0;
            status = VL53L1X_GetRangeStatus(dev, &RangeStatus);
        }
        max = 0;
      status = VL53L1X_GetDistance(dev, &Distance);

		
//----------------------------------------------------------------------------
		
		//1st Quarter
		if(degree>=0&&degree<=90){
			y = Distance*cos((degree)*pi/180);
			z = Distance*sin((degree)*pi/180);
		}
		//2nd Quarter
		else if(degree<=180&&degree>90){
			y = -1*Distance*cos((180-degree)*pi/180);
			z = Distance*sin((180-degree)*pi/180);
		}
		//3rd Quarter
			else if(degree<=275&&degree>180){
			y = -1*Distance*cos((degree-180)*pi/180);
			z = -1*Distance*sin((degree-180)*pi/180);
		}
		//4th Quarter
		else if(degree<360&&degree>275){
			y = Distance*cos((360-degree)*pi/180);
			z = -1*Distance*sin((360-degree)*pi/180);
		}
	
//---------------------------------------------------------------------------------------------

	  status = VL53L1X_ClearInterrupt(dev); /* clear interrupt has to be called to enable next interrupt*/
		
		sprintf(printf_buffer,"$%d$@%d@!%d!\r\n", x,(int)y,(int)z);
		UART_printf(printf_buffer);
		
		if(degree>0){
		degree -= 1.40625;
		}
		else{
			degree = 360-1.40625;
		}
		step();
  }
	stepReturn();
		x += 500;
	new_plane ^= 1;
	}
	}
	
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ 
	
	VL53L1X_StopRanging(dev);
	UART_printf("Z\r\n");

}