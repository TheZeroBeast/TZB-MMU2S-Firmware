# TZB-MMU2S-Firmware
MMU2S 3-axis TMC2130 Stepper controller

## Latest stable precompiled HEX files please refer to **RELEASES** page.  
**https://github.com/TheZeroBeast/TZB-MMU2S-Firmware/releases**

Please review [Wiki](https://github.com/TheZeroBeast/TZB-MMU2S-Firmware/wiki) content regarding all aspects of operating your fresh **TZB-MMU2S** install.

## Points of Note  
You **DO NOT** have to reset to factory settings, all the same EEPROM data structures are used as STOCK-PRUSA-FW  
Bowden Length is still important as load is optimised to minimise the posibility of grinding in the event of the  
MK3-FSensor not triggering, rare if setup correct, dust/particles still build up over time.  
**DEFAULT FACTORY BOWDEN LENGETH OF 350mm**   
  
### Compatibility
   **Only the MK3S is compatible** with this firmware (If anyone has a MK2.5S:MMU2S setup and wants to test it's just the Printer side that'll need tweaking)

# Building this custom setup
This is configured to work directly with **MMU2S** with matching **MK3S** FW at link below to load filament to **IR_SENSOR** and unload to **FINDA** Sensor.

https://github.com/TheZeroBeast/TZB-MK3S-Firmware
### PlatformIO
Download, open in PlatformIO
The HEX which is placed within the .pio root folder still requires the addition of the the bellow line.
~~~
; device = mm-control
~~~

### Arduino
Recomended version is arduino is the latest.
#### Adding MMUv2 board
In Arduino IDE open File / Settings  
Set Additional boards manager URL to:  
https://raw.githubusercontent.com/prusa3d/Arduino_Boards/master/IDE_Board_Manager/package_prusa3d_index.json  
Open Tools / Board: / Boards manager...
Install Prusa Research AVR Boards by Prusa Research
which contains only one board:  
Original Prusa i3 MK3 Multi Material 2.0

Select board Original Prusa i3 MK3 Multi Material 2.0

Download source and open src.ino with ArduinoIDE.

Hex file needs to be edited to be recognized as for MMUv2 in case of Arduino build. This is done automatically in cmake build.

Add following line to the begining of MM-control-01.hex:
~~~
; device = mm-control
~~~
#### Avrdude
Board needs to be reset to bootloader. Bootloader has 5 seconds timeout and then returns to the application.

This can be accomplished manually by clicking reset button on MMU, or programmatically by opening and closing its virtual serial line at baudrate 1500.

Than flash it using following command, replace \<virtual serial port\> with CDC device created by MMU usually com\<nr.\> under Windows and /dev/ttyACM\<nr.\> under Linux. -b baud rate is don't care value, probably doesn't have to be specified at all, as there is no physical uart.
~~~
avrdude -v -p atmega32u4 -c avr109 -P <virtual serial port> -b 57600 -D -U flash:w:MM-control-01.ino.hex:i
~~~
