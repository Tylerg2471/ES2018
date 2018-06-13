/* Author: Tyler Gibney
Program Description: This Program implements a Rtos to control the features of 
a simulated vehicle. The Vehicle is a car which has multiple features which are
simulated using various inputs and Led outputs. The subroutines of this program
implement the behaviour of the different features whilst the variables needed
are setup in the variables section.

The Rtos method for this program is the use of tasks to implement the 
functions at the correct frequency. The frequency of the functions are:
1. Read Accelerator and brake values - 10 HZ or every 100ms
2. Read if engine is on/off and light Led accordingly - 2 Hz or every 500ms
3. Filter the speed using an averaging filter with size 3 - 5 Hz or every 200ms
4. Show the brake usage on an Led - 2 Hz or every 500ms
5. Monitor speed and if it exceeds 88 turn on an Led - 1 Hz or every 1000ms
6. Display odometer value and average speed - 2 Hz or every 500ms
7. Send speed, accelerometer and brake values to mail - 0.2 Hz or every 5000ms
8. Dump mail contents to a PC via Serial - 0.05 Hz or every 20s 
9. Read a Side-light switch and show status on Led - 1 Hz or eveyr 1000ms
10. Read two indicator lights and flash accordingly at 1 Hz, if both lights are 
    on indicate hazard mode via flahsing both Led's at 2 Hz - 0.5 Hz or 2000ms
*/
#include "mbed.h"
#include "rtos.h"
#include "WattBob_TextLCD.h" 
#include "MCP23017.h"

#define BACK_LIGHT_ON(INTERFACE) INTERFACE->write_bit(1,BL_BIT)
#define BACK_LIGHT_OFF(INTERFACE) INTERFACE->write_bit(0,BL_BIT)

MCP23017 *par_port; // pointer to 16-bit parallel I/O object
WattBob_TextLCD *lcd; // pointer to 2*16 chacater LCD object
Serial pc(USBTX, USBRX); //tx, rx

Mutex mail_mutex;

Thread AccBrk_thread;
Thread Sim_thread;
Thread Group_1_thread;
Thread Group_2_thread;
Thread TurnHaz_thread;
Thread Send_mail_thread;
Thread Print_mail_thread;
Thread Hazard_thread;
Thread Turning_thread;
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//

//I/O Declarations

DigitalOut  SideL(LED1);
DigitalOut  IndL(LED2);
DigitalOut  IndR(LED3);
DigitalOut  EngInd(LED4);
DigitalOut  BraLed(p8);
DigitalOut  MonLed(p12);
DigitalOut  Check(p14);

DigitalIn   EngSw(p13);
DigitalIn   SideSw(p6);
DigitalIn   LeftInd(p7);
DigitalIn   RightInd(p11);


AnalogIn    Brake(p15);
AnalogIn    Acc(p16);
//----------------------------------------------------------------------------//
//Variable Declarations
float       Accel;
float       Bra;
float       AccPer;
float       BraPer;
float       Distance;

int         TopSpeed = 120;

int         Sp;
int         SpeedArr [3];
int         AvgSp;

int         c = 0;
int         Write = 0;
int         Print = 0;
//----------------------------------------------------------------------------//
//Timer for Distance calculation
Timer Odo; 
//----------------------------------------------------------------------------//
/* Mail */
typedef struct {
  int    Speed; /* Speed value*/
  float  Accel; /* Accelerator value*/
  float  Brake; /* Brake Value*/
} mail_t;

Mail <mail_t, 100> mail_box;

//----------------------------------------------------------------------------//
//Subroutines
//----------------------------------------------------------------------------//
/*Routine for taking the analog values for accelerator and brake and 
converting to a percentage. In this situation the inputs were between 0 and 1*/
void AccBrk(){
        while(true){
            Accel   =   Acc;
            Bra     =   Brake;
    
            AccPer  =   Accel; //Reading is from 0 to 1`
            BraPer  =   Bra;//Reading is from 0 to 1`
            Thread::wait(100);
        }
      }
//----------------------------------------------------------------------------//   
/*Routine for calculatng the distance travelled in the journey*/
void Odometer(){
        float DisTemp   = 0;
        DisTemp         = AvgSp * (Odo.read()/1000);
        Odo.reset();
        Distance        = Distance + DisTemp;
      }
//----------------------------------------------------------------------------//
/*Routine for engine status indication*/
void Eng(){
        if (EngSw == 1){
            EngInd = 1;
            Odo.start();
        }
        if (EngSw == 0) {
            EngInd = 0;
            Odo.stop();
        }
        Odometer();
      } 
//----------------------------------------------------------------------------//   
/*Routine for Brake indication*/
void BraInd(){
        if (Bra > 0.1){
            BraLed = 1;
        } 
        if(Bra == 0){
            BraLed = 0;
        }
      }
//----------------------------------------------------------------------------//   
/*Routine for calculating the current speed and averaged speed of the car.
  Acts as the simulator portion for the car */
void Speed(){
        while(true){
            if (EngSw ==1){
                int SpSum   = 0;
                int ASp     = TopSpeed * AccPer; 
                int BFactor = (ASp * BraPer); 
                Sp          = ASp - BFactor;
    
                SpeedArr[0] = SpeedArr[1];
                SpeedArr[1] = SpeedArr[2];
                SpeedArr[2] = Sp;
    
                for (int i=0; i<3 ; i++){
                    SpSum   = SpeedArr[i] + SpSum;
                }
                AvgSp           = SpSum/3;
            }
            else if (EngSw ==0){
                Sp = 0;
                AvgSp = 0;
            }
            
            Thread::wait(200);
        }
      }
//----------------------------------------------------------------------------//   
/*Routine for monitoring if the speed is greater than 88mph and if so the 
   MonLed indication light is lit*/
void Monitor(){
        if(AvgSp > 88){
            MonLed  = 1;
        }
        else if (AvgSp < 88) {
            MonLed  = 0;
        }
      }
//----------------------------------------------------------------------------//
/*Side led is switched on if sidelight switch is on*/
void Side(){
        SideL   = SideSw;
     }
//----------------------------------------------------------------------------//
/*This routine if to select the operation of the Leds for indicating turning*/
void Turning(){
        while(true){
            if (LeftInd == 1 && RightInd == 0){
                IndL    = !IndL;
                IndR    = 0;
            }
            else if (LeftInd == 0 && RightInd == 1){
                IndL    = 0;
                IndR    = !IndR;
            }
            else if (LeftInd == 0 && RightInd == 0){
                IndL    = 0;
                IndR    = 0;
            }
            Thread::wait(1000);
        }
      }     
//----------------------------------------------------------------------------//     
/*Hazard lights for flashing at appropriate speed when both turn switches are on
  */
void Hazard(){
        while(true){
            IndL    = !IndL;
            IndR    = !IndR;
            Thread::wait(500);
        }
      }
//----------------------------------------------------------------------------//
/*Routine for checking if any turn switches are on. Selects which thread to run
  based on the turn switch values*/
void TurnHaz(){
        while(true){
            if(LeftInd == 1 && RightInd == 1){
                Hazard_thread.start(Hazard);
                Turning_thread.terminate();
            }
            else {
                Hazard_thread.terminate();
                Turning_thread.start(Turning);
            }
            Thread::wait(2000);
        }
      }
//----------------------------------------------------------------------------//
/*Routine for displaying the odometer and average speed on the LCD*/
void LCD(){
        
        
        if (c == 10){
            lcd->cls();
            c = 0;
        }

        lcd->locate(0,0); // set cursor to location (0,0) - top left corner
        lcd->printf("O=%.1f", Distance ); // print string
        lcd->locate(0,8);
        lcd->printf("AS=%d", AvgSp );
       
        c = c+1;
        
      }
//----------------------------------------------------------------------------//      
/*Routine that is used to cnotain other routines that are grouped based on 
  their required execution timings*/  
void Group_one(){
        while(true){
            Eng();
            BraInd();
            LCD();
            Thread::wait(500);
        }
      }
//----------------------------------------------------------------------------//
/*Routine for grouping together other routines that run at the same frequency*/
void Group_two(){
        while(true){
            Monitor();
            Side();
            Thread::wait(1000);
        }
      }
//----------------------------------------------------------------------------//
/*This routine is used for sending data to the mail queue*/
void send_mail () {
        while (true) {
            mail_mutex.lock();
            mail_t *mail = mail_box.alloc();
            mail->Speed = AvgSp; 
            mail->Accel = Acc;
            mail->Brake = Brake;
            mail_box.put(mail);
            Write++;
            mail_mutex.unlock();
            Thread::wait(5000);
        }
      }
//----------------------------------------------------------------------------//
/*This routine dumps the queued values over the serial port to the Pc*/
void print_mail (){
        while(true){
           mail_mutex.lock();
            while (Write > Print) {
                osEvent evt = mail_box.get();
                if (evt.status == osEventMail) {
                mail_t *mail = (mail_t*)evt.value.p;
                pc.printf("S:%d,A:%.1f,B:%.1f\n ",mail->Speed,mail->Accel,mail->Brake);
                mail_box.free(mail); 
            }
            Print++;
            }
           mail_mutex.unlock();
        Write = 0;
        Print = 0;
        Thread::wait(20000);
        }
      }
//----------------------------------------------------------------------------//
//Main Program Loop: Start the threads.
//----------------------------------------------------------------------------//
int main() {
    par_port = new MCP23017(p9, p10, 0x40); // initialise 16-bit I/O chip
    lcd = new WattBob_TextLCD(par_port); // initialise 2*26 char display
    par_port->write_bit(1,BL_BIT); // turn LCD backlight ON
    pc.baud(115200); //Set Baudrate to 115200
    
    AccBrk_thread.start(AccBrk);
    Group_1_thread.start(Group_one);
    Group_2_thread.start(Group_two);
    Sim_thread.start(Speed);
    TurnHaz_thread.start(TurnHaz);
    Send_mail_thread.start(send_mail);
    Print_mail_thread.start(print_mail);
    
        while(1){
     
        }
   }
