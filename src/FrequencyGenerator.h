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

// See .cpp file for a description of this module and it's functions.

#ifndef _FREQGEN_H
#define _FREQGEN_H

class FrequencyGenerator
{
  public:
    long set(long Freq);
      // Set Timer4 to the frequency specified by 'Freq' (if 'Freq' > 0), shut off 
      // frequency generator (if 'Freq' is 0) or return current frequency (if 'Freq'
      // < 0).  Output the frequency on PC6 (Arduino pin 5) or alternatively on PB6 
      // (Arduino pin 10). 
      // NOTE: Frequency output may not match specified frequency but will be as 
      // close to it as is possible for the system clock speed.  Max frequency is 
      // crystal clock rate.  
      // Function returns current frequency if ok or -1 if unable to set to the 
      // desired frequency. 

    long read(); 
      //  Return the current setting of the frequency generator.

  private:
    long _FreqGenVal=0;
//    int _pin;
};

#endif    // _FREQGEN_H
