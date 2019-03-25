 /****************************************************************************
 Module
   Slave.ino

 Revision
   4.0.0

 Description  
    Script for slave microcontrollers in the shape display. 
    Handles 6 pins.

 Author
    Alexa Siu <afsiu@stanford.edu>
  
 History
 When             Who     What/Why
 --------------   ---     --------
****************************************************************************/

#include "ShapePin.h"
#include "ShapeConstants.h"
#include "Teensy-pin.h"      // Teensy pin definitions
#include "RS485_protocol.h"  // library with error-checking protocol
#include <EEPROM.h>

//-----------EEPROM Definitions & Variables-----------------//
int EEPROMAddress = 0;
byte myID = EEPROM.read(EEPROMAddress);       // Slave address
Mapping mapping(myID);                        // Map pins based on Slave ID

// Switch ISR
volatile bool switchStatus[6] = {false, false, false, false, false, false};
bool lastSwitchStatus[6] = {false, false, false, false, false, false};
typedef void (*GenericSwitchISR)(void);
GenericSwitchISR switchISR[6] = { &Switch0_ISR, &Switch1_ISR, &Switch2_ISR,
                                  &Switch3_ISR, &Switch4_ISR, &Switch5_ISR
                                };                          

//----------ShapePins -----------//
ShapePin pins[NUM_MOTORS] = {
  ShapePin ( 0, true, mapping.pin_motor[0], mapping.pin_motor[1],
  mapping.pin_encoder[0], mapping.pin_encoder[1] ),
  ShapePin ( 1, true, mapping.pin_motor[2], mapping.pin_motor[3],
  mapping.pin_encoder[2], mapping.pin_encoder[3] ),
  ShapePin ( 2, true, mapping.pin_motor[4], mapping.pin_motor[5],
  mapping.pin_encoder[4], mapping.pin_encoder[5] ),
  ShapePin ( 3, true, mapping.pin_motor[6], mapping.pin_motor[7],
  mapping.pin_encoder[6], mapping.pin_encoder[7] ),
  ShapePin ( 4, true, mapping.pin_motor[8], mapping.pin_motor[9],
  mapping.pin_encoder[8], mapping.pin_encoder[9] ),
  ShapePin ( 5, true, mapping.pin_motor[10], mapping.pin_motor[11],
  mapping.pin_encoder[10], mapping.pin_encoder[11] )
};

// Serial commands
const int bytes2Read = 5;
char inputArr[bytes2Read];  // store user input here     
int numRcvd;      
int cmd;
int pinNum;
int target;
const char delim[2] = "-";
const int singlePinSpeed = 255;
// commands for moving single pin
#define MOVE_PIN        1
#define STOP_PIN        2
#define ZERO_PIN        3
#define MOVE_ALL_PINS   4
#define STOP_ALL_PINS   5
#define ZERO_ALL_PINS   6

void setup() {
  Serial.begin(115200);  
  Serial.setTimeout(2);
  delay(1000);
  setupRS485();

  // setup switches and interrupt
  setupSwitches();

  Serial.println("Shape Display Firmware v04: Slave");
  Serial.printf( "Slave ID: %d\n", myID);
  #if PULSES_3
    Serial.println("Encoder: 3-ticks");
  #elif PULSES_4
    Serial.println("Encoder: 4-ticks | 03/14/2018");
  #else
    Serial.println("Encoder: error");
  #endif 
  Serial.println("-------------------------------------------\n");
}

void loop() {
  // read any messages from RS485
  readMSG();

  // analog switches need to be polled
  updateAnalogSwitches();

  // run the pins state machine
  RunPinsSM();

} //end loop

// read serial in case we don't want to use RS485 
// Command format: cmd-pinNum*-target*
//          *denotes optional parameters
// Serial commands:
//  - Move one pin: 1-pinNum-(1,2,or 3) 
//    1 will move the pin to 20 mm 
//    2 will move the pin to 35 mm
//    3 will move the pin to 50 mm
//  - Stop one pin: 2-pinNum
//  - Zero one pin: 3-pinNum
//  - Move all pins: 4
//  - Stop all pins: 5
//  - Zero all pins: 6
void readSerial() {
  
    // read user input
    numRcvd = Serial.readBytes( inputArr, bytes2Read );
    // cmd-pinNum-target*

    char *token;
    // check if we received the correct data
    if ( numRcvd > 0 ) {
      
      Serial.print("received cmd: ");
      Serial.println(inputArr);
    
      // first get the cmd byte
      token = strtok(inputArr, delim);
      cmd = atoi(token);

      int targetPos = 0;
      
      // cmd is target position
      switch ( cmd ) {
        case MOVE_PIN:
          token = strtok(NULL, delim);
          pinNum = atoi( token );
          token = strtok(NULL, delim);
          target = atoi( token );
          if ( pinNum >= 0 && pinNum < NUM_MOTORS ) {
            if (target == 1) {
              targetPos = 20;
              pins[pinNum].CommandTargetPos(targetPos);
            } else if (target == 2) {
              targetPos = 35;
              pins[pinNum].CommandTargetPos(targetPos);
            } else if (target == 3) {
              targetPos = 50;
              pins[pinNum].CommandTargetPos(targetPos);
            } 
            Serial.printf("pin %d, target %d\n", pinNum, targetPos);
          } else {
            Serial.printf("cmd for invalid pin num %d. Pin num should be between 0-5\n", pinNum);
          }
          
        break;
        case STOP_PIN:          
          token = strtok(NULL, delim);
          pinNum = atoi( token );
          if ( pinNum >= 0 && pinNum < NUM_MOTORS ) {
            pins[pinNum].Idle();
            Serial.printf("stop pin %d\n", pinNum);
          } else {
            Serial.printf("stop cmd for invalid pin num %d. Pin num should be between 0-5\n", pinNum);
          }
        break;
        case ZERO_PIN:
          token = strtok(NULL, delim);
          pinNum = atoi( token );
          if ( pinNum >= 0 && pinNum < NUM_MOTORS ) {
            pins[pinNum].Zero();
            Serial.printf("zero pin %d\n", pinNum);
          } else {
            Serial.printf("zero cmd for invalid pin num %d. Pin num should be between 0-5\n", pinNum);
          }
        break;
        case MOVE_ALL_PINS:
          token = strtok(NULL, delim);
          target = atoi( token );
          if (target == 1) {
            targetPos = 20;
            CommandPins(targetPos);
          } else if (target == 2) {
            targetPos = 35;
            CommandPins(targetPos);
          } else if (target == 3) {
            targetPos = 50;
            CommandPins(targetPos);
          } 
          Serial.printf("moving all pins to %d\n", targetPos);
        break;
        case STOP_ALL_PINS:
          StopPins();
          Serial.printf("stopping all pins\n");
        break;
        case ZERO_ALL_PINS:
          ZeroPins();
          Serial.printf("zeroing all pins\n");
        break;
      }
      Serial.println("");
      Serial.flush();
    }
  
}

// Command all pins
void CommandPins ( int newPos ) {
  for (int i = 0; i < NUM_MOTORS; i++) {
    pins[i].CommandTargetPos( newPos );
  }
}

// Zero all pins
void ZeroPins ( void ) {
  for (int i = 0; i < NUM_MOTORS; i++) {
    pins[i].Zero();
  }
}

// Stop all pins
void StopPins ( void ) {
  for (int i = 0; i < NUM_MOTORS; i++) {
    pins[i].Idle();
  }
}

// Run state machine for all pins
void RunPinsSM ( void ) {
  // run each pin's state machine
  for (int i = 0; i < NUM_MOTORS; i++) {
    pins[i].RunSM();
  }
}

//----------------------Switch Functions-----------------------
void setupSwitches () {
  for (int i = 0; i < NUM_MOTORS; i++) {
    
    pinMode( mapping.pin_switch[i], INPUT ); 
    
    // If we're using an MCU with analog switches (MCU_ID is 0)
    if ((myID % 4 == 3) && ( (i == 0) || ((i == 4) || (i == 5)))) {
      // Set analog switch status to true if above threshold
      pins[i].SwitchISR( analogRead(mapping.pin_switch[i]) > ANALOG_SW_THRESH );
    }
    // Otherwise read digitally
    else {
      if ( digitalRead(mapping.pin_switch[i]) == HIGH ) {
        pins[i].SwitchISR(true);
      } else {
        pins[i].SwitchISR(false);
      }
      // Attach interrupts to digital switches
      attachInterrupt ( digitalPinToInterrupt(mapping.pin_switch[i]), switchISR[i], CHANGE);
    }
  } //endfor
  
}

// For analog switches, need to manually call 'ISR function'
void updateAnalogSwitches() {
  for (int i = 0; i < NUM_MOTORS; i++) {
    // If we're using an MCU with analog switches (MCU_ID is 0)
    if ((myID % 4 == 3) && ( (i == 0) || ((i == 4) || (i == 5)) ) ) {
      // Read analog switch status
      pins[i].SwitchISR( analogRead(mapping.pin_switch[i]) > ANALOG_SW_THRESH );
    }
  }
}

//---------------------RS485 Functions-----------------//
void setupRS485( void ) {
  pinMode(SSerialTxControl, OUTPUT);
  digitalWrite(SSerialTxControl, RS485Receive);  // Init Transceiver
  RS485Serial.begin(1000000);
}

// read messages sent through RS485
void readMSG() {
  byte msgReceived[MAX_MSG_SIZE]; // Create a buffer
  byte receivedMsgLen = receiveMsg(msgReceived);

  // declare variabless outside switch so compiler is happy
  int pinNum, newKp, newKi, newKd, newSpeed; 
  
  // Length of msg (length > 0 for real msg)
  if ( receivedMsgLen ) {
    // Then parse the msg
    // First check if it's a msg for this device
    if ( (msgReceived[MSG_ADDR] != myID) && (msgReceived[MSG_ADDR] != UNIVERSAL_SLAVE_ID) ) {
      //Serial.println("Message is not for me.");
      return; //return

      // Check it's a valid command
    } else if ( msgReceived[MSG_CMD] < 245 ) {
      //Serial.println("Not a valid command.");
      return;

      // Else it is for us and it is a valid command
    } else {
      // Get the command type
      byte msgCmd = msgReceived[MSG_CMD];
      // Based on the cmd type choose on of the cases:
      switch ( msgCmd ) {

        case SET_POS:   // Set new setpoints
          //Serial.println("Set positions.");
          for (int i = 0; i < NUM_MOTORS; i++) {
            pins[i].CommandTargetPos( int( msgReceived[MSG_DATA + i] ) );
            if ( i == 5 && myID%8 == 3 && msgReceived[MSG_DATA + i] > 30 ) {
              pins[i].CommandTargetPos( 25 );
            }
          }
          break;

        case SET_KP:  // Set PID gains
          //Serial.println("Set gains.");
          // PID PACKET STRUCTURE
          // [ID]  [CMD]  [PIN#]  [KP]

          pinNum = int(msgReceived[MSG_DATA]);
          newKp = int(msgReceived[MSG_DATA + 1]);		
          pins[pinNum].SetKp(newKp); 
          break;

        case SET_KI:  // Set PID gains
          // PACKET STRUCTURE
          // [ID]  [CMD]  [PIN#]  [KI]

          pinNum = int(msgReceived[MSG_DATA]);
          newKi = int(msgReceived[MSG_DATA + 1]);
          pins[pinNum].SetKi(newKi); 
          break;

        case SET_KD:  // Set PID gains
          // PACKET STRUCTURE
          // [ID]  [CMD]  [PIN#]  [KD]

          pinNum = int(msgReceived[MSG_DATA]);
          newKd = int(msgReceived[MSG_DATA + 1]);
          pins[pinNum].SetKd(newKd); 
          break;

        case SET_MAXSPEED:
          //  PACKET STRUCTURE
          // [ID]  [CMD]  [PIN#]  [SPEED]
          pinNum = int(msgReceived[MSG_DATA]);
          newSpeed = int(msgReceived[MSG_DATA + 1]);
          pins[pinNum].SetMaxSpeed(newSpeed); 
          break;

        case SET_MINSPEED:
          //  PACKET STRUCTURE
          // [ID]  [CMD]  [PIN#]  [SPEED]
          pinNum = int(msgReceived[MSG_DATA]);
          newSpeed = int(msgReceived[MSG_DATA + 1]);
          pins[pinNum].SetMinSpeed(newSpeed); 
          break;

        case DISABLE_PIN: 
          //  PACKET STRUCTURE
          // [ID]  [CMD]  [PIN#]  [GARBAGE]

          pinNum = int(msgReceived[MSG_DATA]);
          pins[pinNum].DisableShapePin(); 
          break;

        case ZERO_MOTORS:   // Re-zero motors
//          Serial.println("Zeroing motors.");
          ZeroPins();
          break;

        case STOP_MOTORS:   // Stop all motors
          //Serial.println("Stopping motors.");
          StopPins();
          break;

        case SET_DEADZONE:
          for (int i = 0; i < NUM_MOTORS; i++) {
            pins[i].SetDeadzone( int(msgReceived[MSG_DATA]) );
          }
          break;

        case SET_MAX_TRAVEL:
          for (int i = 0; i < NUM_MOTORS; i++) {
            pins[i].SetMaxTravel( int(msgReceived[MSG_DATA]) );
          }
        break;

      } //end switch
    } //endif valid cmd
  } //endif msg received
}

/*
    receiveMsg

    Description
      Receives RS485 message and saves it to
      the passed in byte array buffer

    Parameters
      Byte array where the received msg will
      be stored

    Returns
      0 if it was nothing, otherwise returns
      length of msg received

*/
byte receiveMsg(byte *msgBuffer) {
  //  return recvMsg (fAvailable, fRead, msgBuffer, sizeof msgBuffer);
  return recvMsg (fAvailable, fRead, msgBuffer, MSG_LENGTH, 8);
}

/*
    sendMsg

    Description
      Sends RS485 message from Master

    Parameters
      Message to send as a byte array

    Returns
      None

*/
void sendMsg( byte* msg ) {
  // Enable the transmit pin
  RS485Serial.transmitterEnable(SSerialTxControl);
  // Send the message
  sendMsg (fWrite, msg, sizeof msg);
}

/*
    printByteArray

    Description
      Print a byte array msg to the serial monitor

    Parameters
      Takes a byte array with the message to
      print

    Returns
      None
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
    fWrite

   Callback functions used by RS495_protocol
   to write to RS485 hardware serial

*/
void fWrite (const byte msg) {
  RS485Serial.write (msg);
}

/*
   fAvailable

   Callback functions used by RS495_protocol
   to check if there's any bytes being transmitted

*/
int fAvailable () {
  return RS485Serial.available();
}

/*
   fRead

   Callback functions used by RS495_protocol
   to read from RS485 hardware serial

*/
int fRead () {
  return RS485Serial.read();
}

//--------------------- Switch ISR -----------------//
void Switch0_ISR ( void ) {
  if ( digitalRead(mapping.pin_switch[0]) == LOW ) {
    pins[0].SwitchISR(false);
  } else {
    pins[0].SwitchISR(true);
  }
}


void Switch1_ISR ( void ) {
  if ( digitalRead(mapping.pin_switch[1]) == LOW ) {
    pins[1].SwitchISR(false);
  } else {
    pins[1].SwitchISR(true);
  }
}

void Switch2_ISR ( void ) {
  if ( digitalRead(mapping.pin_switch[2]) == LOW ) {
    pins[2].SwitchISR(false);
  } else {
    pins[2].SwitchISR(true);
  }
}

void Switch3_ISR ( void ) {
  if ( digitalRead(mapping.pin_switch[3]) == LOW ) {
    pins[3].SwitchISR(false);
  } else {
    pins[3].SwitchISR(true);
  }
}

void Switch4_ISR ( void ) {
  if ( digitalRead(mapping.pin_switch[4]) == LOW ) {
    pins[4].SwitchISR(false);
  } else {
    pins[4].SwitchISR(true);
  }
}

void Switch5_ISR ( void ) {
  if ( digitalRead(mapping.pin_switch[5]) == LOW ) {
    pins[5].SwitchISR(false);
  } else {
    pins[5].SwitchISR(true);
  }
}
