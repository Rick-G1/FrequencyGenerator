/******************************************************************************/
/*                                                                            */
/*            FrequencyGenerator -- Frequency Generator Module                */
/*                                                                            */
/*                     Copyright (c) 2021  Rick Groome                        */
/*                                                                            */
/* Permission is hereby granted, free of charge, to any person obtaining a    */
/* copy of this software and associated documentation files (the "Software"), */
/* to deal in the Software without restriction, including without limitation  */
/* the rights to use, copy, modify, merge, publish, distribute, sublicense,   */
/* and/or sell copies of the Software, and to permit persons to whom the      */
/* Software is furnished to do so, subject to the following conditions:       */
/*                                                                            */
/* The above copyright notice and this permission notice shall be included in */
/* all copies or substantial portions of the Software.                        */
/*                                                                            */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    */
/* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    */
/* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        */
/* DEALINGS IN THE SOFTWARE.                                                  */
/*                                                                            */
/*  PROCESSOR:  ATmega32U4       COMPILER: Arduino/GNU C for AVR Vers 1.8.5   */ 
/*  Written by: Rick Groome 2021                                              */ 
/*                                                                            */
/******************************************************************************/

/*
  This module implements a variable frequency generator using Timer 4 on an 
  Arduino Pro Micro Module (using an ATMega32U4).   
  
  Output is on Arduino Digital pin 5 [PC6] (using OC4A-) for Pro Micro.
  (Could also output on Arduino Digital pin 10 [PB6] (using OC4B). 

  This module is specifically for the timer 4 module of the ATMEGA32U4 on a 
  Pro Micro device (or other microcontroller devices with similar functional 
  blocks) assuming the controller is running as a USB device with the PLL set 
  at 96MHz and a crystal of 16MHz (This is the default for the Pro Micro 
  device).  It could possibly be reworked for other timers, but the resolution 
  would be less.

  The function 'FrequencyGenerator::set' accepts a long integer that is the 
  frequency to output.  This may range from less than 0 (return current 
  frequency) to 0 (generator off) to some over 1/2 the clock frequency of 
  the micro (8MHz).  (It has been observed  to work to about 12MHz).  It works 
  by selecting the best PLL multiplier, counter prescaler and count value by 
  trying each of 3 possible PLL multipliers and looking for the error between 
  the desired and actual output frequency obtainable with the calculated PLL 
  multiplier, prescale value and count value.  Once the closest combination 
  is determined, the hardware is set up to those values.  Using this 
  algorithm, the output will be as close as possible to the desired frequency.  
  The duty cycle of the output will be 50%.  The function returns either the 
  frequency being output or -1 if the new requested frequency could not be set.
 
  Since the timer is set up to automatically reload, no interrupts or other 
  software overhead is required -- Just call the function and then the 
  hardware will produce the output frequency with no additional intervention. 

  As mentioned above, the output frequency will be the closest value to the 
  desired frequency obtainable with the hardware (without further 
  intervention).  It may not be exactly the same frequency as the frequency 
  set.  To determine the actual output frequency, the call to 
  'FrequencyGenerator::read' will return the actual frequency the hardware 
  is set to (This is also the value returned when calling 
  'FrequencyGenerator::set').  

  While the basic user interface is via a class, only a single instance 
  should be declared as this module uses specific hardware resources. 
  
  The timebase used for the generator is the micros clock, which is normally 
  a crystal oscillator with its inherent accuracy and stability, but since it 
  is not calibrated it will normally vary from its nominal frequency of 16MHz 
  by a few Hertz.  This is typically within about 0.1 to 0.2%, but could be 
  more than that depending on the exact Pro Micro module used.  Since the 
  input frequency can vary some, the output will vary by the same percentage.  
  It is possible to rework the input crystal frequency to include a trimmer 
  capacitor and then tune it to exactly 16MHz.  An alternate is to create a 
  compensation value and then apply this compensation value to any value input 
  so that the actual output frequency is correct.

  If it is desired to "trim" the Pro Micro's 16MHz oscillator to count or 
  generate frequencies that are more accurate, a Knowles Voltronics JR400 
  trimmer capacitor (8-40pF) (or similar) can be used to replace the C2 
  capacitor on the module.  (You could also replace C2 with a 10pF cap and use 
  a JR200 (4.5-20pF) for a more stable adjustment with less range).  This 
  trimmer capacitor can be placed on top of the 32U4 chip and tiny wires 
  (wire wrap?) used to connect the two connections of it to the Pro micro 
  module.  Connect the rotor of the trimmer cap to GND (the '-' side of C19 is 
  a good place) and the other side of the trimmer cap to the non-GND side of C2.  
  If the rotor side of the trimmer cap is connected to the C2 connection, the 
  oscillator will not be as stable. Once wired, glue the trimmer cap to the top
  of the 32U4 chip.  Then set the frequency generator of the chip this is 
  programmed on to 4MHz and using a reference frequency counter, tune the 
  trimmer capacitor to exactly 4MHz. If frequency counter only is programmed, 
  inject a 4MHz signal into the frequency counter input and tune to exactly 4MHz. 

  The repository also includes a documentation file that provides more details 
  on the use of this library and also covers a companion frequency counter 
  library.
  
*/

/*
Revision log: 
  1.00  2-5-21    REG   
    Initial implementation

*/

#include <arduino.h>
#include "FrequencyGenerator.h"

//#define FRQGENUSEPB6  1       // Define to use  PB6 (Arduino pin 10)(using OC4B) instead
//#define FRQGENDEBUG   1       // For debug... show counter values. 

#if FRQGENDEBUG
#define printfROM(fmt, ...)   printf_P(PSTR(fmt),##__VA_ARGS__)
#endif

#if !(defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__))
#error "This module (FrequencyGenerator.cpp) only supports ATMega32U4/16U4"
#endif


#if 0
byte _pin;
FrequencyGenerator::FrequencyGenerator(int pin) { pinMode(pin, OUTPUT);  _pin = pin; }
{
  _FreqGenVal=0; _pin=10;
}
#endif

long FrequencyGenerator::read(void)  
  //  Return the current setting of the frequency generator.
{ 
  return _FreqGenVal; 
}


long FrequencyGenerator::set(long Freq)
  // Set Timer4 to the frequency specified by 'Freq' (if 'Freq' > 0), shut off 
  // frequency generator (if 'Freq' is 0) or return current frequency (if 'Freq'
  // < 0).  Output the frequency on PC6 (Arduino pin 5) or alternatively on PB6 
  // (Arduino pin 10). 
  // NOTE: Frequency output may not match specified frequency but will be as 
  // close to it as is possible for the system clock speed.  Max frequency is 
  // crystal clock rate.  
  // Function returns current frequency if ok or -1 if unable to set to the 
  // desired frequency. 
{
  byte pll,lg,svPLL=0,svLG=0; unsigned PS,cnt,svCNT=0;  long CK, CV,dif,svDIF=0; 
  static byte CKM[]={1,6,4,3};  // Clock pll multipliers (16,96,64,48 Mhz)

  if (Freq<0L) goto DONE; 
  if (!Freq) _FreqGenVal=0;  else
  {
    svDIF=0x7FFFFFFFL; 
    for (pll=0; pll<sizeof(CKM); pll++)
    {
      CK=F_CPU*CKM[pll];          // Clock freq for this pll setting
      CV=CK / Freq / 1024; 
      // Find the log2 of CV
      // From this routine:  for (val = 0; n > 1; val++, n >>= 1);
      // Modified with "if n=0 return 0" and "return Val+1"
      lg=0; if (CV != 0) 
      {
        while (CV > 1) { lg++;  CV=CV>>1; }
        lg++;
      }
      // If pll==1 (96MHz) then ignore this PLL value (counter can't run @96MHz)
      // If the lg2(CV) is out of range of prescaler, ignore this CLK value
      if (pll==1  || lg > 14) continue;              
      // Create the prescaler value and the count value
      PS=1<<lg;  cnt=((CK*2/PS/Freq)+1)/2;
      // If cnt is too small or too big, ignore this clock value  
      //   (NOTE: OCR4C min value is 3.  See data sheet!)
      if (cnt<4 || cnt>0x3FF) continue;   
      // Calculate the difference between the desired frequency and the 
      // actual frequency these divisors will produce.
      // Instead of dividing clock down by the PS/count/freq, multiply PS,cnt,
      // freq and then subtract from  clk... Then divide by the pll scale.  
      // Then absolute.  The resultant integer is the difference in lots of 
      // counts (eg the most precision we can do with ints).  Save/compare this 
      // integer value to figure out which setting is the closest to the 
      // desired frequency.
      dif=(CK - ((long)PS*(long)cnt*Freq));  dif=dif/((long)CKM[pll]);  
      if (dif<0) dif=-dif;
      // Note: the below doesn't work (using freq 1050000).. longdiv routine blows up... so do it like above instead (it works). 
      // dif=(CK - ((long)PS * (long)cnt * Freq))/(long)CKM[pll];  if (dif<0) dif = -dif;
#if FRQGENDEBUG
      CV=(((CK*2)/((long)PS*(long)cnt))+1)/2;  // frequency
      printfROM("CLK=%dM  Pll=%d PS=%-5d Cnt=%-4d Freq=%-8ld Er=%ld\n",
                (int)(CK/1000000), pll,(1<<lg) ,cnt, CV, dif); // CKM[pll]);
#endif
      // If this is the smallest error value then save these settings 
      if (dif<svDIF) { svPLL=pll; svLG=lg; svCNT=cnt; svDIF=dif; }
      // If this is the smallest error value or if err is same and the prescale 
      // value is greater than the current prescale value
      // if (dif<svDIF || (dif==svDIF && lg>svLG)) 
      // { svPLL=pll; svLG=lg; svCNT=cnt; svDIF=dif; }
    }
    if (svDIF<0x7FFFFFFFL)
    {
      // Calculate the actual frequency output
      //   For integer frequency Mult clk *2 then do calc, then add 1, then 
      //   div 2. This gives an output frequency that is (Freq+0.5) then trunc 
      //   to whole number.   
      //   This makes 0.51 output a 1. (eg. an integer "round" function)
      _FreqGenVal=((((F_CPU*CKM[svPLL])*2)/((long)(1<<svLG)*(long)svCNT))+1)/2;
#if FRQGENDEBUG
      printfROM("Selectd: Pll=%d PS=%-5d Cnt=%-4d Freq=%-8ld\n",
                svPLL,(1<<svLG),svCNT,_FreqGenVal);  
#endif
    }
    else  // We never found a valid set of divisors, get out
    {
#if FRQGENDEBUG
      printfROM(" ***  No divisors found  ***\n");   
#endif
      return -1; 
    }
  }
  //
  // Now _FreqGenVal is 0 for turn off or >0 for set new frequency
  // Set the timer registers to the calculated values
  //
  TCCR4D=0;             // Reset this to 0 (init() set it for PWM mode)
  if (!_FreqGenVal)     // Turn off, shut down timer.
  {
    // Shut off timer (if it's running) and release the IO 
    TCCR4A=0; TCCR4B=0;                 // Shut down the timer
    // turn off the IO bits
#if FRQGENUSEPB6
    pinMode(10,INPUT_PULLUP);           // PB6 and OC4B
#else
    pinMode(5, INPUT_PULLUP);           // PC6 and OC4A-
#endif
#if FRQGENDEBUG
    printfROM("Generator off\n"); 
#endif
  }
  else    // *****  Now set up the timer to the values calculated  *****
  {
    // Set IO bits as needed for output. Also set TCCR4A.
#if FRQGENUSEPB6
    pinMode(10, OUTPUT);                // PB6 and OC4B
    TCCR4A = (1<<PWM4B)|(1<<COM4B0);    // Toggle on compare match (COMP B out)
#else
    pinMode(5, OUTPUT);                 // PC6 and OC4A-
    TCCR4A = (1<<PWM4A)|(1<<COM4A0);    // Toggle on compare match (COMP A out)
#endif
    // Note: When just powering up module (no code download) then PLLFRQ is 
    // PDIV3:0=48MHz and PLLUSB=0 (/1).  After downloading code then PLLFRQ 
    // is PDIV3:0=96MHz and PLLUSB=1 (/2)  This causes all frequencies that 
    // use PLLFRQ to be 1/2 of expected values (if no code download).  
    //PLLFRQ=(PLLFRQ & ~(0x30))|((cpll&3)<<4); // Set PLLFRQ to input clock we want.
    // Use this instead... (force 96MHz /2 mode)
    PLLFRQ = 0x4A | ((svPLL&3)<<4);       // Set PLLFRQ to input clock we want.
    // Now set OCR4C and either OCR4A or OCR4B
    PS=(svCNT/2)-1;     // set OCR4A/B to (1/2 of svCNT)-1 for 50% duty cycle
    svCNT-=1;  // Dont forget to subtract 1 from the count loaded into OCR4C !!
    TCNT4H /*upper OCR4C*/ =(svCNT>>8); OCR4C=(svCNT&0xFF); // set counter TOP value
#if FRQGENUSEPB6
    // Set OCR4b to a value.  set to (1/2 of svCNT value)-1 for 50%;  
    TCNT4H /*upper OCR4B*/ =(PS>>8); OCR4B=(PS&0xFF); 
#else
    // Set OCR4a- to a value.  set to (1/2 of svCNT value)-1 for 50%;  
    TCNT4H /*upper OCR4A*/ =(PS>>8); OCR4A=(PS&0xFF); 
#endif
    // Finally set set prescaler and run 
    TCCR4B=(svLG+1)&0xF; 
#if FRQGENDEBUG
    cnt=OCR4C; cnt=cnt | (TCNT4H<<8);
    printfROM("PLLFRQ=0x%X, TCCRB=%d, OCRC=%d, CK=%d, PS=%d, cnt=%d, OCRA=%d\n",
              PLLFRQ, TCCR4B&0xF, cnt, (unsigned)((F_CPU*CKM[svPLL])/1000000),
              ((unsigned)1)<<((unsigned) ((TCCR4B&0xF)-1)), svCNT,
              ((OCR4A)|(TCNT4H<<8)) );   
#endif
  }
DONE:  
  return _FreqGenVal; 
}



