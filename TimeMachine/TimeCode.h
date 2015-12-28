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

#ifndef TIMECODE_H_INCLUDED
#define TIMECODE_H_INCLUDED

#include "stdint.h"

//Classes
//**********************************************************************

class TimeCode
{
  public:
    void zero( void );
    void addms( uint16_t );
    uint8_t frameNumber;
    uint16_t milliseconds;
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;

};

class BeatCode
{
  public:
    void zero( void );
    void beatClockPulse( void );
	uint32_t getTotalPulses( void );
    uint32_t pulses;
    uint8_t beats;
    uint8_t bars;
    uint8_t eightBars;
	uint32_t getQuantizedPulses( uint8_t ); //pass ticks
	uint32_t convertLengthInput( int32_t, uint8_t );
};

#endif // TIMECODE_H_INCLUDED

