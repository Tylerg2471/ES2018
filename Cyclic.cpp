/*
Tyler Gibney

15 1 9 2 2 3
log data to uSD card (serial port used due to no sd card)
Watch pulse is 9mS
watchdog 2 times per second
Display error condition as flashing blue LED
Show execution time of LCD

*/

#include "mbed.h"
#include "PwmIn.h"
#include "MCP23017.h" // include 16-bit parallel I/O header file
#include "WattBob_TextLCD.h" // include 2*16 character display header file
#include "SDFileSystem.h"

#define BACK_LIGHT_ON(INTERFACE) INTERFACE->write_bit(1,BL_BIT)
#define BACK_LIGHT_OFF(INTERFACE) INTERFACE->write_bit(0,BL_BIT)

MCP23017 *par_port; // pointer to 16-bit parallel I/O object
WattBob_TextLCD *lcd; // pointer to 2*16 chacater LCD object
Serial pc(USBTX, USBRX); //tx, rx

/*-----------------------------------------------------------------------
Pin configuration
-----------------------------------------------------------------------*/

PwmIn FreqRead (p26); //Frequency reading pin

DigitalOut myled(LED1);
DigitalOut check(p8); //used for checking execution times
DigitalOut watchd(p7); //used for checking execution times
DigitalIn  Switch_1(p12); //digital input switch
DigitalIn   ShutDown(p6); //digital shutdown switch

AnalogIn an1(p15); //analog input 1
AnalogIn an2(p16); //analog input 2


Ticker  tick;

//SDFileSystem sd(p5, p6, p7, p8, "sd"); // the pinout on the mbed Cool Components workshop board
 
/*-----------------------------------------------------------------------
Variables
-----------------------------------------------------------------------*/




float Freq;
float an1Array [4]; 
float an2Array [4];
float an1Sum = 0;
float an2Sum = 0;
float analog1;
float analog2;
int ti;

int DigIn; 
int ErrorC; 

int TickC = 0;
/*-----------------------------------------------------------------------
This routine uses the PwmIn library to calculate the frequency of the input 
square wave.
-----------------------------------------------------------------------*/

void Frequency() {
    Freq = 1/FreqRead.period();
   
    }
    
/*-----------------------------------------------------------------------
this routine checks the digital switch 1 and the analog values
to see if analog 1 is greater than analog 2. If this is true the error 
code is set to 3.
-----------------------------------------------------------------------*/

    
void DSwitch(){
    DigIn = Switch_1;
    if (DigIn == 1 && (analog1 > analog2)){
        ErrorC = 3; 
        lcd->locate(1,8);
        lcd->printf("E=%d", ErrorC);
               }
    else {
        ErrorC = 0;
        
        }
}


/*-----------------------------------------------------------------------
The two subroutines below take in an analog reading and store it in an array. 
The average filtered result of 4 analog readings are calculated and then 
stored in the variable either analog1 or analog2
-----------------------------------------------------------------------*/
void Analog1(){
    an1Array[0] = an1Array[1];
    an1Array[1] = an1Array[2];
    an1Array[2] = an1Array[3];
    an1Array[3] = an1;
    
    for (int i = 0; i<4; i++){
        an1Sum = an1Sum + an1Array[i];
        }
    analog1 = an1Sum/4;
    analog1 = analog1*3.3;
    an1Sum = 0;
    }

/*-----------------------------------------------------------------------*/

    
void Analog2(){
    
    an2Array[0] = an2Array[1];
    an2Array[1] = an2Array[2];
    an2Array[2] = an2Array[3];
    an2Array[3] = an2;
    
    for (int i = 0; i<4; i++){
        an2Sum = an2Sum + an2Array[i];
        }
    analog2 = an2Sum/4;
    analog2 = analog2*3.3;
    an2Sum = 0;
    }
    
    
/*-----------------------------------------------------------------------    
This routine is used for diasplaying information on the LCd display.
It does this by first clearing the display, going to the correct point 
and then priting thwe string and variable value.
-----------------------------------------------------------------------*/    
void LCD(){
    //lcd->cls(); // clear display
    lcd->locate(0,0); // set cursor to location (0,0) - top left corner
    lcd->printf("A1=%.1f", analog1 ); // print string
    lcd->locate(0,8);
    lcd->printf("A2=%.1f", analog2 );
    lcd->locate(1,0);
    lcd->printf("F=%.1f", Freq);
    lcd->locate(1,8);
    lcd->printf("D=%d", DigIn);
    
    
    }


/*-----------------------------------------------------------------------

This routine checks if error code 3 is true. If it is it sets the LED to 
turn on and off on each cycle creating a blinking effect.
-----------------------------------------------------------------------*/
void ErrInd(){
        if(ErrorC == 3){
            myled = !myled;
            }
        else if (ErrorC == 0){
            
            myled = 0;
        }
        }

/*-----------------------------------------------------------------------
This routine is used as a wachdog pulse which sets a digital output high for 
9 milliseconds then turns it low. 
-----------------------------------------------------------------------*/
void watch(){
    watchd = 1;
    wait_ms(9);
    watchd = 0;
   }
/*-----------------------------------------------------------------------
This routine is the cyclic scheduler which used the tick count to carry
 out tasks based on a predetermined schedule.
-----------------------------------------------------------------------*/   
void Cycle(){
   
   if (TickC % 10 == 1) {
       Analog1();
       }
    else if(TickC % 10 == 2) {
        Analog2();
   }
   else if(TickC % 20 == 3) {
        Frequency(); 
        }
        else if(TickC % 10 == 4) {
        
        watch(); 
        
        }
    else if((TickC % 20 == 7) || (TickC % 20 == 10) || (TickC % 20 == 17)){
        DSwitch(); 
        }
   else if(TickC % 10 == 9) {
        ErrInd(); 
   }
   else if(TickC % 40 == 15) {
        check =1;
        LCD(); 
        check = 0;
   }
   
   else if(TickC % 100 == 5){
   pc.printf("%.1f,%.1f,%.1f,%d\n", Freq , analog1, analog2, DigIn);
   }
   TickC++;
   }   
   

/*-----------------------------------------------------------------------   
This is the main loop where the program carrys out the code included. 
This starts with an initialisation of the lcd dislay.    
-----------------------------------------------------------------------*/    
int main() {
    par_port = new MCP23017(p9, p10, 0x40); // initialise 16-bit I/O chip
    lcd = new WattBob_TextLCD(par_port); // initialise 2*26 char display
    par_port->write_bit(1,BL_BIT); // turn LCD backlight ON
    
   
    tick.attach(&Cycle, 0.050);
    
    while(1) {
   
  

    }
}
