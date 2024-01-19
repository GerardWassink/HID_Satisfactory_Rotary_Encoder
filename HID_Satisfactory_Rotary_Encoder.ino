/* ------------------------------------------------------------------------- *
 * Name   : HID_Satisfactory_Rotary_Encoder
 * Author : Gerard Wassink
 * Date   : January 2024
 * Purpose: Generate keyboard commands
 * Versions:
 *   0.1  : Initial code base
 *   0.2  : Cleaned up code
 *          alphabetized destinations
 *   0.3  : Incorporated new code for interrupt handling by Marko Pinteric
 *           ► Technical paper by Marko Pinteric explaining his code:
 *           https://www.pinteric.com/rotary.html 
 *   1.0  : Built in multiple lines scrolling
 *          Code cleanup
* ------------------------------------------------------------------------- */
#define progVersion "1.0"                     // Program version definition
/* ------------------------------------------------------------------------- *
 *             GNU LICENSE CONDITIONS
 * ------------------------------------------------------------------------- *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * ------------------------------------------------------------------------- *
 *       Copyright (C) Januari 2024 Gerard Wassink
 * ------------------------------------------------------------------------- */


#if ARDUINO_USB_MODE

#warning This sketch should be used when USB is in OTG mode
void setup(){}
void loop(){}

#else

/* ------------------------------------------------------------------------- *
 *       Compiler directives to switch debugging on / off
 *       Do not enable debug when not needed, Serial takes space and time!
 * ------------------------------------------------------------------------- */
#define DEBUG 0

#if DEBUG == 1
  #define debugstart(x) Serial.begin(x)
  #define debug(x) Serial.print(x)
  #define debugln(x) Serial.println(x)
#else
  #define debugstart(x)
  #define debug(x)
  #define debugln(x)
#endif


/* ------------------------------------------------------------------------- *
 *                                                         Include libraries
 * ------------------------------------------------------------------------- */
#include <Wire.h>                       // I2C comms library
#include <LiquidCrystal_I2C.h>          // LCD library

#include <String.h>                     // String library

#include <USB.h>                        // USB library
#include <USBHIDKeyboard.h>             // Keyboard library

/* ------------------------------------------------------------------------- *
 *                                                            Create objects
 * ------------------------------------------------------------------------- */
LiquidCrystal_I2C display(0x26,20,4);   // Instantiate display object
USBHIDKeyboard Keyboard;                // Instantiate keyboard object

/* ------------------------------------------------------------------------- *
 *                                                      Rotary encoder stuff
 * ------------------------------------------------------------------------- */
#define PIN_A D2
#define PIN_B D3
#define buttonPin D4

int rotationCounter = 0;                // Turn counter for the rotary encoder
                                        //   (negative = anti-clockwise)

volatile bool rotaryEncoder = false;    // Interrupt busy flag
volatile int8_t rotationValue = 0;      // Keep score
int8_t previousRotationValue = 1;       // previous value for comparison
int8_t TRANS[] = {0, -1, 1, 14, 1, 0, 14, -1, -1, 14, 0, 1, 14, 1, -1, 0};

/* ------------------------------------------------------------------------- *
 *            Variable definitions for destinations in Satisfactory game
 *                These are named locations between which one can 'teleport'
 *                using a !tp command in the game's chat window
 * ------------------------------------------------------------------------- */
const int numDestinations = 13;         // number of entries in the array
int maxDestinationIndex = numDestinations - 1; // max index (since we're counting from zero)
int currentDestination  = 0;
int previousDestination = 0;

String destinations[numDestinations] = {
              {"DunesAluCasings"} 
            , {"GoldCoastOil"}
            , {"GrassyCentral"}
            , {"Grassy"}
            , {"Hub"}
            , {"Nuclear"}
            , {"Nuclear2"}
            , {"RockyDesertCentral"}
            , {"Rubber"}
            , {"Silica"}
            , {"WaterfallFactory"}
            , {"WaterFallStation"}
            , {"WestDuneForrestCoal"}
            };

String myString;                        // for building up a 20 character string

/* ------------------------------------------------------------------------- *
 *                                        Variables for rotary switch button 
 * ------------------------------------------------------------------------- */
int buttonStateCurrent = HIGH;
int buttonStatePrevious = HIGH;
bool buttonPressed = false;


/* ------------------------------------------------------------------------- *
 *       Interrupt routine, determines:                 checkRotaryEncoder()
 *          validity of movement
 *          number to add/subtract
 * ------------------------------------------------------------------------- */
void checkRotaryEncoder()
{
  if (rotaryEncoder == false)           // Allowed to enter?
  {
    rotaryEncoder = true;               // DO NOT DISTURB, interrupt in progress...

    static uint8_t lrmem = 3;
    static int lrsum = 0;

    int8_t l = digitalRead(PIN_A);      // Read BOTH pin states to deterimine validity 
    int8_t r = digitalRead(PIN_B);      //   of rotation

    lrmem = ((lrmem & 0x03) << 2) + 2 * l + r;  // Move previous value 2 bits 
                                                // to the left and add in our new values
    
    lrsum += TRANS[lrmem];              // Convert the bit pattern to a movement indicator
                                        //    (14 = impossible, ie switch bounce)

// 
// These ifs contained return statements, 
//   hence all the else if's...
// 
    if (lrsum % 4 != 0)                 // encoder not in the neutral (detent) state
    {
        rotationValue = 0;
    }
    else if (lrsum == 4)                // encoder in the neutral state - clockwise rotation
    {
        lrsum = 0;
        rotationValue = 1;
    } else if (lrsum == -4)             // encoder in the neutral state - anti-clockwise rotation
    {
        lrsum = 0;
        rotationValue = -1;
    } else
    {                                   // An impossible rotation has been detected - ignore the movement
      lrsum = 0;
      rotationValue = 0;
    }

    rotaryEncoder = false;              // Ready for next interrupt
  }
}


/* ------------------------------------------------------------------------- *
 *                                                    ISR routine - button()
 * ------------------------------------------------------------------------- */
void button()
{
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  if (interruptTime - lastInterruptTime > 5)
  {
    buttonStateCurrent = digitalRead(buttonPin);
    if ((buttonStateCurrent != buttonStatePrevious) 
        && (buttonStateCurrent == LOW)) 
    {
      buttonPressed = true;    
    }
    buttonStatePrevious = buttonStateCurrent;
  }
  lastInterruptTime = interruptTime;
}


/* ------------------------------------------------------------------------- *
 *         Routine to display stuff on the display of choice - LCD_display()
 * ------------------------------------------------------------------------- */
void LCD_display(LiquidCrystal_I2C screen, int row, int col, String text) {
  screen.setCursor(col, row);
  screen.print(text);
}


/* ------------------------------------------------------------------------- *
 *             Show destination(s) at every scroll action - showDestinaton()
 * ------------------------------------------------------------------------- */
void showDestinations()
{
  int index = 0;                        // index in destination array
  int line = 0;                         // line on LCD display
  //
  // Show destinations, current one on second line
  //
  for (line= 0; line < 4; line++)   // Go thru display lines 0 - 3
  {
    switch (line) 
    {
      case 0:                           // Line 0
        if (currentDestination == 0)
          index = maxDestinationIndex;  // Roll over to last entry when line 1
        else                            //   is current
          index = currentDestination - 1;   // otherwise just show current - 1
        break;

      case 1:                           // Line 1
        index = currentDestination;     // Always show current in line 1
        break;

      case 2:                           // Line 2
        if (currentDestination == maxDestinationIndex)
          index = 0;                      // When current at max, show first here
        else
          index = currentDestination + 1; // otherwise just show current + 1
        break;

      case 3:                           // Line 3
        if (currentDestination == maxDestinationIndex)
          index = 1;                    // When current at max, show second here
        else if (currentDestination == maxDestinationIndex-1)
          index = 0;                    // When current at max-1, show first here
          else
            index = currentDestination + 2; // otherwise just show current + 2
        break;

      default:
        break;
    }

    if (line == 1)
    {
      myString = ">";    // place a '>' befor current entry
    }
    else 
    {
      myString = " ";                   // else just a space
    }
    myString.concat(destinations[index]);
    myString.concat("                    ");
    LCD_display(display, line, 0, myString.substring(0,20) );
  }
}


/* ------------------------------------------------------------------------- *
 *                                 Setup routing, do initial stuff - setup()
 * ------------------------------------------------------------------------- */
void setup()
{
  debugstart(115200);

  pinMode(PIN_A, INPUT_PULLUP);         // mode for 
  pinMode(PIN_B, INPUT_PULLUP);         //  rotary pins

  pinMode(buttonPin, INPUT_PULLUP);     // mode for push-button pin

  display.init();                       // Initialize display
  display.backlight();                  // Backlights on by default
  
  Keyboard.begin();                     // Start keayboard
  USB.begin();                          // Start USB

  //
  // Assemble and show intro text and versionb
  //
  LCD_display(display, 0, 0, "Satisfactory        " );
  myString = "  Teleporter v";
  myString.concat(progVersion);
  myString.concat("          ");
  LCD_display(display, 1, 0, myString.substring(0,20) );

  //
  // Leave intro screen for 5 seconds
  //
  LCD_display(display, 2, 0, "Initializing " );
  for (int t=13; t<16; t++) {
    LCD_display(display, 2, t, "." );
    delay(1000);
  }

  //
  // We need to monitor both pins, rising and falling for all states
  //
  attachInterrupt(digitalPinToInterrupt(PIN_A), checkRotaryEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_B), checkRotaryEncoder, CHANGE);

  //
  // Monitor pushbutton pin
  //
  attachInterrupt(digitalPinToInterrupt(buttonPin), button, CHANGE);

  debugln("Setup completed");
}


/* ------------------------------------------------------------------------- *
 *                                        Do actual repetitive work - loop()
 * ------------------------------------------------------------------------- */
void loop()
{
  if (rotationValue != previousRotationValue)   // Has rotary moved?
  {
    if (rotationValue != 0)             // If valid movement, do something
    {
      currentDestination += rotationValue;
      currentDestination = currentDestination < 0? maxDestinationIndex: currentDestination;
      currentDestination = currentDestination > maxDestinationIndex? 0: currentDestination;
      debug(rotationValue < 1 ? "L" :  "R");
      debugln(currentDestination);
    }

    showDestinations();                 // Put destination on the matrix board

    previousRotationValue = rotationValue; // avoid entering this code every time
  }

  if (buttonPressed == true)            // If the switch is pressed
  {
    myString = "!tp ";                  //   assemble message to send to keyboard
    myString.concat(destinations[currentDestination]);
    Keyboard.println(myString);         // and send it
    buttonPressed = false;              // avoid entering this code every time
   }

}
#endif
