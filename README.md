# HID_Satisfactory_Rotary_Encoder
Rotary encoder used for the Satisfactory game.

## Purpose
I made this sketch to be used to jump around the map in the Satisfactory game. It uses the Teleport '!tp ' from the 'Pak Utility Mod'. There is a table of strings in here with the names of the several locations I use around the map.

## Hardware
For this contraption I used an Arduino Nano ESP32, a HID capable device to simulate the keyboard, a 4 x 20 LCD screen and a rotary encoder with push button.

## Software
Most of the code is my own, the (very clever) way the interrupts are handled is derived from the work of Marko Pinteric
 *           ► Technical paper by Marko Pinteric explaining his code:
 *           https://www.pinteric.com/rotary.html 

## Workings
First an initial screen is displayed for 3 seconds with the program version. After that a part (four lines) of the table of destinations is shown on the screen in a round robin fashion. The rotary encoder can be used to scroll in that table. The current entry has a '>'before its name.

When the desired destination is 'current' and the button is pressed, the command is sent through the keyboard.


