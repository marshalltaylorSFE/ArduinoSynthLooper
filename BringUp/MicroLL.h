//**********************************************************************//
//  BEERWARE LICENSE
//
//  This code is free for any use provided that if you meet the author
//  in person, you buy them a beer.
//
//  This license block is BeerWare itself.
//
//  Written by:  Marshall Taylor
//  Created:  March 21, 2015
//
//**********************************************************************//
#ifndef MICROLL_H_INCLUDED
#define MICROLL_H_INCLUDED

#include "stdint.h"

//Depth into the stack variable
typedef uint8_t listIdemNumber_t;

//Note data type
struct MidiEvent
{
  uint32_t timeStamp;
  uint8_t eventType;
  //uint8_t channel;
  uint8_t value;
  uint16_t data;
  

  //A pointer to the newer object
  MidiEvent * nextObject;

};

//Point ListObject_t to custom object type
typedef MidiEvent listObject_t;

#define NULLNOTE -1

class MicroLL
{
    listIdemNumber_t maxLength;
    listIdemNumber_t currentPosition;

  public:
    //A pointer to the next object, list items enumerate from this
    listObject_t * startObjectPtr;
	
	//A empty note to point to when the list is empty
    listObject_t nullObject;

  public:
    MicroLL( listIdemNumber_t ); //Construct with passed max depth
    MicroLL( void );
    void pushObject( listObject_t & ); //Pass listObject_t
    //listObject_t popObject( void ); //returns listObject_t
    listObject_t readObject( void );
    listObject_t * readObject( listIdemNumber_t );
	listObject_t * seekNextAfter( uint32_t );//Search by tick value
    void dropObject( listIdemNumber_t ); //Pass position, returns listObject_t
	int8_t seekObjectbyTimeStamp( listObject_t & ); //pass listObject_t, returns position
	int8_t seekObjectbyNoteValue( listObject_t & ); //pass listObject_t, returns position
    listIdemNumber_t listLength( void ); //returns depth of stack.
    void printfMicroLL( void );


};

#endif // FILE_H_INCLUDED

