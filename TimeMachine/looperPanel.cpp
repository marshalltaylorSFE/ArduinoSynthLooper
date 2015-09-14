//**LooperPanel*******************************//
#include "looperPanel.h"
//#include "PanelComponents.h"
#include "Panel.h"
#include "Arduino.h"

LooperPanel::LooperPanel( void )
{
	//Controls
	resetTapHeadFlag = 0;
	markLengthFlag = 0;
	screenControlTap = 0;
	
	//trackNum = 1;
	viewingTrack = 1;
	recordingTrack = 0;
	recording = 0;
	
	songClearRequestFlag = 0;
	
	for( int i = 0; i < 16; i++ )
	{
		trackMute[i] = 0; //Not muted
	}

	tapLedFlag = 0;
	syncLedFlag = 0;
	txLedFlag = 0;
	rxLedFlag = 0;	
	BPM = 80;
	BPMUpdateRequestFlag = 0;
	
	state = PInit;
}

void LooperPanel::setTapLed( void )
{
	tapLedFlag = 1;
}

ledState_t LooperPanel::serviceTapLed( void )
{
	ledState_t returnVar = LEDOFF;
	if( tapLedFlag == 1 )
	{
		returnVar = LEDON;
		tapLedFlag = 0;
		
	}
	return returnVar;
	
}

void LooperPanel::setSyncLed( void )
{
	syncLedFlag = 1;
}

ledState_t LooperPanel::serviceSyncLed( void )
{
	ledState_t returnVar = LEDOFF;
	if( syncLedFlag == 1 )
	{
		returnVar = LEDON;
		syncLedFlag = 0;
		
	}
	return returnVar;
	
}

void LooperPanel::setTxLed( void )
{
	txLedFlag = 1;
}

ledState_t LooperPanel::serviceTxLed( void )
{
	ledState_t returnVar = LEDOFF;
	if( txLedFlag == 1 )
	{
		returnVar = LEDON;
		txLedFlag = 0;
		
	}
	return returnVar;
	
}

void LooperPanel::setRxLed( void )
{
	rxLedFlag = 1;
}

ledState_t LooperPanel::serviceRxLed( void )
{
	ledState_t returnVar = LEDOFF;
	if( rxLedFlag == 1 )
	{
		returnVar = LEDON;
		rxLedFlag = 0;
		
	}
	return returnVar;
	
}

void LooperPanel::reset( void )
{
	//Set explicit states
	//Set all LED off
	recordLed.setState(LEDOFF);
	playLed.setState(LEDOFF);
	overdubLed.setState(LEDOFF);
	tapLed.setState(LEDOFF);
	syncLed.setState(LEDOFF);
	txLed.setState(LEDOFF);
	rxLed.setState(LEDOFF);
	leftDisplay.setState(SSOFF);
	rightDisplay.setState(SSOFF);
	
	update();
	
}

void LooperPanel::processMachine( void )
{
	//Do external updates
	tapLed.setState(serviceTapLed());
	syncLed.setState(serviceTapLed());
	txLed.setState(serviceTxLed());
	rxLed.setState(serviceRxLed());
	
	//Do main machine
	tickStateMachine();
	
	//Do small machines
	if( option1Button.serviceRisingEdge() )
	{
		if( viewingTrack != ( recordingTrack + 1 ) )
		{
			//not on current track, good
			//toggle recordingtrack
			trackMute[ viewingTrack - 1 ] ^= 1;
		}
	}
	if( songUpButton.serviceRisingEdge() )
	{
		BPM++;
	}
	if( songDownButton.serviceRisingEdge() )
	{
		BPM--;
	}
	if( trackUpButton.serviceRisingEdge() )
	{
		if( viewingTrack < ( recordingTrack + 1 ) )
		{
			viewingTrack++;
			if(viewingTrack == ( recordingTrack + 1 ) )
			{
				rightDisplay.setState( SSON );
			}
			sprintf(tempString, "%4d", (unsigned int)viewingTrack);
			rightDisplay.setData(tempString);
		}
	}
	if( trackDownButton.serviceRisingEdge() )
	{
		if( viewingTrack > 1 )
		{
			viewingTrack--;
			rightDisplay.setState( SSFLASHING );
			sprintf(tempString, "%4d", (unsigned int)viewingTrack);
			rightDisplay.setData(tempString);
		}
	}

	if( viewingTrack != ( recordingTrack + 1 ) )
	{
		//Ok to give extra options
		if( trackMute[ viewingTrack - 1 ] == 1 )
		{
			option1Led.setState(LEDON);
		}
		else
		{
			option1Led.setState(LEDOFF);
		}
	}
	if( viewingTrack == ( recordingTrack + 1 ) )
	{
		option1Led.setState(LEDOFF);
	}
	
	if(( recording ) || ( playing ) )
	{
		leftDisplay.setData(tapHeadMessage);
	}
	
	update();
}

void LooperPanel::tickStateMachine()
{
	//***** PROCESS THE LOGIC *****//
	
    //Now do the states.
    PStates nextState = state;
    switch( state )
    {
    case PInit:
		Serial.println("Init state!!!!");
		nextState = PIdle;
		leftDisplay.setState(SSON);
		sprintf(tempString, "%4d", (unsigned int)viewingTrack);
		rightDisplay.setData(tempString);
		rightDisplay.setState(SSON);
		
		break;
	case PIdle:
		sprintf(tempString, "%4d", (unsigned int)( BPM ));
		leftDisplay.setData(tempString);
		//Can't be running, if button pressed move on
		if( playButton.serviceRisingEdge() )
		{
			recordLed.setState(LEDON);
			playLed.setState(LEDOFF);
			overdubLed.setState(LEDOFF);
			resetTapHeadFlag = 1;
			screenControlTap = 1;
			recording = 1;
			nextState = PFirstRecord;
		}
        break;
	case PFirstRecord:
	    //Came from PIdle
		if( playButton.serviceRisingEdge() )
		{
			recordLed.setState(LEDOFF);
			playLed.setState(LEDON);
			overdubLed.setState(LEDOFF);
			markLengthFlag = 1; //hold the length
			playing = 1;
			recording = 0;
			nextState = PPlay;

			recordingTrack++;
			
			rightDisplay.setState( SSON );
			viewingTrack = recordingTrack + 1;
			sprintf(tempString, "%4d", (unsigned int)viewingTrack);
			rightDisplay.setData(tempString);

		}
        break;
	case PPlay:
		if( playButton.serviceRisingEdge() )
		{
			recordLed.setState(LEDOFF);
			playLed.setState(LEDOFF);
			overdubLed.setState(LEDON);
			nextState = POverdub;
			recording = 1;

			
		}
		break;
	case POverdub:
		if( playButton.serviceRisingEdge() )
		{
			recordLed.setState(LEDOFF);
			playLed.setState(LEDON);
			overdubLed.setState(LEDOFF);
			nextState = PPlay;
			recording = 0;
			
			recordingTrack++;
			
			rightDisplay.setState( SSON );
			viewingTrack = recordingTrack + 1;
			sprintf(tempString, "%4d", (unsigned int)viewingTrack);
			rightDisplay.setData(tempString);

		}
		break;
    default:
        nextState = PInit;
		//Serial.println("!!!DEFAULT STATE HIT!!!");
        break;
    }
	////Master with the E-Stop
	if( stopButton.serviceRisingEdge() )
	{
		recordLed.setState(LEDOFF);
		playLed.setState(LEDOFF);
		overdubLed.setState(LEDOFF);
		nextState = PIdle;
		recording = 0;
		playing = 0;
		
	}
	else if( stopButton.serviceHoldRisingEdge() )
	{
		recordLed.setState(LEDFLASHINGFAST);
		trackNum = 1;
		recordingTrack = 0;
		songClearRequestFlag = 1;
		viewingTrack = 1;
		sprintf(tempString, "%4d", (unsigned int)viewingTrack);
		rightDisplay.setData(tempString);
	}

    state = nextState;

}

uint8_t LooperPanel::serviceResetTapHead( void )
{
	uint8_t returnVar = 0;
	if( resetTapHeadFlag == 1 )
	{
		resetTapHeadFlag = 0;
		returnVar = 1;
	}
	
	return returnVar;
}

uint8_t LooperPanel::serviceMarkLength( void )
{
	uint8_t returnVar = 0;
	if( markLengthFlag == 1 )
	{
		markLengthFlag = 0;
		returnVar = 1;
	}
	
	return returnVar;
}

uint8_t LooperPanel::serviceSongClearRequest( void )
{
	uint8_t returnVar = 0;
	if( songClearRequestFlag == 1 )
	{
		songClearRequestFlag = 0;
		returnVar = 1;
	}
	
	return returnVar;
}

uint8_t LooperPanel::serviceBPMUpdateRequest( void )
{
	uint8_t returnVar = 0;
	if( BPMUpdateRequestFlag == 1 )
	{
		BPMUpdateRequestFlag = 0;
		returnVar = 1;
	}
	
	return returnVar;
}

void LooperPanel::setTapHeadMessage( BeatCode & inputHead )
{
	sprintf(tempString, "%4d", (unsigned int)( inputHead.beats ));
	tapHeadMessage[3] = tempString[3];
	sprintf(tempString, "%4d", (unsigned int)( inputHead.bars ));
	tapHeadMessage[2] = tempString[3];
	sprintf(tempString, "%4d", (unsigned int)( inputHead.eightBars ));
	tapHeadMessage[1] = tempString[3];
	tapHeadMessage[0] = tempString[2];
	
	tapHeadMessage[4] = '\0';
	Serial.println(tapHeadMessage);
	Serial.println(inputHead.beats);
	
}

//This is 0 = first track referenced
void LooperPanel::getTrackMute( uint8_t * workingArray )
{
	for( int i = 0; i < 16; i++ )
	{
		workingArray[i] = trackMute[i];
	}
}

uint8_t LooperPanel::getRecordingTrack( void )
{
	return recordingTrack;
}
