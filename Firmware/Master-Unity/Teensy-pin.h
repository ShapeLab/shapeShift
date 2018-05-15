/************************************
NAME: Teensy-pin.h
PURPOSE: Mapping for all the motor, encoder, 
		 and switch pins for High-res shape
		 display.
Date:   Feb 1st, 2017
*************************************/ 


#ifndef _HIGH_RES_TEENSY_PINOUT_H
#define _HIGH_RES_TEENSY_PINOUT_H

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


/********** PCB Top View ***********

BOTTOM	0 1 2 3 4 5 6 0 1 2 3 4 5 6
PCB     ============================
TOP     0 1 2 3 4 5 6 0 1 2 3 4 5 6 

*************************************/ 

const int Mapping_motor[4][12] = {
	//TOP LEFT
	{MK28, MK29, MK41, MK42,   MK44, MK45, MK46, MK49,   MK61, MK62, MK63, MK64},
	//TOP RIGHT
	{MK41, MK42, MK44, MK45,   MK46, MK49, MK61, MK62,   MK63, MK64, MK28, MK29},
	// BOTTOM LEFT
	{MK64, MK63, MK62, MK61,   MK49, MK46, MK45, MK44,   MK42, MK41, MK29, MK28},
	// BOTTOM RIGHT
	{MK64, MK63, MK62, MK61,   MK49, MK46, MK45, MK44,   MK42, MK41, MK29, MK28}
};


// CLOSER TO MCU AND FURTHER AWAY
const int Mapping_encoder[4][12] = {
	//TOP LEFT
	{MK9, MK10, MK37, MK36,   MK52, MK50, MK57, MK54,   MK60, MK59, MK2,  MK1 },	// Reassign the first 2 to analog
	//TOP RIGHT
	{MK37, MK36, MK50, MK43,   MK53, MK52, MK56, MK55,   MK59, MK58, MK2,  MK1 },
	// BOTTOM LEFT
	{MK59, MK58, MK56, MK55,   MK53, MK52, MK50, MK43,   MK37, MK36, MK35, MK27},
	// BOTTOM RIGHT
	{MK60, MK59, MK57, MK56,   MK54, MK53, MK51, MK50,   MK38, MK37, MK35, MK27}
};

const int Mapping_switch[4][6] = {
	//TOP LEFT
	{MK27, MK35, MK43, MK53, MK58, MK11},        // Reassign the last one to analog
	//TOP RIGHT
	{MK35, MK38, MK51, MK54, MK57, MK60},
	// BOTTOM LEFT
	{MK60, MK57, MK54, MK51, MK38, MK9 },       // Reassign the last one to analog
	// BOTTOM RIGHT
	{MK1,  MK58, MK55, MK52, MK43, MK36}
};

/***************************************
	Actual Mapping:
	ID      Position (From top view)
	----------------
	0       Top layer left
	1       Top layer right
	2       Bottom layer left
	3       Bottom layer right

	LOOK AT THIS WHEN ASSIGNING MCU IDs
****************************************/

struct Mapping
{
	const int* pin_motor;
	const int* pin_encoder;
	const int* pin_switch;
	Mapping(int ID) {
		pin_motor = Mapping_motor[ID % 4];
		pin_encoder = Mapping_encoder[ID % 4];
		pin_switch = Mapping_switch[ID % 4];
	}
};

// Mapping mapping(ID);
// mapping.pin_motor[0]

#endif
