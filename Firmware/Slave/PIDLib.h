/****************************************************************************

  Header file for PIDLib used by ShapePin

 ****************************************************************************/
 
#ifndef PID_LIB_H
#define PID_LIB_H

#include "Arduino.h"

class PID {

  public:
    PID ( int Kp, int Ki, int Kd ) ;
    int Compute( int myInput, int mySetpoint );
    void SetOutputLimits( int Min, int Max );
    void SetTunings( int Kp, int Ki, int Kd );
    void SetSampleTime( int NewSampleTime );

  private:
    int kp;
    int kd;
    int ki;

    unsigned long lastTime;
    int ITerm, lastInput, lastOutput;

    unsigned long SampleTime;
    int outMin, outMax;

    int clamp( int val, int max, int min );

};
#endif
