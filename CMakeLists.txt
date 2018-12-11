cmake_minimum_required(VERSION 3.0)

SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_C_COMPILER avr-gcc)
SET(CMAKE_CXX_COMPILER avr-g++)
find_program(CAT cat)

SET(CSTANDARD "-std=gnu11")
SET(CPPSTANDARD "-std=gnu++11")

SET(WARN "-Wall -Wextra")
SET(CPPTUNING "-fno-exceptions -fno-threadsafe-statics")
SET(OPT "-Os -ffunction-sections -fdata-sections -flto -fno-fat-lto-objects")
SET(MCU "-mmcu=atmega32u4")

add_compile_definitions(
	F_CPU=16000000
	ARDUINO=10805
	ARDUINO_AVR_prusa_mm_control
	ARDUINO_ARCH_AVR
	USB_VID=0x2c99
	USB_PID=0x0004
	USB_MANUFACTURER="Unknown"
	USB_PRODUCT="Original Prusa i3 MK3 Multi Material 2.0 upgrade"
)

SET(CFLAGS "${OPT} ${WARN} ${CSTANDARD} ${MCU}")
SET(CXXFLAGS "${OPT} ${WARN} ${CPPSTANDARD} ${CPPTUNING} ${MCU}")
SET(ASMFLAGS "${OPT} ${WARN} ${MCU}")

SET(CMAKE_C_FLAGS ${CFLAGS})
SET(CMAKE_CXX_FLAGS ${CXXFLAGS})
SET(CMAKE_ASM_FLAGS ${ASMFLAGS})
SET(CMAKE_EXE_LINKER_FLAGS "-fuse-linker-plugin -Wl,--gc-sections")


project(MM-control-01 C CXX ASM)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GetGitRevisionDescription)

get_git_head_revision(GIT_HEAD_REF GIT_COMMIT_HASH)
git_commits_in_master(GIT_COMMITS_MASTER)
git_local_changes(GIT_LOCAL_CHANGES)

configure_file(
${CMAKE_SOURCE_DIR}/MM-control-01/version.h.in
version.h
)
# The 'real' git information file
set(GITREV_BARE_FILE dirty.h)
# The temporary git information file
set(GITREV_BARE_TMP dirty-tmp.h)
set(GITREV_FILE ${CMAKE_BINARY_DIR}/${GITREV_BARE_FILE})
set(GITREV_TMP ${CMAKE_BINARY_DIR}/${GITREV_BARE_TMP})

set(SOURCES 
	MM-control-01/abtn3.c
	MM-control-01/adc.c
	MM-control-01/motion.cpp
	MM-control-01/main.cpp
	MM-control-01/tmc2130.c
	MM-control-01/permanent_storage.cpp
	MM-control-01/Buttons.cpp
	MM-control-01/uart.cpp
	MM-control-01/shr16.c
	MM-control-01/mmctl.cpp
	MM-control-01/spi.c
	core/abi.cpp
	core/hooks.c
	core/Stream.cpp
 	core/wiring_digital.c
	core/IPAddress.cpp
	core/Tone.cpp
	core/wiring_pulse.c
	core/CDC.cpp
	core/main.cpp
	core/wiring_pulse.S
	core/HardwareSerial0.cpp
	core/new.cpp
	core/USBCore.cpp
	core/wiring_shift.c
	core/HardwareSerial1.cpp
	core/WMath.cpp
	core/HardwareSerial2.cpp
	core/PluggableUSB.cpp
	core/WString.cpp
	core/HardwareSerial3.cpp
	core/HardwareSerial.cpp
	core/WInterrupts.c
	core/Print.cpp
	core/wiring_analog.c
	core/wiring.c	
)
add_executable(MM-control-01 ${SOURCES} ${GITREV_TMP})
target_include_directories(MM-control-01 PRIVATE board core ${CMAKE_BINARY_DIR})


add_custom_command(
	OUTPUT		MM-control-01.hex
	MAIN_DEPENDENCY	MM-control-01.hex.tmp
	DEPENDS		${CMAKE_SOURCE_DIR}/MM-control-01.hex.in
	COMMAND		${CAT}
	ARGS		${CMAKE_SOURCE_DIR}/MM-control-01.hex.in
			MM-control-01.hex.tmp
			> MM-control-01.hex
	COMMENT		"Generating HEX image"
	VERBATIM
)

add_custom_command(
	OUTPUT		MM-control-01.hex.tmp
	MAIN_DEPENDENCY	MM-control-01
	COMMAND		${CMAKE_OBJCOPY}
	ARGS		-O ihex -R .eeprom
			MM-control-01
			MM-control-01.hex.tmp
	VERBATIM
)

add_custom_target(
	hex ALL
	DEPENDS MM-control-01.hex
)



add_custom_command(
  OUTPUT ${GITREV_TMP}
  COMMAND ${CMAKE_COMMAND} -E echo_append "#define FW_LOCAL_CHANGES " > ${GITREV_TMP}
  COMMAND ${GIT_EXECUTABLE} diff-index --quiet HEAD -- && echo 0 >> ${GITREV_TMP} || echo 1 >> ${GITREV_TMP}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GITREV_TMP} ${GITREV_FILE}
  COMMAND ${CMAKE_COMMAND} -E remove ${GITREV_TMP}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  VERBATIM
)

if ("Windows" STREQUAL CMAKE_HOST_SYSTEM_NAME)
configure_file(
${CMAKE_SOURCE_DIR}/build.bat.in
build.bat
)
endif()
