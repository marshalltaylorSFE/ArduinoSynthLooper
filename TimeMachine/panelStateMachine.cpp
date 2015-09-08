//**********************************************************************//
//  BEERWARE LICENSE
//
//  This code is free for any use provided that if you meet the author
//  in person, you buy them a beer.
//
//  This license block is BeerWare itself.
//
//  Written by:  Marshall Taylor
//  Created:  May 26, 2015
//
//**********************************************************************//

#include "panelStateMachine.h"

//#include "SoftwareSerial.h"
//extern SoftwareSerial Serial;
//#include "Arduino.h"
#include "Panel.h"

#include "timeKeeper.h"

PanelStateMachine::PanelStateMachine()
{
	//Outputs
	sSDisplay[0] = ' ';
	sSDisplay[1] = ' ';
	sSDisplay[2] = ' ';
	sSDisplay[3] = ' ';
	tapLED = LEDOFF;
	syncLED = LEDOFF;
	recordLED = LEDOFF;
	playLED = LEDOFF;
	overdubLED = LEDOFF;
	
	//Inputs
	tapButton = 0;
	syncButton = 0;
	songUpButton = 0;
	songDownButton = 0;
	trackUpButton = 0;
	trackDownButton = 0;
	playButton = 0;
	stopButton = 0;
	
	//Controls
	resetTapHead = 0;
	markLength = 0;
	screenControlTap = 0;
	
	state = PIdle;
}

void PanelStateMachine::tick()
{
	//***** PROCESS THE LOGIC *****//

    //Now do the states.
    PStates nextState = state;
    switch( state )
    {
    case PIdle:
		//Can't be running, if button pressed move on
		if( (playButton) == 1 )
		{
			recordLED = LEDON;
			playLED = LEDOFF;
			overdubLED = LEDOFF;
			resetTapHead = 1;
			screenControlTap = 1;
			recording = 1;
			nextState = PFirstRecord;

		}
		playButton = 0;
        break;
	case PFirstRecord:
	    //Came from PIdle
		if( playButton == 1 )
		{
			recordLED = LEDOFF;
			playLED = LEDON;
			overdubLED = LEDOFF;
			markLength = 1; //hold the length
			playing = 1;
			recording = 0;
			nextState = PPlay;
		}
		playButton = 0;
        break;
	case PPlay:
		if( playButton == 1 )
		{
			recordLED = LEDOFF;
			playLED = LEDOFF;
			overdubLED = LEDON;
			nextState = POverdub;
		}
		playButton = 0;
		break;
	case POverdub:
		if( playButton == 1 )
		{
			recordLED = LEDOFF;
			playLED = LEDON;
			overdubLED = LEDOFF;
			nextState = PPlay;
		}
		playButton = 0;
		break;
    default:
        nextState = PIdle;
		//Serial.println("!!!DEFAULT STATE HIT!!!");
        break;
    }
	////Master with the E-Stop
	if( stopButton == 1 )
	{
		stopButton = 0;
		recordLED = LEDOFF;
		playLED = LEDOFF;
		overdubLED = LEDOFF;
		screenControlTap = 0;
		nextState = PIdle;
		recording = 0;
		playing = 0;
	}
	else if( stopButton == 2 )
	{
		stopButton = 0;
		recordLED = LEDFLASHINGFAST;
	}
    state = nextState;

}
