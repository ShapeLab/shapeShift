 /****************************************************************************
 Module
   Master-Unity.ino

 Revision
   4.0.0

 Description
    Script that talks through Serial to Unity program. 
    Based on Unity commands, it sends information to 
    the shape display hardware through RS485.

    Msg format from Unity: 
      Byte  0 : command byte (data, zero, stop, etc...)
      Byte 1-n: any data bytes

 Author
    Alexa Siu <afsiu@stanford.edu>
  
 History
 When             Who     What/Why
 --------------   ---     --------
****************************************************************************/
// Debug booleans | If true, system will have some significant delays
bool debug = false;         // true if want minimal print statements
bool debugData = false;     // true if want to print the data Teensy receives from unity
bool sendRS485msg = true;   // true if connected to RS485
bool ledOnSerialReceive = true;
bool ledOnRS485send = !ledOnSerialReceive;
unsigned long receivedMsgStartTime;

/// IMPORTANT to update these otherwise arrays will
//            random register values
// Set the physical display parameters
const int displaySizeX = 12;                        // number of rows
const int displaySizeZ = 24;                        // pins per row
const int displaySize = displaySizeX*displaySizeZ;  // display size
char zMap[displaySize];         // buffer to store data sent from Unity
int numRcvd;                    // keep count of how many bytes are received
const int setupSize = 4;              // size of setup command sent from Unity
char setupData[setupSize];      // buffer for storing setup command from Unity
#define PINS_PER_MCU 6

#include "RS485_protocol.h"  // library with error-checking protocol
#include <EEPROM.h>

// This board's address
int EEPROMAddress = 0;
byte myID = 0;

// SM states
typedef enum { SENDING, FORWARDING_SETUP, WAITING_2_RECEIVE, WAITING_4_CMD } MasterState_t;
MasterState_t currentState = WAITING_4_CMD;

// RS485 variables
#define RS485Serial Serial1 
#define SSerialRX        0     // Serial Receive pin
#define SSerialTX        1     // Serial Transmit pin
#define SSerialTxControl 2     // RS485 Transmit control
#define RS485Transmit    HIGH
#define RS485Receive     LOW

// Msg definitions
#define MAX_MSG_SIZE 63
#define MSG_LENGTH 8
#define SLAVE_ID 1
#define UNIVERSAL_SLAVE_ID 255

// Msg types from unity
#define DataCMD   127
#define ZeroCMD   126
#define StopCMD   125
#define SetupCMD  124

// Msg types for hardware slave
#define MSG_ADDR 0  // bit 0 is the slave ID
#define MSG_CMD  1  // bit 1 is the msg command/function
#define MSG_DATA 2  // bit 2 and higher is any data

// Msg commands for hardware slave
#define SET_POS           254   // set pins position
#define SET_KP            253   // set Kp gain
#define ZERO_MOTORS       252   // re-zero the motors
#define STOP_MOTORS       251   // stop motors
#define SET_DEADZONE      250   // set deadzone
#define SET_MAXSPEED      249   // set max pin speed (0-255)
#define SET_MINSPEED      248   // set max pin speed (0-255)
#define SET_KI            247   // set Ki gain
#define SET_KD            246   // set Kd gain
#define DISABLE_PIN       245   // disable pin

// LED for debugging
#define ledPin 13
bool ledOn = false;

void setup() {

  // Assign ID
  myID = EEPROM.read(EEPROMAddress);
  
  // Start the built-in serial port, probably to Serial Monitor
  Serial.begin(115200);
  
  // LED setup
  pinMode(ledPin, OUTPUT);  
  digitalWrite(  ledPin, HIGH );

  // RS485 setup
  pinMode(SSerialTxControl, OUTPUT);
  digitalWrite(SSerialTxControl, RS485Receive);  // Init Transceiver
  // Start the software serial port, to another device
  RS485Serial.begin(1000000); // set the data rate 
  delay(1000);
  
  // Unity setup
  // Initialize the buffer with 0s
  for ( int  i = 0; i < displaySize; i++ ) {
    zMap[i] = 0;
  }

  Serial.println(F("Teensy: Initialized shape display Master - v04"));

}

void loop() {
// Latency measurement
//  unsigned long lastTime = micros();
//  SendNewPositions();
//  Serial.printf("Time to send all pin position: %i micros \n", (micros() - lastTime));
//  Serial.println();

  
  switch ( currentState ) {

//    // Master will request the display size and won't do anything
//    // until this is received
//    case INIT:
//    break;

    // in this state the master will wait for a cmd from Unity
    // Current commands include:
    //  - DataCMD to update the pin positions
    //  - SetupCMD to forward setup parameters
    //  - ZeroCMD to reset the display back to all zeros
    //  - StopCMD to stop and reset the display 
    case WAITING_4_CMD:
      numRcvd = 0;
      char cmd[1];
      // Read the bytes from serial
      // Expecting bytes to be equal to one cmd byte
      
      numRcvd = Serial.readBytes( cmd, 1 );
      if ( numRcvd > 0 ) {
        if ( ledOnSerialReceive ) {
          digitalWrite( ledPin, HIGH ); 
        }
        if (  ( (int)cmd[0] )  == DataCMD ) {
          if (debug) {
            receivedMsgStartTime = micros();
            Serial.println( F("Teensy: Received a data cmd.") );
          }
          currentState = WAITING_2_RECEIVE;
          break;
        } else if (  ( (int)cmd[0] ) == SetupCMD ) {
          if ( debug ) {
            Serial.println( F("Teensy:  Received a setup cmd.") );
          }
          currentState = FORWARDING_SETUP;
        } else if (  ( (int)cmd[0] ) == ZeroCMD ) {
          if ( debug ) {
            Serial.println( F("Teensy:  Received a zeroing cmd.") );
          }
          ZeroDisplay();
        } else if (  ( (int)cmd[0] )  == StopCMD ) {
          if ( debug ) {
            Serial.println( F("Teensy: Received a stop cmd.") );
          }
          StopDisplay();
        } 
      } //endif
      //Serial.flush();  // clear the buffer
      if ( ledOnSerialReceive ) {
        digitalWrite( ledPin, LOW );
      }
    break;

    // wait for setup commmand from unity, forward the message when received
    case FORWARDING_SETUP:
    
      numRcvd = 0;
      // Read the bytes from serial into setup data buffer
      // Expecting bytes to be equal to setupSize 
       numRcvd = Serial.readBytes( setupData, setupSize );
      // Check-sum to ensure correct amount of data was received
      if ( numRcvd == setupSize ) {
        if ( debug ) {
          Serial.printf( F("Teensy: received %i setup bytes for Slave %i.\n"), numRcvd, (int)setupData[0] );
          Serial.printf( F("Teensy: Sending setup msg: %i %i %i %i.\n"), (int)setupData[0], (int)setupData[1], (int)setupData[2], (int)setupData[3]);
        }

        // forward message to display
        byte msg[4] = {setupData[0], setupData[1], 
                      setupData[2], setupData[3]};
        sendMsg(msg, 4);
        
        currentState = WAITING_4_CMD;
        //Serial.flush();  // clear the buffer
   
      } else if ( numRcvd > 0 ) {
        if ( debug ) {
          Serial.printf( F("Teensy: received %i bytes; but expecting %i\n"), numRcvd, setupSize );
        } 
      }
    break;
    
    // in this state, the master will wait for pin display data from unity
    case WAITING_2_RECEIVE:
      numRcvd = 0;
      // Read the bytes from serial into zMap buffer
      // Expecting bytes to be equal to displaySize 
      if ( ledOnSerialReceive ) {
        digitalWrite( ledPin, HIGH );   
      }
      numRcvd = Serial.readBytes( zMap, displaySize );
      if ( ledOnSerialReceive ) {
        digitalWrite( ledPin, LOW );
      }
      Serial.flush();  // clear the buffer
      // Check-sum to ensure correct amount of data was received
      if ( numRcvd == displaySize ) {
        if ( debug ) {
          Serial.printf( F("Teensy: received %i floats.\n"), numRcvd );
          Serial.printf( F("Teensy: Time to receive was %i micros. \n"), micros()-receivedMsgStartTime );
        }
        // change states to send to the display
        currentState = SENDING;
      } else {
        if ( numRcvd > 0 ) {
           if ( debug ) {
            Serial.printf( F("Teensy: received %i floats; but expecting %i\n"), numRcvd, displaySize );
            } 
        }
        currentState = WAITING_4_CMD;
      }
    break; // break WAITING_2_RECEIVE

    // in this state, the master will send data to the shape display
    case SENDING:
      
      // LED will be ON when teensy is not listening
      if (ledOnRS485send ) {
        digitalWrite(ledPin, HIGH);
      }
      if (sendRS485msg) {
        SendNewPositions();
      }
      if (ledOnRS485send) {
        digitalWrite( ledPin, LOW );
      }

      if ( debug ) {
        Serial.println( F("Teensy: Sending to RS485") );
       int countZeros = 0;
        // send the data back so we can check if it received correctly
        if ( debugData ) {
          for ( int i = 0; i < displaySize; i++ ) {
            //Serial.printf( F("\nTeensy data %i: %i \n"), i, zMap[i] );
            if ( zMap[i] == 0 ) {
              digitalWrite( ledPin, HIGH ); 
              countZeros++;
            }
          } //endfor
          //Serial.printf( F("Teensy: count zeros: %i\n"), countZeros );
        } //endif debugdata

      } // end if debug

      // change states back to WAITING_4_CMD
      // so we can wait for a new refresh msg
      currentState = WAITING_4_CMD;
      

      // LED will be OFF when teensy is listening
      //digitalWrite( ledPin, LOW );

      Serial.flush();  // clear the buffer
      
    break; // break SENDING
    
  } // end switch
  
} // end loop

// Decode position data for full display sent from Unity and send 
// to hardware display through RS485
void SendNewPositions( void ) {
  // NOTE:  8 add currently added to slave IDs since hardware is actually MCU 8-15
  //        To apply same values to all rows, remove rowOffset variable 

  static byte msg[] = {0,0,0,0,0,0,0,0};
  int MCUoffset = 0;
  int rowNum = 0;
  int rowOffset = 0;
  
  // iterate through slave mcu ids
  for (int SlaveID = 0; SlaveID < displaySizeX*4; SlaveID++) {
      
    // Grab row offset
    rowNum = floor(SlaveID/4);
    rowOffset = rowNum*displaySizeZ;
 
    // Even Row (front of half-module)
    if (SlaveID%8 < 4) {
      MCUoffset = (SlaveID%4)*PINS_PER_MCU;
      msg[0] = SlaveID;
      msg[1] = SET_POS;
      //Serial.printf("MCU: %i, ", SlaveID);
      for (int i = 2, j = 0; i < 9; i++, j++ ) {
        msg[i] = zMap[j+MCUoffset+rowOffset];
        //Serial.printf("%i, ", msg[i]);
      }
            //Serial.printf(", rn: %i, ro: %i \n", rowNum, rowOffset);
      sendMsg(msg, 8);
    }
    
    // Odd Row (back of half-module, flipped pin order)
    else {
      MCUoffset = abs((SlaveID%4)-3)*PINS_PER_MCU;
      msg[0] = SlaveID;
      msg[1] = SET_POS;
      //Serial.printf("MCU: %i, ", SlaveID);
      for (int i = 2, j = 5; i < 9; i++, j-- ) {
        msg[i] = zMap[j+MCUoffset+rowOffset];
        //Serial.printf("%i, ", msg[i]);
      }
            //Serial.printf(", rn: %i, ro: %i \n", rowNum, rowOffset);
      sendMsg(msg, 8);
    }
    
  }
  
}

// Send a zeroing command to the hardware display
void ZeroDisplay ( void ) {
  static byte msg[2] = {
    UNIVERSAL_SLAVE_ID, ZERO_MOTORS
  };
  // Send the message
  sendMsg(msg, 2);
}

// Send a stop command to the hardware display
void StopDisplay ( void ) {
  static byte msg[2] = {
    UNIVERSAL_SLAVE_ID, STOP_MOTORS
  };
  // Send the message
  sendMsg(msg, 2);
}

// Send a command to set max speed
void SetMaxSpeed ( char ID, char pin, char speed ) {
  static byte msg[4] = {
    ID, SET_MAXSPEED, pin, speed
  };
  msg[0] = ID;
  msg[2] = pin;
  msg[3] = speed;
  
  // Send the message
  sendMsg(msg, 4);
}

// Send a command to set min speed
void SetMinSpeed ( char ID, char pin, char speed ) {
  static byte msg[4] = {
    ID, SET_MINSPEED, pin, speed
  };
  msg[0] = ID;
  msg[2] = pin;
  msg[3] = speed;
  
  // Send the message
  sendMsg(msg, 4);
}

// Send a command to set Kp
void SetKp ( char ID, char pin, char kp ) {
  static byte msg[4] = {
    ID, SET_KP, pin, kp
  };
  msg[0] = ID;
  msg[2] = pin;
  msg[3] = kp;
  // Send the message
  sendMsg(msg, 4);
}

// Send a command to set Ki
void SetKi ( char ID, char pin, char ki ) {
  static byte msg[4] = {
    ID, SET_KI, pin, ki
  };
  msg[0] = ID;
  msg[2] = pin;
  msg[3] = ki;
  // Send the message
  sendMsg(msg, 4);
}

// Send a command to set Kd
void SetKd ( char ID, char pin, char kd ) {
  static byte msg[4] = {
    ID, SET_KD, pin, kd
  };
  msg[0] = ID;
  msg[2] = pin;
  msg[3] = kd;
  // Send the message
  sendMsg(msg, 4);
}

// Toggle LED on Pin 13
void toggleLED  ( void ) {
  if ( ledOn == LOW ) {
    digitalWrite( ledPin, HIGH );
  } else if ( ledOn == HIGH ) {
    digitalWrite( ledPin, LOW );
  }
} // end toggleLED

/* 
 *  receiveMsg
 *  
 *  Description
 *    Receives RS485 message and saves it to 
 *    the passed in byte array buffer
 *  
 *  Parameters
 *    Byte array where the received msg will 
 *    be stored
 *    
 *  Returns
 *    0 if it was nothing, otherwise returns 
 *    length of msg received
 *    
*/
byte receiveMsg(byte *msgBuffer) {
  return recvMsg (fAvailable, fRead, msgBuffer, sizeof msgBuffer);
}


/* 
 *  sendMsg
 *  
 *  Description
 *    Sends RS485 message from Master
 *    
 *  Parameters 
 *    Message to send as a byte array 
 *    By default sends 8 bytes
 *    
 *  Returns
 *    None
 *    
*/
void sendMsg( byte* msg ) {
  // Enable the transmit pin
  RS485Serial.transmitterEnable(SSerialTxControl);
  // Send the message
  sendMsg (fWrite, msg, MSG_LENGTH);  
}

/* 
 *  sendMsg
 *  
 *  Description
 *    Sends RS485 message from Master
 *    
 *  Parameters 
 *    Message to send as a byte array 
 *    Length of the message to send
 *    
 *  Returns
 *    None
 *    
*/
void sendMsg( byte* msg, int len ) {
  // Enable the transmit pin
  RS485Serial.transmitterEnable(SSerialTxControl);
  // Send the message
  sendMsg (fWrite, msg, len);  
}

/* 
 *  printByteArray
 *  
 *  Description
 *    Print a byte array msg to the serial monitor
 *    
 *  Parameters
 *    Takes a byte array with the message to 
 *    print
 *    
 *  Returns
 *    None
 */
void printByteArray(byte arr[], int len) {
  Serial.print( "msg: " );
  for ( int i = 0; i < len; i++ ) {
    Serial.print( arr[i] );
    Serial.print(" " );
  }
  Serial.println(".");
} // end printByteArray

/* 
 *  fWrite
 *  
 * Callback functions used by RS495_protocol
 * to write to RS485 hardware serial
 * 
*/
void fWrite (const byte msg) {
  RS485Serial.write (msg);  
}

/*
 * fAvailable
 * 
 * Callback functions used by RS495_protocol
 * to check if there's any bytes being transmitted
 * 
*/
int fAvailable () {
  return RS485Serial.available();
}

/*
 * fRead
 * 
 * Callback functions used by RS495_protocol
 * to read from RS485 hardware serial
 * 
*/
int fRead () {
  return RS485Serial.read();
}

