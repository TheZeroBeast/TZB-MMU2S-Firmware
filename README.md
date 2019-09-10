# TZB-MMU2S-Firmware
MMU2S 3-axis TMC2130 Stepper controller

## Latest stable versions & link to **RELEASES** for Precompiled HEX files. 
**MMU2S: V#: 3.0.0		B#:  331**  
**MK3S:  V#: 3.0.0    B#: 2431**  

**https://github.com/TheZeroBeast/TZB-MMU2S-Firmware/releases**

## Points of Notes  
You **DO NOT** have to reset to factory settings, all the same EEPROM data structures are used as STOCK-PRUSA-FW  
Bowden Length is still important as load is optimised to minimise the posibility of grinding in the event of the  
MK3-FSensor not triggering, rare if setup correct, dust/particles still build up over time.  
**DEFAULT FACTORY BOWDEN LENGETH OF 350mm**   
**Action commands** added for failure states that can be used for notifications in Octoprint
  1.  // action:m600
  2.  // action:mmuAttention
  3.  // action:mmuFailedLoadFINDA
  4.  // action:mmuFailedLoadIR_SENSOR
  5.  // action:mmuFailedUnload
  6.  // action:JamDetected
  
### Example telegram system command to use with Octoprint Plugin **Action Command**  
`curl https://api.telegram.org/`**YOUR_BOT_ADDRESS**`/sendMessage -d chat_id="`**YOUR_CHAT_ID**`" -d text="MMU2S ‘I failed to load past the IR_SENSOR (possible Jam). Please fix me MeatBag’"`
  
![MMU2S-Octoprint-Action-Command-Setup](/MMU2S-Octoprint-Action-Command-Setup-3.0.0-RC2.png)
  
### Compatibility
   **Only the MK3S is compatible** with this firmware (If anyone has a MK2.5S:MMU2S setup and wants to test it's just the Printer side that'll need tweaking)

# **STEPS TO FIRST GET SETUP AND PRINTING WITH THIS FW**  
1. Adjust **BOWDEN_LENGTH** in **SETUP** menu **(MK3 LCD Messages to guide you will appear)**  
**(NOTE ONLY FIRST FILAMENT CHANNEL IS CALIBRATED)**
   1. Move selector all the way to the **RIGHT** with **RIGHT** button to Position 6 which is **SERVICE POSITION**
   2. Enter **SETUP** menu using **MIDDLE** button while on **SERVICE POSITION**
   3. As per STOCK FW, use **LEFT** button to move LED to fourth position
   4. Use **MIDDLE** button to enter calibration
   5. Use **MIDDLE** button again to load for **Bowden Length** Calibration
   6. Use **LEFT/RIGHT** buttons to adjust until flush with end of tube when detatched from extruder
   7. Use **MIDDLE** button to unload
   8. Use **LEFT** button to save and exit

2. Blade isn't being used and can be removed. It has been known to add resistance to selector.

3. Get familiar with the differences in failure states and recovery procedures from STOCK

4. Happy **PRINTING!**

## When in error state Active Extruder & or Previous Extruder LED/s will blink

###   RED and GREEEN Idicate F.I.N.D.A. State @ Load Failure
* Troubleshoot issue, usually was due to being caught on an edge or ground section of filament
  * Note: Failed **LOAD** where filament is found partly in Extruder requires a burst of air to clean MK3-FSensor
  * **RIGHT** Button can now be used to retract slowely back to park (LED goes green to advise you can try load again)
* If printer has shut down heaters, click MK3 wheel before clearing MMU2 issue with middle button
* **ALWAYS** ensure filament pulled clear of selector (**Auto as above or manually**)
* Push middle button to rehome and continue
  * If nothing happens, filament is in FINDA, check again
  * If load fails again, give MK3-FSensor another burst of air and try again
  * **IF FSENSOR FAILED/DEAD LCD WILL DISPLAY FSensor Disabled (Stop Print and Diagnose FSensor Fault)**

###   RED and GREEEN Idicate F.I.N.D.A. State pluss GREEN for next extruder @ Unload Failure
* Troubleshoot issue, usually was due to ground section of filament
  * Note: If fail on **UNLOAD** the LED will show on the next extruder instead of current
* If printer has shut down heaters, click MK3 wheel before clearing MMU2 issue with middle button
* **ALWAYS** ensure filament pulled clear of selector.
* Push middle button to rehome and continue
  * If nothing happens, filament is in FINDA, check again

# Building this custom setup
This is configured to work directly with **MMU2S** with matching **MK3S** FW at link below to load filament to **Extruder Laser Filament Sensor** and unload to FINDA Sensor.

https://github.com/TheZeroBeast/TZB-MK3S-Firmware
### PlatformIO
Download, open in PlatformIO and build.
The HEX which is placed within the .pio root folder still requires the addition of the **; device = mm-control** line as bellow.

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

Download source, rename the "SRC" folder to "Firmware" then open MM-control-01.ino with ArduinoIDE.

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
