/****************************************************************************

  Header file for ShapePin

 ****************************************************************************/

#ifndef SHAPE_PIN_H
#define SHAPE_PIN_H

/*----------------------------- Include Files -----------------------------*/
#include "ShapeConstants.h"
#include <Encoder.h>
#include "PIDLib.h"

typedef enum { IDLE, WAITING4SWITCH, MOVING2TARGET, 
               UP_STATE, DOWN_STATE, DEBUG } PinState_t;

// library interface description
class ShapePin
{

  public:
    /*----------------------------- Module Defines ----------------------------*/
    #define ENCODER_USE_INTERRUPTS
    #define ENCODER_OPTIMIZE_INTERRUPTS

    /*---------------------------- Module Functions ---------------------------*/
    ShapePin( int id, bool enabled,    // * constructor. sets pinout & initial params
              int motorA, int motorB,
              int encoderA, int encoderB );

    int pinID;  // pin ID number can be from 0 to 5

    // Commands
    void RunSM(void);			        // * run pin state machine. Should be called
    //   every loop() cycle
    void Zero( void );					        // * zero the pin to recalibrate
    void CommandTargetPos(int newPos);  // * set a new target pos for the pin
                                        //   in units of mm
    void Idle ( void );					        // * set the pin in idle state
    void DisableShapePin( void );       // * disable the pin so it is no longer used
    void SwitchISR ( bool );            // * should be called by the switch ISR with the updated
                                        //   switch state
    
    // Setters & Getters
    void SetState ( PinState_t newState );
    void SetSwitchDown( bool );         
    void SetKp ( int newkp );
    void SetKi ( int newi );
    void SetKd ( int newkd );
    void SetMaxSpeed ( int speed ); // duty cycle (0-255)
    void SetMinSpeed ( int speed ); // duty cycle (0-255)
    void SetDeadzone ( int mm );    // in mm 
    void SetDeadzone_Pulses ( int pulses );  // in pulses (aka ticks*4)
    void SetMaxTravel ( int mm );   // in mm (0-60)

    // Display Functions
    int GetPosMM( void );
    int GetPosPulses( void );
    bool GetSwitchDown( void );
    PinState_t GetState( void );
    String PrintState( void );

    bool debugFlag = false;

  private:
    /*---------------------------- Module Functions ---------------------------*/
    void RunControlLoop ( void );
    void RunControlLoopWithoutMoving ( void );
    void Move ( int direction, int speed );
    bool CheckIfStalled ( void );
    bool CheckIfStalledZeroing ( void );
    void CheckSwitch ( void );
    void Stop ( void );
    void debug ( void );
    
    /*----------------------------- Module Variables --------------------------*/
    bool pinEnabled = true;
    
    PID* motorPID;
    Encoder* encoder;

    /* Pin Assignment */
    int motorApin, motorBpin;
    int encoderApin, encoderBpin;

    /* Control variables */
    bool isTraveling;
    int targetPos;   //[pulses]
    double deadzone; //[pulses]
    int pid_output;          
    int Kp, Kd, Ki;         
    int maxTravel;   //[mm] max travel distance e.g. 60 mm

    /* Stall check variables */
    unsigned long travelStartTime; // [ms]
    int lastTargetPos;             // [pulses]
    int travelStartPosition;       // [pulses]

    /* Switch */
    int loweringSpeed = 200;     
    int maxSpeed;
    int minSpeed;     
    bool switchDown = false;
    
    /* State */
    PinState_t currentPinState;

};

#endif
