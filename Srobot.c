////////////////////////////////////
// clock AND protoThreads configure!
// You MUST check this file!
#include "config_1_2_3.h"
// threading library
#include "pt_cornell_1_2_3.h"
// yup, the expander


////////////////////////////////////
// graphics libraries
// SPI channel 1 connections to TFT
#include "tft_master.h"
#include "tft_gfx.h"
// need for rand function
#include <stdlib.h>
// need for sin function
#include <math.h>

//**********define the frequencies and declare variables*****
#define	SYS_FREQ 40000000
#define ISR_FREQ 1000

#define DAC_config_chan_A 0b0011000000000000
#define DAC_config_chan_B 0b1011000000000000

#define PWM_Period 22

static int timerlimit,  entry_time, execution_time, isr_time;
static int pwm_on_time_wheel1 =0,pwm_on_time_wheel2=0, pwm_on_time_idle=0,pwm_on_time_max=0,pwm_on_time_min=0;
static int pwm_on_time_1 =0, pwm_on_time_2=0;
static int val1=0,val2=0,val3=0;
static int kp=1,kd=3;
static int proportional_cntl_1=0,differential_cntl_1=0,integral_cntl_1=0;
static int proportional_cntl_2=0,differential_cntl_2=0,integral_cntl_2=0;
static int int_time=0;
static float ki=0.01;
static int sys_time_seconds=0;
static int tdoa1[10], tdoa2[10];
static int tdoa1_error=0, tdoa1_last_error=0;
static int tdoa2_error=0, tdoa2_last_error=0;
static int tdoa1_cmd=0, tdoa2_cmd=0, tdoa1_threshold=0;
static struct pt pt_display,  pt_timer;
char buffer[50];
static float speed_sound =344;
static float mic_distance =0.16;
static float theta=0;
static float theta1=0;
#define MAX_DELAY 450
void printLine(int line_number, char* print_buffer, short text_color, short back_color){
    // line number 0 to 31 
    /// !!! assumes tft_setRotation(0);
    // print_buffer is the string to print
    int v_pos;
    v_pos = line_number * 10 ;
    // erase the pixels
    tft_fillRoundRect(0, v_pos, 239, 8, 1, back_color);// x,y,w,h,radius,color
    tft_setTextColor(text_color); 
    tft_setCursor(0, v_pos);
    tft_setTextSize(1);
    tft_writeString(print_buffer);
}

static int time=0, time1=0, time2=0, time3=0, time12=0,time23=0,time31=0;

static PT_THREAD (protothread_display(struct pt *pt))
{
     PT_BEGIN(pt);
  
  
    sprintf(buffer,"time1 = %d", time1);
    printLine(8, buffer, ILI9340_WHITE, ILI9340_BLUE);
    sprintf(buffer,"time2 = %d", time2);
    printLine(10, buffer, ILI9340_WHITE, ILI9340_BLUE);
    sprintf(buffer,"time3 = %d", time3);
    printLine(12, buffer, ILI9340_WHITE, ILI9340_BLUE);
    sprintf(buffer,"time12 = %d", time12);
    printLine(14, buffer, ILI9340_WHITE, ILI9340_BLUE);
    sprintf(buffer,"time23 = %d", time23);
    printLine(16, buffer, ILI9340_WHITE, ILI9340_BLUE);
    sprintf(buffer,"time31 = %d", time31);
    printLine(18, buffer, ILI9340_WHITE, ILI9340_BLUE);
    sprintf(buffer,"theta = %6.3f", theta);
    printLine(20, buffer, ILI9340_WHITE, ILI9340_BLUE);
    sprintf(buffer,"theta1 = %6.3f", theta1);
    printLine(22, buffer, ILI9340_WHITE, ILI9340_BLUE);
    sprintf(buffer,"sys time = %d", sys_time_seconds);
    printLine(24, buffer, ILI9340_WHITE, ILI9340_BLUE);
    PT_YIELD_TIME_msec(1000) ;
  
    PT_END(pt);

    
}


static PT_THREAD (protothread_timer(struct pt *pt))
{
    PT_BEGIN(pt);
   
        PT_YIELD_TIME_msec(1000) ;
        sys_time_seconds++ ;
     //  if(int_time-sys_time_seconds>2)
      //   INTEnable(INT_IC4,1);

      //  tft_fillScreen(ILI9340_BLACK);
    
    
  PT_END(pt);

}

//**************Interrupt Service Routine****************

void calculate(){

time12=time1-time2;
time23=time2-time3;
time31=time3-time1;

if(time3<=time1 && time3<=time2){
    if(time1<time2){
       
        time =time23*1.6;
        if(time<MAX_DELAY)
        theta= 60 +  ((float) asin ( (double) (( time * (float) speed_sound ) / ( (float) mic_distance * (float) 1000000)) ) * ( (float) 180 / 3.14 ));
        
    }
    else{
        time =-time31*1.6;
        if(time<MAX_DELAY)
        theta= - ((float) asin ( (double) (( time * (float) speed_sound ) / ( (float) mic_distance * (float) 1000000)) ) * ( (float) 180 / 3.14 ));
        
        
    }
}
else if(time2<=time1 && time2<=time3){
    if(time1<time3){
        time =-time23*1.6;
        if(time<MAX_DELAY)
        theta= 60+ ( (float) asin ( (double) (( time * (float) speed_sound ) / ( (float) mic_distance * (float) 1000000)) ) * ( (float) 180 / 3.14 ));
        
    }
    else{
        time =time12*1.6;
        if(time<MAX_DELAY)
        theta= ( (float) asin ( (double) (( time * (float) speed_sound ) / ( (float) mic_distance * (float) 1000000)) ) * ( (float) 180 / 3.14 ));
        
    
}
            }

else if(time1<=time2 && time1<=time3){
    if(time2<time3){
        time =time31*1.6;
        if(time<MAX_DELAY)
        theta= - ( (float) asin ( (double) (( time * (float) speed_sound ) / ( (float) mic_distance * (float) 1000000)) ) * ( (float) 180 / 3.14 ));
        
    }
    else{
        time =-time12*1.6;
        if(time<MAX_DELAY)
        theta= ( (float) asin ( (double) (( time * (float) speed_sound ) / ( (float) mic_distance * (float) 1000000)) ) * ( (float) 180 / 3.14 ));
        
    
}
                
                
                
}
    
    if(time12*1.6<MAX_DELAY && time12*1.6>-MAX_DELAY)
            theta1= ( (float) asin ( (double) (( time12 * 1.6*(float) speed_sound ) / ( (float) mic_distance * (float) 1000000)) ) * ( (float) 180 / 3.14 ));


    
   // SetDCOC3PWM(pwm_on_time_wheel1);
   // SetDCOC2PWM(pwm_on_time_wheel2);
 SetDCOC3PWM(timerlimit-pwm_on_time_max);
    SetDCOC2PWM(timerlimit-pwm_on_time_min);



     
} // end ISR TIMER2
/*

void __ISR(_EXTERNAL_4_VECTOR, ipl7) INT4Interrupt() 
{    mINT4ClearIntFlag();  
    INTEnable(INT_INT4,0);
//   WriteTimer3(0x000);
  
 
      
    // read the capture register 
   
       //  mIC4ReadCapture();

   // CloseCapture5();
//delay_ms(20);
WriteTimer3(0x000);   
val1=0;
val2=0;
val3=0;
int_time=sys_time_seconds;
INTEnable(INT_IC1,1);
INTEnable(INT_IC2,1);
INTEnable(INT_IC5,1);


} 
 */
 
void __ISR(_INPUT_CAPTURE_1_VECTOR, ipl3) C1Handler(void)
{      mIC1ClearIntFlag();
    INTEnable(INT_IC1,0);
    // read the capture register 
   if(val1==0){ 
    time1 = mIC1ReadCapture();
    CloseCapture1();
      val1=1;
   }
    if(1){
    //if(val1 && val2 && val3){
        delay_ms(20);
           INTEnable(INT_IC4,1);
          calculate();
          
      }
   
    // clear the timer interrupt flag
  
}

void __ISR(_INPUT_CAPTURE_2_VECTOR, ipl4) C2Handler(void)
{     mIC2ClearIntFlag();

    INTEnable(INT_IC2,0);
    // read the capture register 
       if(val2==0){ 
    time2 = mIC2ReadCapture();
    CloseCapture2();
     val2=1;
       }
      if(val1 && val2 && val3){
          delay_ms(20);
           INTEnable(INT_IC4,1);
          calculate();
          
      }

    // clear the timer interrupt flag
   
}


void __ISR(_INPUT_CAPTURE_5_VECTOR, ipl5) C5Handler(void)
{     mIC5ClearIntFlag();
  
    INTEnable(INT_IC5,0);
    // read the capture register 
      if(val3==0){ 
    time3 = mIC5ReadCapture();
    CloseCapture5();
     val3=1;
      }
  if(val1 && val2 && val3){
       delay_ms(20);
           INTEnable(INT_IC4,1);
          calculate();
          
      }

    // clear the timer interrupt flag
   
}

void __ISR(_INPUT_CAPTURE_4_VECTOR, ipl6) C4Handler(void)
{  mIC4ClearIntFlag();
    INTEnable(INT_IC4,0);
      
    // read the capture register 
   
         mIC4ReadCapture();

   // 
  
         delay_ms(100);
WriteTimer3(0x000);   
     val1=0;
val2=0;
val3=0; 
int_time=sys_time_seconds;
OpenCapture1(  IC_EVERY_RISE_EDGE | IC_INT_1CAPTURE | IC_TIMER3_SRC | IC_ON );
OpenCapture2(  IC_EVERY_RISE_EDGE | IC_INT_1CAPTURE | IC_TIMER3_SRC | IC_ON );
OpenCapture5(  IC_EVERY_RISE_EDGE | IC_INT_1CAPTURE | IC_TIMER3_SRC | IC_ON );
INTEnable(INT_IC1,1);
INTEnable(INT_IC2,1);
INTEnable(INT_IC5,1);

}
void main(){

  ANSELA = 0; ANSELB = 0; 

  // === config threads ==========
  // turns OFF UART support and debugger pin, unless defines are set
  PT_setup();

    

  
  // === setup system wide interrupts  ========
INTEnableSystemMultiVectoredInt();
timerlimit = SYS_FREQ* PWM_Period/256000;
pwm_on_time_idle=1.5*SYS_FREQ/256000;  
pwm_on_time_max=1.7*SYS_FREQ/256000;
pwm_on_time_min=1.3*SYS_FREQ/256000;
tdoa1_threshold=SYS_FREQ/100000;

OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_256, timerlimit);
OpenTimer3(T3_ON | T3_SOURCE_INT | T3_PS_1_64, 0xffff);

    // Need ISR to compute PID controller 
//ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);     // set up compare3 for PWM mode 
OpenOC3(OC_ON | OC_TIMER2_SRC | OC_PWM_FAULT_PIN_DISABLE , pwm_on_time_wheel1, pwm_on_time_wheel1);
OpenOC2(OC_ON | OC_TIMER2_SRC | OC_PWM_FAULT_PIN_DISABLE , pwm_on_time_wheel2, pwm_on_time_wheel2);
// OC3 is PPS group 4, map to RPB9 (pin 18) 
PPSOutput(4, RPB9, OC3);// disconnect port expander
PPSOutput(2, RPB8, OC2);



  // turn on the interrupt so that every capture can be recorded

  // connect PIN 24 to IC1 capture unit
PPSInput(3, IC1, RPA4);
PPSInput(4, IC2, RPA3);
PPSInput(3, IC5, RPA2);
//PPSInput(1, INT4, RPB3);
PPSInput(1, IC4, RPB3);
  // init the display

OpenCapture4(  IC_EVERY_FALL_EDGE | IC_INT_1CAPTURE | IC_TIMER3_SRC | IC_ON );
ConfigIntCapture4(IC_INT_ON | IC_INT_PRIOR_6 | IC_INT_SUB_PRIOR_3 );
INTClearFlag(INT_IC4);

    
ConfigIntCapture1(IC_INT_OFF | IC_INT_PRIOR_3 | IC_INT_SUB_PRIOR_3 );
INTClearFlag(INT_IC1);
    
ConfigIntCapture2(IC_INT_OFF | IC_INT_PRIOR_4 | IC_INT_SUB_PRIOR_3 );
INTClearFlag(INT_IC2);
    
ConfigIntCapture5(IC_INT_OFF | IC_INT_PRIOR_5 | IC_INT_SUB_PRIOR_3 );
INTClearFlag(INT_IC5);
    // clear the timer interrupt flag

//ConfigINT4(EXT_INT_PRI_7 | RISING_EDGE_INT | EXT_INT_ENABLE);
//INTClearFlag(INT_INT4);
  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9340_BLACK);
   PT_INIT(&pt_timer);
  PT_INIT(&pt_display);
  
while (1){
      PT_SCHEDULE(protothread_timer(&pt_timer));
      PT_SCHEDULE(protothread_display(&pt_display));
    
  }
    
}
}