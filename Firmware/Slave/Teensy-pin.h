/************************************
NAME: Teensy-pin.h
PURPOSE: Mapping for all the motor, encoder, 
     and switch pins for High-res shape
     display.
Date:   Feb 1st, 2017
*************************************/ 

#ifndef _HIGH_RES_TEENSY_PINOUT_H
#define _HIGH_RES_TEENSY_PINOUT_H

bool isTesting = true;
const int OnboardID[4] = {2, 3, 1, 0};

/************************************
Mapping between MK20 chip and Teensy 3.2
*************************************/ 
// Digital Pins
#define MK39 0
#define MK40 1
#define MK57 2
#define MK28 3
#define MK29 4
#define MK64 5
#define MK61 6
#define MK59 7
#define MK60 8
#define MK46 9
#define MK49 10
#define MK51 11
#define MK52 12
#define MK50 13
#define MK58 14
#define MK43 15
#define MK35 16
#define MK36 17
#define MK38 18
#define MK37 19
#define MK62 20
#define MK63 21
#define MK44 22
#define MK45 23
#define MK27 24
#define MK42 25
#define MK2  26
#define MK54 27
#define MK53 28
#define MK55 29
#define MK56 30
#define MK1  31
#define MK41 32
#define MK26 33
// Analog pins
#define MK9  34
#define MK10 35
#define MK11 A12
#define MK12 A13
#define MK18 A14
#define MK14 AREF


/********** PCB LAYOUT ***********
- Top View in Circuit Maker (CM)
- Viewed from below actual vertical PCB

MCU ID   |<---2--->| |<---3--->|  
MOTOR#   0 1 2 3 4 5 0 1 2 3 4 5   BOTTOM (CM)
(PWR SIDE)======================== PCB
MOTOR#   5 4 3 2 1 0 5 4 3 2 1 0   TOP (CM)
MCU ID   |<---0--->| |<---1--->|
*************************************/ 

/***************** DIAGRAM OF ACTUAL PCB PAIR *********************
          LEFT PCB            RIGHT PCB
         ___|_|_|_|_|_|__|_|_|_|_|_|_  _|_|_|_|_|_|__|_|_|_|_|_|__
        | | | | | | |  | | | | | | || | | | | | |  | | | | | |  |
       ...  | | | | | |  | | | | | | || | | | | | |  | | | | | | ...
        | | | | | | |  | | | | | | || | | | | | |  | | | | | |  |
  MOTOR#|   0 1 2 3 4 5  0 1 2 3 4 5 || 0 1 2 3 4 5  0 1 2 3 4 5  |
   PWR==|  MCU:  2            3      ||      1            0       |==PWR
        |____________________________||___________________________|

*******************************************************************/

// TODO: MIRROR MAPPING FOR 0 AND 1
const int Mapping_motor[4][12] = {
  // MCU 0
  {MK63, MK64, MK61, MK62,   MK46, MK49, MK44, MK45,   MK41, MK42, MK28, MK29},  //OLD: {MK28, MK29, MK41, MK42,   MK44, MK45, MK46, MK49,   MK61, MK62, MK63, MK64},
  // MCU 1
  {MK28, MK29, MK63, MK64,   MK61, MK62, MK46, MK49,   MK44, MK45, MK41, MK42},  //OLD: {MK41, MK42, MK44, MK45,   MK46, MK49, MK61, MK62,   MK63, MK64, MK28, MK29},
  // MCU 2
  {MK63, MK64, MK61, MK62,   MK46, MK49, MK44, MK45,   MK41, MK42, MK28, MK29}, //{MK64, MK63, MK62, MK61,   MK49, MK46, MK45, MK44,   MK42, MK41, MK29, MK28},
  // MCU 3
  {MK63, MK64, MK61, MK62,   MK46, MK49, MK44, MK45,   MK41, MK42, MK28, MK29}  //{MK64, MK63, MK62, MK61,   MK49, MK46, MK45, MK44,   MK42, MK41, MK29, MK28}
};

const int Mapping_encoder[4][12] = {
  // MCU 0
  {MK2,  MK1,  MK60, MK59,   MK57, MK54, MK52, MK50,   MK37, MK36, MK35, MK27},  //OLD: {MK35, MK27, MK37, MK36,   MK52, MK50, MK57, MK54,   MK60, MK59, MK2,  MK1 },
  // MCU 1
  {MK2,  MK1,  MK59, MK58,   MK56, MK55, MK53, MK52,   MK50, MK43, MK37, MK36},  //OLD: {MK37, MK36, MK50, MK43,   MK53, MK52, MK56, MK55,   MK59, MK58, MK2,  MK1 },
  // MCU 2
  {MK59, MK58, MK56, MK55,   MK53, MK52, MK50, MK43,   MK37, MK36, MK35, MK27},
  // MCU 3
  {MK60, MK59, MK57, MK56,   MK54, MK53, MK51, MK50,   MK38, MK37, MK35, MK27}
};

const int Mapping_switch[4][6] = {
  // MCU 0                   // 10, 11, 12 are analog pins
  {MK10, MK58, MK53, MK43, MK12, MK11},  //OLD: {MK11, MK12, MK43, MK53, MK58, MK10},
  // MCU 1
  {MK60, MK57, MK54, MK51, MK38, MK27},  //OLD: {MK27, MK38, MK51, MK54, MK57, MK60},
  // MCU 2
  {MK60, MK57, MK54, MK51, MK38, MK1 },
  // MCU 3
  {MK1,  MK58, MK55, MK52, MK43, MK36}
};


/***************************************
  Actual Mapping:
  ID      Position (From top view in CM)
  ----------------
  0       Top layer left
  1       Top layer right
  2       Bottom layer left
  3       Bottom layer right

  LOOK AT THIS WHEN ASSIGNING MCU IDs
****************************************/

struct Mapping {
  const int* pin_motor;
  const int* pin_encoder;
  const int* pin_switch;
  Mapping(int ID) {
    int mcuID;
    mcuID = OnboardID[ID % 4];
    pin_motor = Mapping_motor[mcuID % 4];
    pin_encoder = Mapping_encoder[mcuID % 4];
    pin_switch = Mapping_switch[mcuID % 4];
  }
};

// Format:
// Mapping mapping(ID);    SETUP (call once)
// mapping.pin_motor[0]    Returns pin for motor0

#endif
