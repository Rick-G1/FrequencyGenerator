## [Frequency Generator](https://github.com/Rick-G1/FrequencyGenerator) library for Arduino and Pro Micro (ATMega32U4)

Available as Arduino library "**FrequencyGenerator**"

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

[![Hit Counter](https://hitcounter.pythonanywhere.com/count/tag.svg?url=https%3A%2F%2Fgithub.com%2FRick-G1%2FFrequencyGenerator)](https://github.com/brentvollebregt/hit-counter)

Frequency generator library for AVR **ATMega32U4** (and similar) processor using **Timer 4** and PLL.  

The library sets the PLL, prescaler, and counter registers to the appropriate calculated values from a single long integer value passed to it.  The library does not use interrupts and can be used with other Arduino libraries and functions. 

Produces square wave from 1Hz to about 12MHz.

## Interface

The library is implemented as a class named "`FrequencyGenerator`" with 2 member functions:

`long `**set**`(long Frequency)` Sets the PLL source, the prescaler and the counter registers to produce a square wave on Arduino Digital pin 5 [PC6]  (or alternatively Arduino Digital pin 10 [PB6]) from the single long integer passed to the function.  Function returns the actual frequency set, or -1 if value could not be set.

`long `**read**`(void)` Returns the currently set value of the frequency generator as a long integer.

## Internal Details

This module implements a variable frequency generator using Timer 4 on an Arduino Pro Micro Module (using an ATMega32U4).   
  
Output is on Arduino Digital pin 5 [PC6] (using OC4A-) for Pro Micro.
(Could also output on Arduino Digital pin 10 [PB6] (using OC4B). 

This module is specifically for the timer 4 module of the ATMEGA32U4 on a Pro Micro device (or other microcontroller devices with similar functional blocks) assuming the controller is running as a USB device with the PLL set at 96MHz and a crystal of 16MHz (This is the default for the Pro Micro device).  It could possibly be reworked for other timers, but the resolution would be less.
The function 'FrequencyGenerator::set' accepts a long integer that is the frequency to output.  This may range from less than 0 (return current frequency) to 0 (generator off) to some over 1/2 the clock frequency of the micro (8MHz).  (It has been observed  to work to about 12MHz).  It works by selecting the best PLL multiplier, counter prescaler and count value by trying each of 3 possible PLL multipliers and looking for the error between the desired and actual output frequency obtainable with the calculated PLL multiplier, prescale value and count value.  Once the closest combination is determined, the hardware is set up to those values.  Using this algorithm, the output will be as close as possible to the desired frequency.   The duty cycle of the output will be 50%.  The function returns either the frequency being output or -1 if the new requested frequency could not be set.
 
Since the timer is set up to automatically reload, no interrupts or other   software overhead is required -- Just call the function and then the hardware will produce the output frequency with no additional intervention. 

As mentioned above, the output frequency will be the closest value to the desired frequency obtainable with the hardware (without further intervention).  It may not be exactly the same frequency as the frequency set.  To determine the actual output frequency, the call to 'FrequencyGenerator::read' will return the actual frequency the hardware is set to (This is also the value returned when calling 'FrequencyGenerator::set').  

While the basic user interface is via a class, only a single instance should be declared as this module uses specific hardware resources. 
  
The timebase used for the generator is the micros clock, which is normally a crystal oscillator with its inherent accuracy and stability, but since it is not calibrated it will normally vary from its nominal frequency of 16MHz by a few Hertz.  This is typically within about 0.1 to 0.2%, but could be more than that depending on the exact Pro Micro module used.  Since the input frequency can vary some, the output will vary by the same percentage.  It is possible to rework the input crystal frequency to include a trimmer capacitor and then tune it to exactly 16MHz.  An alternate is to create a compensation value and then apply this compensation value to any value input so that the actual output frequency is correct.

If it is desired to "trim" the Pro Micro's 16MHz oscillator to count or generate frequencies that are more accurate, a Knowles Voltronics JR400 trimmer capacitor (8-40pF) (or similar) can be used to replace the C2 capacitor on the module.  (You could also replace C2 with a 10pF cap and use a JR200 (4.5-20pF) for a more stable adjustment with less range).  This trimmer capacitor can be placed on top of the 32U4 chip and tiny wires (wire wrap?) used to connect the two connections of it to the Pro micro module.  Connect the rotor of the trimmer cap to GND (the '-' side of C19 is a good place) and the other side of the trimmer cap to the non-GND side of C2.  If the rotor side of the trimmer cap is connected to the C2 connection, the oscillator will not be as stable. Once wired, glue the trimmer cap to the top of the 32U4 chip.  Then set the frequency generator of the chip this is programmed on to 4MHz and using a reference frequency counter, tune the trimmer capacitor to exactly 4MHz. If frequency counter only is programmed, inject a 4MHz signal into the frequency counter input and tune to exactly 4MHz. 

The repository also includes a documentation file that provides more details on the use of this library and also covers a companion frequency counter library.

