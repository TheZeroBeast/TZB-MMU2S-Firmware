# MM-control-01
MMU 3-axis stepper control

# Below are the stable versions of MMU2 & MK3 FW required.
MMU: V#: 2.1.2	B#:  200 waiting to merge patch1

MK3: V#: 5.0.1	B#: 2131 waiting to merge patch1

# When in error state Active Extruder LED will blink, FINDA Sensing is RED and GREEN for not, realtime
  - if unloading when failed
    - pull back filament
    - push selector aside
    - push out and cut damaged/ground end off
    - push filament back, clear of selector
    - push middle button to rehome selector and continue (if idler suspected not synced press right to rehome and continue)
  
  - if loading when failed NOTE: MK3 will be waiting for filament
    - troubleshoot issue, usually was due to being caught on an edge
    - push middle button to clear problem and continue
    - detach extruder PTFE and load filament into MK3, should grab and continue
    - reattach PTFE

## Slic3r Advance Settging for load/unload Speed
![MMU2-Slic3r-LoadUnload-Speeds](/MMU2-Slic3r-LoadUnload-Speeds.png)

## Building this custom setup
This is configured to work directly with MK2.5/MK3 with matching printer FW at link below to load filament to ExtruderLaserFilamentSensor and unload to FINDA Sensor.

https://github.com/TheZeroBeast/Prusa-Firmware/tree/MMU-MK3-FSensorBuild

Ensure filament sensor is enalbed via MK3 menu the first time you flash your MK3 to ensure your MMU2 gets messages when filament gets to the MK3.

### Building
### Arduino
Recomended version is arduino 1.8.5.
#### Adding MMUv2 board
In Arduino IDE open File / Settings  
Set Additional boards manager URL to:  
https://raw.githubusercontent.com/prusa3d/Arduino_Boards/master/IDE_Board_Manager/package_prusa3d_index.json  
Open Tools / Board: / Boards manager...
Install Prusa Research AVR Boards by Prusa Research  
which contains only one board:  
Original Prusa i3 MK3 Multi Material 2.0

Select board Original Prusa i3 MK3 Multi Material 2.0

Bootloader binary is shipped with the board, source is located at https://github.com/prusa3d/caterina
#### Build
click verify to build
### PlatformIO
PlatformIO build is not supported by Prusa Research, please report any PlatformIO related issues at https://github.com/awigen/MM-control-01/issues

## Flashing
### Windows
#### Arduino
click Upload
#### Slic3er
Hex file needs to be edited to be recognized as for MMUv2, to be specified later (in several years)
#### Avrdude
Board needs to be reset to bootloader. Bootloader has 5 seconds timeout and then returns to the application.

This can be accomplished manually by clicking reset button on MMU, or programmatically by opening and closing its virtual serial line at baudrate 1500.

Than flash it using following command, replace \<virtual serial port\> with CDC device created by MMU usually com\<nr.\> under Windows and /dev/ttyACM\<nr.\> under Linux. -b baud rate is don't care value, probably doesn't have to be specified at all, as there is no physical uart.
~~~
avrdude -v -p atmega32u4 -c avr109 -P <virtual serial port> -b 57600 -D -U flash:w:MM-control-01.ino.hex:i
~~~

### Linux
Same as Windows, but there is known issue with ModemManager:

If you have the modemmanager installed, you either need to deinstall it, or blacklist the Prusa Research USB devices:

~~~
/etc/udev/rules.d/99-mm.rules

# Original Prusa i3 MK3 Multi Material 2.0 upgrade
ATTRS{idVendor}=="2c99", ATTRS{idProduct}=="0003", ENV{ID_MM_DEVICE_IGNORE}="1"
ATTRS{idVendor}=="2c99", ATTRS{idProduct}=="0004", ENV{ID_MM_DEVICE_IGNORE}="1"

$ sudo udevadm control --reload-rules
~~~
A request has been sent to Ubuntu, Debian and ModemManager to blacklist the whole Prusa Research VID space.

https://bugs.launchpad.net/ubuntu/+source/modemmanager/+bug/1781975

https://bugs.debian.org/cgi-bin/pkgreport.cgi?dist=unstable;package=modemmanager

and reported to
https://lists.freedesktop.org/archives/modemmanager-devel/2018-July/006471.html

## Building documentation
Run doxygen in MM-control-01 folder.
Documentation is generated in Doc subfolder.
