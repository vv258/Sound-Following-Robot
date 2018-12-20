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

static int kp=1,kd=3;
static int proportional_cntl_1=0,differential_cntl_1=0,integral_cntl_1=0;
static int proportional_cntl_2=0,differential_cntl_2=0,integral_cntl_2=0;

static float ki=0.01;
static int sys_time_seconds=0;
static int tdoa1[10], tdoa2[10];
static int tdoa1_error=0, tdoa1_last_error=0;
static int tdoa2_error=0, tdoa2_last_error=0;
static int tdoa1_cmd=0, tdoa2_cmd=0, tdoa1_threshold=0;
static struct pt pt_display,  pt_timer;
char buffer[50];

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

static int time1, time2, time3, time12,time13;

static PT_THREAD (protothread_display(struct pt *pt))
{
     PT_BEGIN(pt);
  
    sprintf(buffer,"Proportional gain= %d", kp);
    printLine(0, buffer, ILI9340_WHITE, ILI9340_BLUE);
    sprintf(buffer,"Integral gain= %6.4f", ki);
    printLine(2, buffer, ILI9340_WHITE, ILI9340_BLUE);
    sprintf(buffer,"Differential gain= %d", kd);
    printLine(4, buffer, ILI9340_WHITE, ILI9340_BLUE);
    sprintf(buffer,"ISR TIME= %d", isr_time);
    printLine(6, buffer, ILI9340_WHITE, ILI9340_BLUE);
    sprintf(buffer,"time1 = %d", time1);
    printLine(8, buffer, ILI9340_WHITE, ILI9340_BLUE);
    sprintf(buffer,"time2 = %d", time2);
    printLine(10, buffer, ILI9340_WHITE, ILI9340_BLUE);
    sprintf(buffer,"time3 = %d", time3);
    printLine(12, buffer, ILI9340_WHITE, ILI9340_BLUE);
    sprintf(buffer,"time12 = %d", time12);
    printLine(14, buffer, ILI9340_WHITE, ILI9340_BLUE);
    sprintf(buffer,"time13 = %d", time13);
    printLine(16, buffer, ILI9340_WHITE, ILI9340_BLUE);
    sprintf(buffer,"sys time = %d", sys_time_seconds);
    printLine(18, buffer, ILI9340_WHITE, ILI9340_BLUE);
    PT_YIELD_TIME_msec(100) ;
  
    PT_END(pt);

    
}


static PT_THREAD (protothread_timer(struct pt *pt))
{
    PT_BEGIN(pt);
   
        PT_YIELD_TIME_msec(1000) ;
        sys_time_seconds++ ;
    

      //  tft_fillScreen(ILI9340_BLACK);
    
    
  PT_END(pt);

}

//**************Interrupt Service Routine****************

void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void){
entry_time= ReadTimer2();                  // time required to enter the ISR
mT2ClearIntFlag();                                 // clear the interrupt flag
     //code for calculating PID values

time12=time1-time2;
time13=time1-time3;
   
     int i;
    for(i=9;i>0;i--){
        tdoa1[i]=tdoa1[i-1];
        tdoa2[i]=tdoa2[i-1];

    }
    tdoa1[0]=time12;
    tdoa2[0]=time13;

    
    tdoa1_last_error = tdoa1_error;
    tdoa1_error = tdoa1_cmd - tdoa1[0];
    
    tdoa2_last_error = tdoa2_error;
    tdoa2_error = tdoa2_cmd - tdoa2[0];

    

    

    if(tdoa1_error>tdoa1_threshold){
            proportional_cntl_1 = kp * tdoa1_error ;
            differential_cntl_1 = kd * (tdoa1_error - tdoa1_last_error); 
            integral_cntl_1 = integral_cntl_1 + tdoa1_error ;
            
            if((tdoa1_error<0 && tdoa1_last_error>0) ||(tdoa1_error>0 && tdoa1_last_error<0))
                    integral_cntl_1=90*integral_cntl_1/100;

            pwm_on_time_1 =  proportional_cntl_1 + differential_cntl_1 + ki * integral_cntl_1 ;
            pwm_on_time_wheel1=pwm_on_time_idle + pwm_on_time_1;
            pwm_on_time_wheel2=pwm_on_time_idle + pwm_on_time_1;

    }
    
    else{
            proportional_cntl_2 = kp * tdoa2_error ;
            differential_cntl_2 = kd * (tdoa2_error - tdoa2_last_error); 
            integral_cntl_2 = integral_cntl_2 + tdoa2_error ;
            
            if((tdoa2_error<0 && tdoa2_last_error>0) ||(tdoa2_error>0 && tdoa2_last_error<0))
                    integral_cntl_2=90*integral_cntl_2/100;
            
            pwm_on_time_2 =  proportional_cntl_2 + differential_cntl_2 + ki * integral_cntl_2 ;
            pwm_on_time_wheel1=pwm_on_time_idle + pwm_on_time_1;
            pwm_on_time_wheel2=pwm_on_time_idle - pwm_on_time_1;
        
    }
    
    if(pwm_on_time_wheel1<pwm_on_time_min)
        pwm_on_time_wheel1=pwm_on_time_min;
    else if(pwm_on_time_wheel1>pwm_on_time_max)
        pwm_on_time_wheel1=pwm_on_time_max;

        
    if(pwm_on_time_wheel2<pwm_on_time_min)
        pwm_on_time_wheel2=pwm_on_time_min;
    else if(pwm_on_time_wheel2>pwm_on_time_max)
        pwm_on_time_wheel2=pwm_on_time_max;
    
   // SetDCOC3PWM(pwm_on_time_wheel1);
   // SetDCOC2PWM(pwm_on_time_wheel2);
 SetDCOC3PWM(timerlimit-pwm_on_time_max);
    SetDCOC2PWM(timerlimit-pwm_on_time_min);


execution_time = ReadTimer2() ; //  execution time for ISR
isr_time= entry_time+ execution_time;
     
} // end ISR TIMER2
void __ISR(_EXTERNAL_3_VECTOR, ipl7) INT3Interrupt() 
{  //
//   WriteTimer3(0x000);
    mINT3ClearIntFlag(); 
   delay_ms(20);
   
} 
void __ISR(_INPUT_CAPTURE_1_VECTOR, ipl3) C1Handler(void)
{
    // read the capture register 
    time1 = mIC1ReadCapture();
      mIC1ClearIntFlag();
  //  CloseCapture1();
    // clear the timer interrupt flag
  
}

void __ISR(_INPUT_CAPTURE_2_VECTOR, ipl4) C2Handler(void)
{
    // read the capture register 
    time2 = mIC2ReadCapture();
     mIC2ClearIntFlag();
   // CloseCapture2();

    // clear the timer interrupt flag
   
}


void __ISR(_INPUT_CAPTURE_5_VECTOR, ipl5) C5Handler(void)
{       
    // read the capture register 
    time3 = mIC5ReadCapture();
     mIC5ClearIntFlag();
   // CloseCapture5();

    // clear the timer interrupt flag
   
}

void __ISR(_INPUT_CAPTURE_4_VECTOR, ipl2) C4Handler(void)
{       
    // read the capture register 
     mIC4ClearIntFlag();
         mIC4ReadCapture();

   // CloseCapture5();
//WriteTimer3(0x000);
    // clear the timer interrupt flag
   
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
OpenTimer3(T3_ON | T3_SOURCE_INT | T3_PS_1_1, 0xffff);

    // Need ISR to compute PID controller 
ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);     // set up compare3 for PWM mode 
OpenOC3(OC_ON | OC_TIMER2_SRC | OC_PWM_FAULT_PIN_DISABLE , pwm_on_time_wheel1, pwm_on_time_wheel1);
OpenOC2(OC_ON | OC_TIMER2_SRC | OC_PWM_FAULT_PIN_DISABLE , pwm_on_time_wheel2, pwm_on_time_wheel2);
// OC3 is PPS group 4, map to RPB9 (pin 18) 
PPSOutput(4, RPB9, OC3);// disconnect port expander
PPSOutput(2, RPB8, OC2);

OpenCapture1(  IC_EVERY_RISE_EDGE | IC_INT_1CAPTURE | IC_TIMER3_SRC | IC_ON );
   OpenCapture2(  IC_EVERY_RISE_EDGE | IC_INT_1CAPTURE | IC_TIMER3_SRC | IC_ON );
   OpenCapture5(  IC_EVERY_RISE_EDGE | IC_INT_1CAPTURE | IC_TIMER3_SRC | IC_ON );
    OpenCapture4(  IC_EVERY_FALL_EDGE | IC_INT_1CAPTURE | IC_TIMER3_SRC | IC_ON );
   ConfigIntCapture1(IC_INT_ON | IC_INT_PRIOR_1 | IC_INT_SUB_PRIOR_3 );
INTClearFlag(INT_IC1);
    
ConfigIntCapture2(IC_INT_ON | IC_INT_PRIOR_2 | IC_INT_SUB_PRIOR_3 );
INTClearFlag(INT_IC2);
    
ConfigIntCapture5(IC_INT_ON | IC_INT_PRIOR_3 | IC_INT_SUB_PRIOR_3 );
INTClearFlag(INT_IC5);
ConfigIntCapture4(IC_INT_ON | IC_INT_PRIOR_4 | IC_INT_SUB_PRIOR_3 );
INTClearFlag(INT_IC4);
  // turn on the interrupt so that every capture can be recorded

  // connect PIN 24 to IC1 capture unit
PPSInput(3, IC1, RPA4);
PPSInput(4, IC2, RPA3);
PPSInput(3, IC5, RPA2);
//PPSInput(2, INT3, RPA1);
PPSInput(1, IC4, RPB3);

//ConfigINT3(EXT_INT_PRI_7 | FALLING_EDGE_INT | EXT_INT_ENABLE);

int k;
    for(k=0;k<10;k++){
        tdoa1[k]=0;
        tdoa2[k]=0;
    }
    
    
  PT_INIT(&pt_timer);
  PT_INIT(&pt_display);
    
  // init the display
  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9340_BLACK);
  

while (1){
      PT_SCHEDULE(protothread_timer(&pt_timer));
      PT_SCHEDULE(protothread_display(&pt_display));
    
  }
    
}