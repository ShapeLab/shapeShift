/****************************************************************************

  Header file for ShapeConstants.h
  Constants used by the shape display

 ****************************************************************************/
 
#ifndef _HIGH_RES_TEENSY_CONSTANTS_H
#define _HIGH_RES_TEENSY_CONSTANTS_H

// If using 3-tick encoder change PULSES_3 to 1
// Else set to 0
#define PULSES_3  0
#if PULSES_3
  #define PULSES_4  0
#else 
  #define PULSES_4  1
#endif

#define NUM_MOTORS      6

// optimmize encoder readings
#define ENCODER_USE_INTERRUPTS
#define ENCODER_OPTIMIZE_INTERRUPTS    

//------------RS485 Definitions & Variables-----------------
#define RS485Serial Serial1    // Using hardware serial for Teensy
#define SSerialRX        0     // Serial Receive pin
#define SSerialTX        1     // Serial Transmit pin
#define SSerialTxControl MK26  // RS485 Transmit control
// SSerialRxControl is tied to GND so device
// is always receiving
#define RS485Transmit    HIGH
#define RS485Receive     LOW
#define MAX_MSG_SIZE 8   // Max buffer size
#define MSG_LENGTH 8
#define MASTER_ID 65                  // Master Teensy ID
#define UNIVERSAL_SLAVE_ID 255        // Universal ID

// RS485 MSG IDs 
#define MSG_ADDR 0  // bit 0 is the slave ID
#define MSG_CMD  1  // bit 1 is the msg command/function
#define MSG_DATA 2  // bit 2 and higher is any data

// RS485 MSG ACTIONS
#define SET_POS         	254   // set pins position
#define SET_KP       		  253   // set Kp gain
#define ZERO_MOTORS     	252   // re-zero the motors
#define STOP_MOTORS     	251   // stop motors
#define SET_DEADZONE    	250   // set deadzone
#define SET_MAXSPEED    	249   // set max pin speed (0-255)
#define SET_MINSPEED      248   // set max pin speed (0-255)
#define SET_KI				    247	  // set Ki gain
#define SET_KD				    246   // set Kd gain
#define DISABLE_PIN       245   // disable pin
#define SET_MAX_TRAVEL    244   // disable pin

//----------Translation Definitions & Variables-----------
#define UP                 1
#define DOWN               -1
#define DEBOUNCE_DELAY     1
#define SWITCH_THRESH      700  // HIGH threshold for analog switches 
#define DEFAULT_SPEED      250
#define DEFAULT_MAX_TRAVEL 60

// Rotary to linear conversion variables
#if PULSES_3 // number of marks in the encoder
  #define PULSES_PER_REV  3.0f   
#elif PULSES_4
  #define PULSES_PER_REV  4.0f   
#endif
#define SCREW_PITCH     3.0f   // [mm/rev]
#define ZERO_OFFSET     -10  // switch position [mm]
#define ZERO_POS        0

// Stall-check variables
#define STALL_TIME      2000 // stall time threshold in ms
#define MAX_ZERO_TIME   6000 // max time for zeroing in ms   

// Conversion Factors - Multiply to convert e.g. 8 mm * MM_TO_PULSE = X pulses 
#define PULSE_TO_MM     ((1.0f/ ( PULSES_PER_REV / SCREW_PITCH )) * (1 / 4.0f)) // [mm/pulse]
#define MM_TO_PULSE     1/PULSE_TO_MM  //[pulse/mm]

// Threshold for special case switch connected to analog pin
#define ANALOG_SW_THRESH 950

#endif

