/******************************************************************************/
/*                                                                            */
/*     FreqGenCtrApp -- Frequency Generator/Frequency Counter Test Module     */
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

   This is a main program to test the frequency generator and frequency 
   counter functions.  It can be configured to function based on input commands 
   from a comport, or alternatively with a LCD display and four buttons to be
   used in a stand-alone mode. 
   
   If "COMIF" is defined below, it uses either the USB virtual comport or a 
   conventional com port to set and read the frequency generator and frequency 
   counter module. 

   It uses a simple command line interpreter that looks for these commands:

        G=<freq><CR>  Set frequency generator to 'freq'.
                      <freq> is 0..20000000 (0=off)
        G<CR>         Get currently set generator frequency.
  
        F<CR>         Read the frequency counter.
        F1<CR>        To wait for the next reading and then read the 
                      frequency counter.
        FS<CR>        See if a new frequency counter value is ready. (1=yes,0=no)
        T[0..9]<CR>   Set the frequency counter gate time to off...10sec,ext
                      0=off, 1=1sec, 2=10mS, 3=100mS, 4=10S, 5=100S, 6=ext                      
                      7=Period mode, 8=Period(10 avg), 9=Period(100 avg)
        T<CR>         Get the current gate time (returned value same as set value)
        R<CR>         Turn on/off automatic reporting of the frequency read by 
                      the frequency counter.  

        ?             Show help info.

   If "FREEIF" is defined below, then four push buttons are used to set the 
   frequency counter mode and set the generator's output frequency.  Two of the 
   buttons are frequency generator frequency up and down.  The other two buttons
   change the frequency counter mode up and down.  

   If "HASLCD" is defined below, a 16 x 2 LCD display is used to display the 
   frequency from the frequency counter and frequency output from the generator. 

   Either or both of "COMIF" and "FREEIF" can be defined allowing use as both 
   a com port driven device and/or a stand alone device.
   
   If "FREEIF" is defined then "HASLCD" is automatically defined, presuming 
   the frequency count will want to be seen.

   This module is not needed to use the frequency generator or frequency 
   counter module (FrequencyGenerator.cpp or FrequencyCounter.cpp). 
  
*/

/* 
Revision log: 
  1.0.0    2-5-21    REG   
    Initial implementation

*/

#define VERSION     "1.0.0"

//******************************************************************************
//*                        User configurable defines                           *
//******************************************************************************

#define FREQCTR     1           // define as non-zero for freq counter 
#define FREQGEN     1           // define as non-zero for freq generator 

#define COMIF       1           // define for COM port interface
#define FREEIF      1           // define for Free-standing device interface

#define HASLCD      1           // define if LCD display is attached

#define COMM        Serial      // "Serial" for USB or "Serial1" for UART
//#define COMM        Serial1      // "Serial" for USB or "Serial1" for UART
#define COMMBAUD    19200       // Baudrate if UART. 

#define LED         2           // define to blink digital pin 2 in main


#if FREQGEN
#include "FrequencyGenerator.h" 
FrequencyGenerator FG; 
#endif

#if FREQCTR
#include "FrequencyCounter.h"
FrequencyCounter FC; 
#endif


#if FREEIF
// Define I/O bits used for each of the four switches. 
#if FREQCTR
#define FCBUTTONUP  7             // advance freq ctr to next gate time
#define FCBUTTONDN  8             // retard  freq ctr to previous gate time
#endif
#if FREQGEN
#define GBUTTONUP   3             // advance freq gen to next frequency
#define GBUTTONDN   4             // retard  freq gen to previous frequency
#endif
#undef HASLCD
#define HASLCD      1             // force HASLCD to be active when using FREEIF
#endif

#if HASLCD
#include <LiquidCrystal.h>
// User interface via 16x2 LCD display and two/four pushbuttons  (HDM16216)
//     XXXXXXXXXXXXXXXX
//     FrequencyCounter
//       Test Program
//     XXXXXXXXXXXXXXXX
//        12345678  Hz  
//      1234.56789  Hz  
//     XXXXXXXXXXXXXXXX
//      Gate: 1 Sec (3)
//      Gate: 0.1S  (3)
//      Gate: EXT   (6)
//      #Avgs: 100  (9)
//     XXXXXXXXXXXXXXXX
//     Gen=2000000  Hz
//     XXXXXXXXXXXXXXXX
LiquidCrystal Lcd(16,14, 15,18,19,20);      // rs,enable, d4,d5,d6,d7
#else 
#define HASLCD      0             // HASLCD must be 0 or 1 for code below
#endif

//******************************************************************************
//*      Code to implement printf via the USB virtual or UART serial port.     *
//******************************************************************************
#if COMIF
int COM1putter( char c, FILE *t __attribute__((unused))) 
{ 
  if (c=='\n') COMM.write('\r');   
  COMM.write(c);   return 1; 
}
// This is a hack to get around C++ not being able to conditionally initialize 
// parts of a variable and producing the message
// Error:  "expected primary-expression before '.' token".
// So temporarily define it this way and hope that the definition of "FILE" 
// doesn't change soon.  
// Using this definition saves some code space by not including 'alloc' code.
#undef FDEV_SETUP_STREAM 
#define FDEV_SETUP_STREAM(putter, getter, f) { 0, 0, f, 0, 0, putter, getter, 0 } 
// When COM1 is defined, then the library printf function will output to the 
// USB or UART port.
// You must then include  "stdout = &COM1" in setup or loop to use printf;
FILE COM1 = FDEV_SETUP_STREAM(COM1putter, NULL, _FDEV_SETUP_WRITE);
// macro to get format string from rom.
#define printfROM(fmt, ...)   printf_P(PSTR(fmt),##__VA_ARGS__)
#endif  // COMIF


//******************************************************************************
//*                            Support functions                               *
//******************************************************************************

#if FREEIF && FREQGEN
// The following is the table of frequencies output for each press of the 
// generator frequency up/down buttons. These may be changed as desired. 
// Note: Index 7 and 16 are used when changing frequency counter modes
static const long PROGMEM Freqs[] =  
// Idx=   0  1  2  3   4   5   6    7    8    9    10    11
         {0,10,20,50,100,200,500,1000,2000,5000,10000,20000,
// Idx=       12    13     14     15      16      17      18 
          50000,100000,200000,500000,1000000,2000000,4000000};
#define NUMFREQS        (sizeof(Freqs)/sizeof(long))    

void SetFreqGenfromIndex(byte Idx)
  // Set generator frequency to one of values in the Freqs array.
{
  if (Idx>=NUMFREQS) return;
  FG.set(pgm_read_dword_near(&Freqs[Idx]));
}
#endif  // FREQGEN

#if HASLCD
#if FREQGEN
void ShowGenFreq(void)
{
  char St[17]; byte i; 
  Lcd.setCursor(0,1);  Lcd.print("Gen=");   
  sprintf_P(St,PSTR("%ld  Hz "),FG.set(-1)); 
  for (i=12-strlen(St); i>0; i--) Lcd.print(" ");
  Lcd.print(St); 
}
#endif  // FREQGEN

#if FREQCTR
void ShowCtrMode(sbyte Mode)
{
  char St[17];
  Lcd.setCursor(0,1);     
  if (!Mode)  { Lcd.print("                "); return; }
  if (((byte)Mode)<7) Lcd.print(" Gate: "); else Lcd.print(" #Avgs: ");
  switch (Mode)
  {
    case 1:  strcpy_P(St,PSTR("1 Sec"));  break;
    case 2:  strcpy_P(St,PSTR("10 mS"));  break; 
    case 3:  strcpy_P(St,PSTR("100mS"));  break;
    case 4:  strcpy_P(St,PSTR("10 S "));  break;
    case 5:  strcpy_P(St,PSTR("100 S"));  break; 
    case 6:  strcpy_P(St,PSTR("EXT  "));  break;
    case 7:  strcpy_P(St,PSTR("1   "));   break;
    case 8:  strcpy_P(St,PSTR("10  "));  break;
    case 9:  strcpy_P(St,PSTR("100 "));  break;
  }
  Lcd.print(St);  Lcd.print(" ("); Lcd.print((byte)Mode); Lcd.print(")  ");
}
#endif  // FREQCTR
#endif  // HASLCD


//******************************************************************************
//*                 setup and loop -- Main Arduino functions                   *
//******************************************************************************

#define INBUFSIZ  80
char InBuf[INBUFSIZ+1];  byte InBufPtr = 0; 
#if FREEIF
sbyte FCMode = 1;  byte FcBtnUH = 0, FcBtnDH = 0;    
byte  GMode = 16;  byte GBtnUH = 0,  GBtnDH = 0;    
#endif  //FREEIF

void setup() 
  // The setup routine runs once when you press reset:
{                
#if COMIF
  COMM.begin(COMMBAUD); stdout = &COM1; // Set printf output to USB serial port.
#endif
#if LED
  pinMode(LED,OUTPUT);  
#endif
#if FREEIF
#if FREQGEN
  pinMode(GBUTTONUP,INPUT_PULLUP);    pinMode(GBUTTONDN,INPUT_PULLUP);  
  GBtnUH=digitalRead(GBUTTONUP);        GBtnDH=digitalRead(GBUTTONDN);
#endif
#if FREQCTR
  pinMode(FCBUTTONUP,INPUT_PULLUP);   pinMode(FCBUTTONDN,INPUT_PULLUP);  
  FcBtnUH=digitalRead(FCBUTTONUP);      FcBtnDH=digitalRead(FCBUTTONDN);
#endif
#endif  // FREEIF
#if HASLCD
  Lcd.begin(16,2);  Lcd.noCursor(); Lcd.noAutoscroll();
#if !FREQCTR
  Lcd.print(" Freq Generator ");  
#else
  Lcd.print("FrequencyCounter");  
#endif  
  Lcd.setCursor(0,1); Lcd.print("  Test Program"); 
  delay(2000);  Lcd.clear();
#endif 
#if FREQGEN 
  FG.set(1000000);
#endif
#if FREQCTR 
  FC.mode(1);
#if HASLCD
  ShowCtrMode(FC.mode()); 
#endif
#endif
}


void loop() 
  // The loop routine runs over and over again forever:
{
  static unsigned long MS=millis();   static bool FCState=0;
  char FCBuffer[20] = {0};   byte i; 

  // Read the frequency counter if FREQCTR and its ready and either FCState or LCD
#if FREQCTR
  if (FC.available() && (FCState || HASLCD)) { FC.read(FCBuffer,0); }
#if HASLCD
// If we have a new frequency count value then display it. 
  if (FCBuffer[0])
  {
    Lcd.setCursor(0,0); 
    for (i=11-strlen(FCBuffer); i>0; i--) Lcd.print(" ");
    Lcd.print(FCBuffer); Lcd.print("  Hz "); 
  }
#endif 
#endif  // FREQCTR

#if FREEIF
#define KBDDEBOUNCE       5     // 5=50mS
  static byte FcChg=1, GChg=1;  static byte KbdDB = KBDDEBOUNCE; 

  // Read buttons (on each debounce time) and inc/dec FCMode/GMode if in range. 
  if (!--KbdDB)  
  { 
    KbdDB=KBDDEBOUNCE; 
#if FREQGEN
    i=digitalRead(GBUTTONUP);
    if (!i && GBtnUH && GMode<(NUMFREQS-1)) { GMode++; GChg++; }   GBtnUH=i; 
    i=digitalRead(GBUTTONDN); 
    if (!i && GBtnDH && GMode>0 ) { GMode--; GChg++; }   GBtnDH=i; 
#endif

#if FREQCTR
    i=digitalRead(FCBUTTONUP);
    if (!i && FcBtnUH && FCMode<9)
    {
      // Go to next gate time (input to SetGateTime has indexes out of order)
      if (FCMode==1) FCMode=4; else if (FCMode==3) FCMode=1; else FCMode++; 
      FcChg++; 
    } FcBtnUH=i; 
    i=digitalRead(FCBUTTONDN);
    if (!i && FcBtnDH && FCMode!=2)  
    {
      // Go to previous gate time (input to SetGateTime has indexes out of order)
      if (FCMode==4) FCMode=1; else if (FCMode==1) FCMode=3; else FCMode--; 
      FcChg++; 
    } FcBtnDH=i; 
#endif  // FREQCTR
  }  
#if FREQGEN
  // If the freq generator mode changed
  if (GChg)  { SetFreqGenfromIndex(GMode); ShowGenFreq();  GChg=0; }
#endif
#if FREQCTR
  // If the freq counter mode changed
  if (FcChg) 
  { 
    // try to set gate mode... if it's good then show new counter mode
    FcChg=0;  i=FC.mode(FCMode);  
    // else if we incremented to invalid value then decrement back to valid
    if(((sbyte)i)>=0) ShowCtrMode(FCMode); else if (!FcBtnUH) FCMode--;
#if FREQGEN 
    // If we changed from period mode to traditional mode (or reverse), change
    // the generator frequency to be appropriate for the counter mode.
    // (Traditional: 1MHz, period: 1000Hz)
    if (FCMode==7) SetFreqGenfromIndex(GMode=7);  if (FCMode==6) SetFreqGenfromIndex(GMode=16);
#endif
  }
#endif   // FREQCTR
#endif   // FREEIF


#if COMIF
  if (COMM) 
  {
    static char lastch;  char ch;  long Val; 
    while (COMM.available() > 0)  
    {
      ch=COMM.read(); 
      // get rid of linefeed characters (if we just got a CR character)
      if (ch=='\n' && lastch=='\r') continue;
      // echo character  
      if (ch=='\r' || ch=='\n') printfROM("\n"); else printfROM("%c",ch);
      lastch=ch;   // Save this char so that if we get CR/LF we can dump the LF
      // if the character is not CR or LF and it will fit in the buffer
      if (!(ch=='\r' || ch=='\n' ) && InBufPtr<INBUFSIZ) 
      { 
        // If its a backspace character, remove last char from buffer if possible.
        if (ch=='\b') { if (InBufPtr) InBufPtr--;  continue; }  
        InBuf[InBufPtr++] = ch;      // else put the character in the buffer
      }   
      else
      {
        char *last;
        InBuf[InBufPtr]=0;      // terminate the input string
        // Remove any control characters and spaces from string
        InBufPtr=0; while (InBuf[InBufPtr]) 
        { 
          if (InBuf[InBufPtr]<'!') { strcpy(InBuf+InBufPtr,InBuf+InBufPtr+1); } else InBufPtr++; 
        }
        // if second char is '=' remove it
        if (InBufPtr>1 && InBuf[1]=='=') { strcpy(InBuf+1,InBuf+2); InBufPtr--; }
        //
        // Now interpret the command
        switch (toupper(InBuf[0]))
        {
#if FREQGEN
          // Get or set the frequency generator
          case 'G': 
            if (InBufPtr>1)             // if set freq command
            {
              // Convert characters to 'Val' and see if we consumed all characters 
              // and that the value interpreted is >=0
              Val=strtol(InBuf+1,&last,10);  i=((last-InBuf)<(InBufPtr) || Val<0);          
              if (!i) Val=FG.set(Val);   // if no error then set the frequency
              if (i || Val<0) printfROM("Error setting frequency\n"); 
              else 
              {
#if HASLCD
                ShowGenFreq();
#endif
                goto ReturnGen; 
              }
            }
            else                        // Get set freq command
            {
ReturnGen:    printfROM("Frequency generator set to %ld Hz\n", FG.read());
            }
            break;
#endif
#if FREQCTR
          // Get or set the frequency generator gate time
          case 'T': 
              static sbyte Gate; 
              if (InBufPtr>1)
              {
                // Convert characters to 'Val' and see if we consumed all characters 
                // and that the value interpreted is a signed byte (i=0 if ok)
                Val=strtol(InBuf+1,&last,10);  i=((last-InBuf)<(InBufPtr) || Val<-127 || Val>127);  
                if (!i) 
                { 
                  Gate=FC.mode((sbyte)Val); 
                  if (Gate<0) goto Invalid;
#if FREEIF
                  FCMode=Gate;
#endif
#if HASLCD
                  ShowCtrMode(Gate);
#endif
                  goto ReturnGate;  
                }
                else goto Invalid;
              }
              else
              {
                Gate=FC.mode(-1);
ReturnGate:     printfROM("Frequency counter gate set to %d\n", Gate); 
              }
            break;
          case 'F': 
            if ( InBufPtr==1 || (InBufPtr==2 && toupper(InBuf[1])=='1'))
              printfROM("Frequency is %s Hz\n", FC.read(InBuf,InBufPtr>1)); 
            else if (InBufPtr==2 && toupper(InBuf[1])=='S')
              printfROM("CountReady = %d\n",FC.available());
            else goto Invalid;
            break; 
          case 'R': 
            FCState=!FCState; 
            printfROM("Frequency counter auto read is "); 
            printf_P((FCState)?PSTR("ON\n"):PSTR("OFF\n"));
            break;
#endif          
          case '?':
            printfROM("Frequency Generator and Frequency Counter Test Module.\n");
            printfROM("Vers: " VERSION "        (c) Rick Groome 2021\n\n");
            printfROM("Commands are:\n");
#if FREQGEN
            printfROM("G<freq>   Set the generator frequency.\n");
            printfROM("          <freq> is 0..20000000 (0=off)\n");
            printfROM("G         Get currently set generator frequency.\n");
#endif
#if FREQCTR
            printfROM("T[0..9]   Set frequency counter gate time.\n");
            printfROM("          0=off, 1=1sec, 2=10mS, 3=100mS, 4=10S, 5=100S, 6=ext\n");
            printfROM("          7=Period mode, 8=Period(10 avg), 9=Period(100 avg)\n");
            printfROM("T         Get currently set frequency counter gate time.\n");
            printfROM("F         Get last read frequency counter value.\n");
            printfROM("F1        Wait for and get next freq counter value.\n");
            printfROM("FS        Returns 0 if not ready, or 1 if new value available.\n");
            printfROM("R         Turns on/off auto read of frequency counter. (toggle)\n");
#endif
            printfROM("?         Show this help screen.\n");
            break;
          default: 
Invalid:    if (InBufPtr) printfROM("Invalid command '%s'\n", InBuf);
        }
        // command done. prep buffer for next time. 
        InBufPtr=0; InBuf[InBufPtr]=0;       
      }
    }  // while (COMM.available() > 0)   
  }    // if (COMM)
#if FREQCTR
  // if "R" command and freq ctr is running, show the value.
  if (FCState && FCBuffer[0]) { printfROM("%s\n",FCBuffer); }
#endif
#endif   // COMIF

#ifdef LED
  // All of the below is for debug only..
#define LEDBLINKRATE    500           // 500mS
  static int dbgctr=LEDBLINKRATE/10;
  if (!--dbgctr) 
    { dbgctr=LEDBLINKRATE/10; digitalWrite(LED,!digitalRead(LED)); }  
#endif  // LED

  // Clear the saved frequency count
  FCBuffer[0]=0;
  
  // Wait 10mS     NOTE: "Serial()" by itself takes almost 10mS
  while (millis() < MS+10) continue;  MS=millis();
}

