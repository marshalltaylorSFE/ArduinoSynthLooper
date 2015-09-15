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

//Includes
#include "TimeCode.h"
#include "Arduino.h"

//**********************************************************************//
void TimeCode::zero( void )
{
  frameNumber = 0;
  milliseconds = 0;
  seconds = 0;
  minutes = 0;
  hours = 0;

}

void TimeCode::addms( uint16_t msVar )
{
  milliseconds += msVar;
  if ( milliseconds >= 1000 )
  {
    milliseconds -= 1000;
    seconds++;
    if ( seconds >= 60 )
    {
      seconds -= 60;
      minutes++;
      if ( minutes >= 60 )
      {
        minutes -= 60;
        if ( hours < 24 )
        {
          hours++;
        }
      }
    }
  }

}

//**********************************************************************//
void BeatCode::zero( void )
{
  pulses = 0;
  beats = 1;
  bars = 1;
  eightBars = 1;

}

void BeatCode::beatClockPulse( void )
{
  pulses++;
  if( pulses >= 24 )
  {
	pulses -= 24;
    beats++;
    if ( beats >= 5 )
    {
      beats -= 4;
      bars++;
      if ( bars >= 9 )
      {
        bars -= 8;
        eightBars++;
      }
    }
  }
}

uint32_t BeatCode::getTotalPulses( void )
{
	uint32_t returnVar = 0;
	returnVar += pulses;
	returnVar += ((beats - 1) * 24);
	returnVar += ((bars - 1) * 96);
	returnVar += ((eightBars - 1) * 768);
	
	return returnVar;
}

uint32_t BeatCode::getQuantizedPulses( uint8_t qTicks )
{
	uint8_t limit = ( qTicks / 2 ); //say, qTicks = 24, 1/4 note. Limit = 12
	uint32_t totalPulses = getTotalPulses();
	int8_t remainder = totalPulses % qTicks; //Range 0-23
	if( remainder < limit ) //Than we should subtract on 0-11 remainders
	{
		remainder = 0 - remainder;
	}
	else //or add on 12-23 remainders, add 1-12
	{
		remainder = qTicks - remainder;
	}
	Serial.println("\n\n");
	Serial.println("Recording ticks");
	Serial.println(qTicks);
	Serial.println(remainder);
	
	return totalPulses + remainder;
}