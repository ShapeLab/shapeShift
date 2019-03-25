/****************************************************************************
 Module
   ShapePin.cpp

 Revision
   2.0.0

 Description
   Library for a shape display pin

 Notes
 
 Author
    A. Siu <afsiu@stanford.edu>
   
 History
 When           Who     What/Why
 -------------- ---     --------
 05/18/17 10:30 afs     first pass
 05/19/17 15:04 afs     most functions but interrupts for switch
 05/22/17 21:44 afs     integrated with main function + interrupts
 05/24/17 04:50 afs     wrote our own PID library
 05/25/17 19:40 afs     tested with high gear and low gear ratio motors
 05/31/17 14:11 afs     pins initial state is now idle
 06/29/17 16:28 afs     zeroing now uses pid loop
****************************************************************************/

/*----------------------------- Include Files -----------------------------*/
#include "ShapePin.h"

/*----------------------------- Module Defines ----------------------------*/
#define ENCODER_OPTIMIZE_INTERRUPTS
#define ENCODER_USE_INTERRUPTS

/****************************************************************************

  Public Functions

****************************************************************************/

/****************************************************************************
 Function
    ShapePin

 Parameters
  motorApin: pin for motor lead A
  motorBpin: pin for motor lead B 
  encoderApin: pin for TOP encoder **direction!! otherwise encoder readings will be off
  encoderBpin: pin for BOTTOM encoder
  switchPin: pin for the limit switch
  analog: true if this pin has switch on an analog pin 

 Returns
    None

 Description
    Constructor. Sets the motor, encoder, and switch pins.

 Author
     A. Siu, 05/18/17, 5:00
****************************************************************************/
ShapePin::ShapePin ( int id, bool enabled, int motorA, int motorB, 
                     int encoderA, int encoderB ) 
{
  // initialize variables
  pinID = id;
  pinEnabled = enabled;
  motorApin = motorA;
  motorBpin = motorB;
  encoderApin = encoderA;
  encoderBpin = encoderB;

  /* Control variables */
  targetPos = 0;
  pid_output = 0;
  // Values that work 
  Kp = 1500; //60;  //60 //60;
  Kd = 200;  //40;  //30 //35;
  Ki = 5;    //2;   //3 //25;
  SetDeadzone( 1 ); //[mm]
  SetMaxTravel( DEFAULT_MAX_TRAVEL ); 
  
  // set pins as output/input
  pinMode( motorApin, OUTPUT );
  pinMode( motorBpin, OUTPUT );

  // start with motors off
  Stop();

  // initialize encoder object
  encoder = new Encoder(encoderApin, encoderBpin);

  // set PID mode and limits
  motorPID = new PID( Kp, Ki, Kd );
  motorPID->SetOutputLimits(-DEFAULT_SPEED, DEFAULT_SPEED);
  // sampleTime = speed * res = 75mm/s * 1/0.25mm = 60Hz --> 1/60=0.0167 --> 3.3 ms
  motorPID->SetSampleTime(2);

  // start initially IDLE
  currentPinState = IDLE;

  /* Stall check variables */
  isTraveling = true;
  travelStartTime = millis(); // [ms]
  lastTargetPos = 0.0;        // [pulses]
  travelStartPosition = 0.0;  // [pulses]
}

/****************************************************************************
 Function
    RunSM

 Parameters
    None

 Returns
    None

 Description
    Act according to pin's current state.

 Author
     A. Siu, 05/18/17, 5:00
****************************************************************************/
void ShapePin::RunSM( void ) {
 
  switch (currentPinState) {
    case IDLE:
      Stop();
    break;

    case WAITING4SWITCH:
      CheckSwitch();
      // check if pin is jammed
      CheckIfStalledZeroing();
    break;

    case MOVING2TARGET:
      // Check switch
      CheckSwitch();
      // run PID to move pin
      RunControlLoop();
      // Check if the pin has been stalled
      CheckIfStalled();
    break;

    /// The states below are mostly for debugging ///
    case UP_STATE:
      Move( UP, 150 );
      //RunControlLoopWithoutMoving();
    break;
    
    case DOWN_STATE:
      Move( DOWN, 150 );
      //RunControlLoopWithoutMoving();
    break;

    case DEBUG:
      debug();
    break;
    /// end debug states
    
    default:
    break;
  } //end switch

  //debug();
  
}

void ShapePin::SetState( PinState_t newState ) {
  currentPinState = newState;
}

/****************************************************************************
 Function
  Zero

 Parameters
  None

 Returns
    None

 Description
  Re-zeros pin based on switch.

 Author
     A. Siu, 05/18/17, 5:00
****************************************************************************/
void ShapePin::Zero( void ) {
  currentPinState = WAITING4SWITCH;
  SetMaxSpeed(loweringSpeed);
  SetMinSpeed(loweringSpeed);
  // command pin to keep moving down
  CommandTargetPos ( GetPosMM() - 10000 );
  // keep track of time to check if stalled
  isTraveling = true;
  travelStartTime = millis();
}

/****************************************************************************
 Function
  CommandTargetPos

 Parameters
  newPos: the new pin position to go to [mm]

 Returns
    None

 Description
  Sets a new position for the pin and changes pin state to move

 Author
     A. Siu, 05/18/17, 5:00
****************************************************************************/
void ShapePin::CommandTargetPos ( int newPos ) {
  // cap the max travel
  if ( newPos > maxTravel) {
    newPos = maxTravel;
  }
  targetPos = newPos * MM_TO_PULSE;
  // keep track of time to check if stalled
  isTraveling = true;
  travelStartTime = millis();
  travelStartPosition = GetPosPulses();
  lastTargetPos = targetPos;
  
  // change states to moving
  currentPinState = MOVING2TARGET;
}

/****************************************************************************
 Function
   Idle

 Parameters
  None

 Returns
    None

 Description
  Sets the pin to IDLE

 Author
     A. Siu, 05/18/17, 5:00
****************************************************************************/
void ShapePin::Idle ( void ) {
  Stop();
  currentPinState = IDLE;
}

/****************************************************************************
 Function
  DisableShapePin

 Parameters
  None

 Returns
    None

 Description
  Disable the pin and set state to IDLE.

 Author
     A. Siu, 05/18/17, 5:30
****************************************************************************/
void ShapePin::DisableShapePin( void ) {
  pinEnabled = false;
  Idle();
}

/****************************************************************************
 Function
   SwitchISR

 Parameters
    None

 Returns
    None

 Description
  Called by the switch ISR to toggle the switch state

 Author
     A. Siu, 05/18/17, 5:00
****************************************************************************/
void ShapePin::SwitchISR ( bool switchState ) {
  switchDown = switchState;
}

/****************************************************************************
 Function
   SetSwitchDown

 Parameters
    switchState: true if switch down, false otherwise

 Returns
    None

 Description
    Set the switch status as down or not

 Author
     A. Siu, 05/18/17, 5:00
****************************************************************************/
void ShapePin::SetSwitchDown ( bool switchState ) {
  switchDown = switchState;
}

/****************************************************************************
 Function
   GetSwitchDown

 Parameters
    None

 Returns
    True if switch is down, False otherwise.

 Description
    Set the switch status as down or not

 Author
     A. Siu, 05/18/17, 5:00
****************************************************************************/
bool ShapePin::GetSwitchDown( void ) {
  if (switchDown) {
    return true;
  }
  return false;
}

/****************************************************************************
 Function
  SetKp

 Parameters
  newkp: the new Kp gain value

 Returns
    None

 Description
  Sets Kp value for PID control

 Author
     A. Siu, 05/18/17, 5:00
****************************************************************************/
void ShapePin::SetKp ( int newkp ) {
  Kp = newkp;
}

/****************************************************************************
 Function
  SetKi

 Parameters
  newki: the new Ki gain value

 Returns
    None

 Description
  Sets Ki value for PID control

 Author
     A. Siu, 05/18/17, 5:00
****************************************************************************/
void ShapePin::SetKi ( int newki ) {
  Ki = newki;
}

/****************************************************************************
 Function
  SetKd

 Parameters
  newkd: the new Kd gain value

 Returns
    None

 Description
  Sets Kd value for PID control

 Author
     A. Siu, 05/18/17, 5:00
****************************************************************************/
void ShapePin::SetKd ( int newkd ) {
  Kd = newkd;
}

/****************************************************************************
 Function
   SetMaxSpeed

 Parameters
  speed: the new max duty cycle during PID control

 Returns
    None

 Description
  Sets duty cycle value for when the pin moving upwards

 Author
     A. Siu, 08/15/17, 20:00
****************************************************************************/
void ShapePin::SetMaxSpeed ( int speed ) {
  maxSpeed = speed;
  motorPID->SetOutputLimits(minSpeed, maxSpeed);
}

/****************************************************************************
 Function
   SetMinSpeed

 Parameters
  speed: the new min duty cycle during PID control

 Returns
    None

 Description
  Sets duty cycle value for when the pin moving downwards

 Author
     A. Siu, 08/15/17, 20:00
****************************************************************************/
void ShapePin::SetMinSpeed ( int speed ) {
  minSpeed = -speed;
  motorPID->SetOutputLimits(minSpeed, maxSpeed);
}

/****************************************************************************
 Function
   SetDeadzone

 Parameters
  mm: the new deadzone value in mm

 Returns
    None

 Description
  Sets deadzone for control loop

 Author
     A. Siu, 05/22/17, 22:00
****************************************************************************/
void ShapePin::SetDeadzone ( int mm ) {
  deadzone = (int) (mm * MM_TO_PULSE);
}

/****************************************************************************
 Function
   SetDeadzone_pulses

 Parameters
  pulses: the new deadzone value in pulses (i.e., ticks*4)

 Returns
    None

 Description
  Sets deadzone for control loop in pulses

 Author
     A. Siu, 05/22/17, 22:00
****************************************************************************/
void ShapePin::SetDeadzone_Pulses ( int pulses ) {
  deadzone = pulses;
}

/****************************************************************************
 Function
   SetMaxTravel

 Parameters
  mm: the new max travel value in mm

 Returns
    None

 Description
  Sets the pin's max travel distance

 Author
     A. Siu, 11/04/17, 07:30
****************************************************************************/
void ShapePin::SetMaxTravel ( int maxMM ) {
  maxTravel = maxMM;
}

/****************************************************************************
 Function
  GetPos

 Parameters
  None

 Returns
    None

 Description
  Returns the current pin position in mm

 Author
     A. Siu, 05/18/17, 5:00
****************************************************************************/
int ShapePin::GetPosMM ( void ) {
  return (int) (encoder->read() * PULSE_TO_MM);
}

/****************************************************************************
 Function
   GetPos

 Parameters
  None

 Returns
    None

 Description
  Returns the current pin position in pulses

 Author
     A. Siu, 05/18/17, 5:00
****************************************************************************/
int ShapePin::GetPosPulses ( void ) {
  return encoder->read();
}

/****************************************************************************
 Function
  GetState

 Parameters
  None

 Returns
    None

 Description
  Returns the current pin state 

 Author
     A. Siu, 05/18/17, 5:00
****************************************************************************/
PinState_t ShapePin::GetState( void ) {
  return currentPinState;
}

/****************************************************************************
 Function
   PrintState

 Parameters
    None

 Returns
    None

 Description
  Returns the current pin state in string

 Author
     A. Siu, 06/08/17, 5:00
****************************************************************************/
String ShapePin::PrintState( void ) {
  if ( currentPinState == IDLE ) {
    return "IDLE";
  } else if ( currentPinState == WAITING4SWITCH ) {
    return "WAITING4SWITCHDOWN";
  } else if ( currentPinState == MOVING2TARGET ) {
    return "MOVING2TARGET";
  }
  return "";
}

/****************************************************************************
 
  Private Functions

 ****************************************************************************/

/****************************************************************************
 Function
  RunControlLoop

 Parameters
  None

 Returns
    None

 Description
    Run one cycle of the PID loop. 
    Computes the PID term and outputs it to the motor.

 Author
     A. Siu, 05/18/17, 5:00
****************************************************************************/
void ShapePin::RunControlLoop ( void ) {
  // update the current position
  int currPos = GetPosPulses(); //[pulses * 4]
  // stop motor if we're in the dead zone
  if ( (currPos < (targetPos + deadzone)) && (currPos > (targetPos - deadzone)) ) {
    Stop();
  } else { // compute PID term and control pin
    // calculate PID term (output)
    pid_output = motorPID->Compute(currPos, targetPos);
    // set dir and speed to move the pin
    int speed = abs(pid_output);
    int dir = speed / pid_output;
    Move( dir, speed );
  } // End if deadzone
  currentPinState = MOVING2TARGET;
}

/****************************************************************************
 Function
   RunControlLoopWithoutMoving

 Parameters
  None

 Returns
    None

 Description
    Run one cycle of the PID loop but don't output
    to motors.

 Author
     A. Siu, 05/18/17, 5:00
****************************************************************************/
void ShapePin::RunControlLoopWithoutMoving ( void ) {
  // update the current position
  int currPos = GetPosPulses(); //[pulses * 4]
  // stop motor if we're in the dead zone
  if ( (currPos < (targetPos + deadzone)) && (currPos > (targetPos - deadzone)) ) {
    Stop();
  } else { // compute PID term and control pin
    // calculate PID term (output)
    pid_output = motorPID->Compute(currPos, targetPos);
  } // End if deadzone
  currentPinState = MOVING2TARGET;
} 


/****************************************************************************
 Function
  Move

 Parameters
  direction: 0 for down or 1 for up
  speed: duty cycle (0-255)

 Returns
    None

 Description
  Starts motor to set duty cycle to move pin in given direction.

 Author
     A. Siu, 05/18/17, 5:00
****************************************************************************/
void ShapePin::Move ( int direction, int speed ) {
  if ( pinEnabled ) {
    if (direction == UP) {
      analogWrite( motorApin, 0);
      analogWrite( motorBpin, speed);
    } else { // else direction == DOWN
      analogWrite( motorApin, speed);
      analogWrite( motorBpin, 0);
    }
  }
}

/****************************************************************************
 Function
  CheckIfStalled

 Parameters
  None

 Returns
    True if the pin was stalled, false otherwise

 Description

 Author
     A. Siu, 05/18/17, 5:00
****************************************************************************/
bool ShapePin::CheckIfStalled ( void ) {
    // if motor should be traveling
  if ( isTraveling ) {
      if ( ((millis()-travelStartTime) > STALL_TIME) 
        && ( (abs(GetPosPulses()-travelStartPosition) < 3*MM_TO_PULSE) && ( abs(pid_output) > 0) ) ) {
        // if we've been traveling for some seconds and (we haven't moved and we've been trying to) 
        // turn off motor until system reset
        Idle();
        Serial.printf("stalled %d", pinID);
        return true;
      }
  }
  return false;
}

/****************************************************************************
 Function
   CheckIfStalledZeroing

 Parameters
  None

 Returns
    True if the pin was stalled, false otherwise

 Description

 Author
     A. Siu, 05/22/17, 22:13
****************************************************************************/
bool ShapePin::CheckIfStalledZeroing ( void ) {
  // if motor should be traveling
  if ( isTraveling ) {
      if ( (millis() - travelStartTime) > MAX_ZERO_TIME ) {
        // if we've been traveling for some seconds and ( we haven't moved and we've been trying to) 
        // turn off motor until system reset
        Idle();
        return true;
      }
  }
  return false;
}

/****************************************************************************
 Function
   CheckSwitch

 Parameters
  None

 Returns
    None

 Description
   Checks if switch status has changed

 Author
     A. Siu, 05/18/17, 5:00
****************************************************************************/
void ShapePin::CheckSwitch ( void ) {
  if ( switchDown ) {
    Stop(); // stop motors
    SetMaxSpeed(DEFAULT_SPEED);
    SetMinSpeed(DEFAULT_SPEED);
    // set the curr pos to the zero offset 
    encoder->write( ZERO_OFFSET * MM_TO_PULSE );
    // set new target to zero offset and change states
    CommandTargetPos( ZERO_POS ); 
  }
}

/****************************************************************************
 Function
   Stop

 Parameters
  None

 Returns
    None

 Description
  Stops the pin movement

 Author
     A. Siu, 05/18/17, 5:00
****************************************************************************/
void ShapePin::Stop ( void ) {
  Move(DOWN, 0);
  isTraveling = false;
  // change states to not moving
  currentPinState = IDLE;
}


/****************************************************************************
 Function
   debug

 Parameters
  None

 Returns
    None

 Description
  for debugging specific code parts

 Author
     A. Siu
****************************************************************************/
void ShapePin::debug ( void ) {

//    Move(UP, 255);
    
///// test encoder reading
//  Move(UP, 40);
//  Serial.println( encoder->read() * PULSE_TO_MM );

///// test negative values
//  currentPinState = MOVING2TARGET;
//  targetPos = 30.0 * MM_TO_PULSE;
//  encoder->write( ZERO_OFFSET * MM_TO_PULSE );
//  currPos = encoder->read();
//  Serial.printf( "c: %f; t: %f\n", currPos * PULSE_TO_MM, targetPos * PULSE_TO_MM );
  
/// test switch isr
//  static int x = 0;
//  if (switchDown) {
//    if(x%100==0) {
//      Serial.println("down");
//    }
//    x++;
//    Stop();
//  }

 //// test position reading
  static int x = 0;
//  if (debugFlag) {
    if(x%100==0) {
      Serial.printf( "c: %d \n", GetPosMM());
    }
//  }
  x++;  


}

