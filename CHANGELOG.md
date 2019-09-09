Changelog of MMU 2.0 Firmware
=============================
# V3.0.0 RMM  
* Incorporated changes from 3.8.0 (7x7, ASA, Sheet Setups, menu updates & autodeplete)
* Implemented timer0 @ 30Hz for PWM of Heatbed.
* Overhaul of the TZB-Comms protocol, now with five data bytes, no CSUM used anymore.
* Implemented STEALTH mode for those times you don't want sirens going off in your home, hahaha.
* Changed FINDA Status updates to be received in the background via UART2 ISR. Still requested in the same way.
* Removed ACK & NACK as I wasn't currently using it and other resend/retry methods are in place for the rare missed payload.
* Implemented a payload RX timeout on MK3S UART2 ISR at 5microseconds past 7 byte reception. This is to reset the protocol if any garbage appears on the input line.

# V2.1.8 RMM  
* Fixed long bowden cal from 2.1.8-RC2
* Idler holding current now much lower when not engaged, allowing proper continual homing  
  of idler when required as well as less chance of loosing idler steps during engagement.
* Unload type based sync speeds have now been implemented and configurable in config.h
* Removed setup menu from boot to remove confusion. Now only accessible via service position.
* Implemented direct positioning for idler and selector resulting more efficient moves.
* Changed Jam detection to M219 E, off by default enalbed by adding M219 E at filament start gcode on filaments you want jam detect on
* load, unload messages changed to status updates on MK3
* Button debounce implemented for ensure correct button read
* Implemented toolChange from mmu count instead of the attempt from mk3.

# V2.1.7 RMM  
* Skipped due to checksum issues with sending over serial

# V2.1.6 RMM  
* Added feedback at start of Active Extruder to MK3 so display is acurate
* Also added same feedback during load menu commands and feed filament
* Added feedback from set_positions to update MK3 active filament info display
* Added a bunch of MK3 messages to advise user of MMU state in setup menu and when it isn't doing a function due to filament detected
* Added 7x7 MBL
* Added Jam detection
* Added Setup guided BowLen and FSensor to BondTech distance setup
* More action commands added for failure states
  1. // action:m600
  2. // action:mmuAttention
  3. // action:mmuFailedLoad1
  4. // action:mmuFailedLoad2
  5. // action:mmuFailedUnload
  6. // action:jamDetected

# V2.1.5 RMM  
* Hotend and extruder motor off when in m600 waiting
* Added RIGHT Button retract option on failed load
* Added RIGHT Button retract option after eject when checking tip formation
* Added MMU2-OFF by putting selector in service position then restarting MK3
  * To re-enable move selector to any filament position then restart MK3

# V2.1.4 RMM  
* Added Bowden Length Cal off service menu or holding middle @ boot (you need to cal first filament to end of tube. This is used as a ballpark for those with different length tubes.
* Implemented entire serial communication re-write. using ISR either side with modbus inspired payload ack-nack.
* Fully implemented shr16 methods to utilise stepper enable lines and leds are now better handled.
* Steppers now run a 5m timer when not printing (no hiss, no heat)
* Implemented MK3 FSensor testing when not printing, disconnect bowden tube and test filaments for reactivity.
* POWER PANIC is now functional when Printing from SD card.
* Increased toolSync to 100 changes, as this seams to be plenty sufficient.
* MK3 Serial Log now idicates tool change count per print. i.e. first T? cmd to U0 cmd.
* SELECTOR MAX SPEED Reduced to help those with a bit more resistance
* Idler homing improved further for wider variances in setup. Please ensure no contact with case as this will still present an issue.
* Two action commands setup to be used with octoprint. "action: m600" & "action: mmuAttention" These can be setup with plugin Action Commands.
* Bowden Length limit increased t. ~2.8m

# V2.1.3 RMM  
* toolChanges every 25
* Tuned Bowden length for less destructive failed load situation
* Re-written home on fail to optimise fail recovery
* Re-written unload sequence for vastly more reliable unloads
* Load to Extruder start now synced with MMU2 instead of an arbitrary 1.5s wait
* Tuned load/unload speeds for PLA, tension bolts must be 2-3 full turns past flush to stop grinding

# V2.1.2 RMM  
* corrected bug in 20 toolChange rehome resulting in extruder 0 being used for that change instead of intended filament. Eventually results in incorrect execution of T? command as MK3 thinks a different extruder is being used.
* corrected bug resulting in adjacent filament extruder C0 command amount after you have fixed an issue.
* MMU now does positive acknowledgement on all commands so it can't miss any as it has been doing
* improvements for fixTheProblem() that now works better hand in hand with load, unload and FeedFilament issues
* corrected a moveSmooth() error resulting in a successful move with FINDA enabled even if FINDA never triggered. Now resulting in an error to be fixed by user.

# V2.1.1 RMM  
* moveSmooth ACC and FINDA triggering within, defaults ACC to NORMAL and FINDA to false
* implemented new Karl motion into entire MMU firmware
* adjusted homing methods and settings, hopefully it works on majority of setups
* implemented rehome after 20 toolChanges, idea credit to Chuck Kozlowski
* implemented MK3 comms to confirm when filament arrives at MK3, removed all tube length cal methods
* implemented fixTheProblem() that pauses for user to fix issue, Middle button to continue, this version fixed loop that was possible if two triggered, nested

# Project Fork  
* forked project from V1.0.1 (51aea8692)
* add QtCreator project file (usable for PlatformIO)
  used `pio init --ide qtcreator` for setting it up. 
* reformatted files with clang-format
* defined some constants for pin toggling
* rename function `park_idler()` to `engage_filament_pulley()`
  fixes weird negation
* add many lines of documentation
* cleanup of `static`/`extern` (alias private and public)
* bugfix of `isIdlerParked` state, issue #60
  and simplified it's calls
* add `move_idler()`, `move_pulley()` and `move_selector()`
  instead of `move()` with 3 arguments, where alway 2 of them were 0
* remarked some TODOs
* changed version to a dummy version 9.9.9
* moved many stepper constants into config.h
* **implement motion control** with acceleration
  mostly fixes issue #52
* implement fast homing of selector
* replaced all calls of `digitalRead(A1)` by `isFilamentInFinda()`
* moved menu request button on boot before homing
  fixes issue #55
* removed delays on button presses to move the selector (while not printing)
  fixes #56
* removed unnecessary engaging pulley before moving selector
* renamed function from `load_filament_in_printer()` to 
  `load_filament_intoExtruder()`
* simplified `load_filament_intoExtruder()` by using move-commands
  onyl useful with TESTING flag, otherwise the filament has wrong speed
* add explanations on stall gurad and stall guard threshold
* retract filament on `recover_after_eject()`
  useful to inspect tips and continue afterwards
* reduced delay for buttons on calibrating bowden length from 400ms to 100ms
* add shell script for formatting (**`format.sh`**) using 
  Artistic Style and `.astylerc`
* reformatted whole code with new format settings with `format.sh`
* **merged V1.0.2** into rework branch
* add define TESTING_STEALTH
* defined max. speeds and acceleration for normal and stealth mode

# V1.0.2
* fixed current settings
* stealth mode support
* anything else?


# V1.0.1
* add separate calibration for each filament channel
* anything else?


# V0.9.0
* initial release for officially shipped MMU 2.0 kits
