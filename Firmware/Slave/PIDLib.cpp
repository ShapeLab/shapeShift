/****************************************************************************
 Module
   PIDLib.cpp

 Revision
   1.0.0

 Description
   Library written for PID control of ShapePin

 Notes

 Author
    A. Siu <afsiu@stanford.edu>
   
 History
 When           Who     What/Why
 -------------- ---     --------
 05/20/17 10:30 afs     tested with hardware
****************************************************************************/

#include "PIDLib.h"

/****************************************************************************

  Public Functions

****************************************************************************/

PID::PID ( int Kp, int Ki, int Kd ) {

  SetOutputLimits(0, 255);

  SampleTime = 25;

  SetTunings(Kp, Ki, Kd);
  
  lastOutput = 0;
  
  lastTime = millis() - SampleTime;
  
}

int PID::Compute( int myInput, int mySetpoint ) {
  unsigned long now = millis();
  unsigned long timeChange = (now - lastTime);
  
  if (timeChange >= SampleTime)
  {
    /*Compute all the working error variables*/
    int input = myInput;
    int error = mySetpoint - input;
    // error windup
    ITerm += (ki * error);
    ITerm = clamp(ITerm, outMax, outMin);
    int dInput = (input - lastInput);

    /*Compute PID Output*/
    int out = kp * error + ITerm - kd * dInput;
    out = clamp(out, outMax, outMin);

    /*Remember some variables for next time*/
    lastOutput = out;
    lastInput = input;
    lastTime = now;
    return out;
  }
  return lastOutput;
}

void PID::SetOutputLimits(int Min, int Max) {
  outMin = Min;
  outMax = Max;
}

void PID::SetTunings(int Kp, int Ki, int Kd) {
  if (Kp < 0 || Ki < 0 || Kd < 0) {
    return;
  }
  kp = Kp;
  ki = Ki;
  kd = Kd;
//    double SampleTimeInSec = ((double)SampleTime)/1000;
//    kp = Kp;
//    ki = Ki * SampleTimeInSec;
//    kd = Kd / SampleTimeInSec;
}

void PID::SetSampleTime(int NewSampleTime) {
  if ( NewSampleTime > 0) {
//        double ratio  = (double) NewSampleTime / (double)SampleTime;
//        ki *= ratio;
//        kd /= ratio;
    SampleTime = (unsigned long)NewSampleTime;
  }
}

/****************************************************************************

  Private Functions

****************************************************************************/

int PID::clamp( int val, int max, int min ) {
  if (val > max) {
    val = max;
  }
  else if (val < min) {
    val = min;
  }
  return val;
}

