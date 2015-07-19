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
  framePiece = 0;
  frames = 0;
  beats = 0;
  bars = 0;
  eightBars = 0;
  ticks = 0;

}

void BeatCode::addFramePiece( void )
{
  framePiece++;
  ticks = ticks + 3;
  if( framePiece == 8 )
  {
    framePiece = 0;
    frames++;
    if( frames >= 8 )
    {
      frames -= 8;
      beats++;
      if ( beats >= 4 )
      {
        beats -= 4;
        bars++;
        if ( bars >= 8 )
        {
          bars -= 8;
          eightBars++;
        }
      }
    }
  }
}
