Changelog of MMU 2.0 Firmware
=============================



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
