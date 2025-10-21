// STM32L431CCT6 module pinout made by VasiliSk under MIT license
#pragma once

/*----------------------------------------------------------------------------
 *        STM32 pins number
 *----------------------------------------------------------------------------*/

// Not available
// PC14
// PC15

#define NUM_DIGITAL_PINS        38
#define NUM_ANALOG_INPUTS       10

// On-board LED pin number
#ifndef LED_BUILTIN
  #define LED_BUILTIN           PB_2
#endif

// On-board user button
#ifndef USER_BTN
  #define USER_BTN              PH_3
#endif

// SPI1 definitions
#ifndef PIN_SPI_SS
  #define PIN_SPI_SS            PA_12
#endif
#ifndef PIN_SPI_SS2
  #define PIN_SPI_SS2           PB_14
#endif
#ifndef PIN_SPI_SS3
  #define PIN_SPI_SS3           PNUM_NOT_DEFINED
#endif
#ifndef PIN_SPI_MOSI
  #define PIN_SPI_MOSI           PB_5
#endif
#ifndef PIN_SPI_MISO
  #define PIN_SPI_MISO           PB_4 
#endif
#ifndef PIN_SPI_SCK
  #define PIN_SPI_SCK            PB_3
#endif

// I2C2 definitions
#ifndef PIN_WIRE_SDA
  #define PIN_WIRE_SDA          PB_7
#endif
#ifndef PIN_WIRE_SCL
  #define PIN_WIRE_SCL          PB_6
#endif
#ifndef PIN_WIRE_SCS
  #define PIN_WIRE_SCS          PC_13
#endif

// Timer Definitions

// UART Definitions
#ifndef SERIAL_UART_INSTANCE
  #define SERIAL_UART_INSTANCE  2
#endif

// Default pin used for generic 'Serial' instance
// USART1
#if SERIAL_UART_INSTANCE==1
#ifndef PIN_SERIAL_RX
  #define PIN_SERIAL_RX          PA_10
#endif
#ifndef PIN_SERIAL_TX
  #define PIN_SERIAL_TX          PA_9
#endif
#endif

#if SERIAL_UART_INSTANCE==2
#ifndef PIN_SERIAL_RX
  #define PIN_SERIAL_RX          PA_3
#endif
#ifndef PIN_SERIAL_TX
  #define PIN_SERIAL_TX          PA_2
#endif
#endif

// Alias
#ifndef DEBUG_SUBGHZSPI_MOSI
  #define DEBUG_SUBGHZSPI_MOSI  (PA_7 | ALT1)
#endif
#ifndef DEBUG_SUBGHZSPI_MISO
  #define DEBUG_SUBGHZSPI_MISO  (PA_6 | ALT1)
#endif
#ifndef DEBUG_SUBGHZSPI_SCLK
  #define DEBUG_SUBGHZSPI_SCLK  (PA_5 | ALT1)
#endif
#ifndef DEBUG_SUBGHZSPI_SS
  #define DEBUG_SUBGHZSPI_SS    (PA_4 | ALT1)
#endif

// Extra HAL modules
#if !defined(HAL_DAC_MODULE_DISABLED)
  #define HAL_DAC_MODULE_ENABLED
#endif


/*----------------------------------------------------------------------------
 *        Arduino objects - C++ only
 *----------------------------------------------------------------------------*/

#ifdef __cplusplus
  // These serial port names are intended to allow libraries and architecture-neutral
  // sketches to automatically default to the correct port name for a particular type
  // of use.  For example, a GPS module would normally connect to SERIAL_PORT_HARDWARE_OPEN,
  // the first hardware serial port whose RX/TX pins are not dedicated to another use.
  //
  // SERIAL_PORT_MONITOR        Port which normally prints to the Arduino Serial Monitor
  //
  // SERIAL_PORT_USBVIRTUAL     Port which is USB virtual serial
  //
  // SERIAL_PORT_LINUXBRIDGE    Port which connects to a Linux system via Bridge library
  //
  // SERIAL_PORT_HARDWARE       Hardware serial port, physical RX & TX pins.
  //
  // SERIAL_PORT_HARDWARE_OPEN  Hardware serial ports which are open for use.  Their RX & TX
  //                            pins are NOT connected to anything by default.
  #ifndef SERIAL_PORT_MONITOR
    #define SERIAL_PORT_MONITOR   Serial
  #endif
  #ifndef SERIAL_PORT_HARDWARE
    #define SERIAL_PORT_HARDWARE  Serial
  #endif
#endif
