//**LooperPanel*******************************//
#include "looperPanel.h"
//#include "PanelComponents.h"
#include "Panel.h"
#include "Arduino.h"
#include "flagMessaging.h"

LooperPanel::LooperPanel( void )
{
	//Controls
	//trackNum = 1;
	viewingTrack = 1;
	recordingTrack = 0;
	
	for( int i = 0; i < 16; i++ )
	{
		trackMute[i] = 0; //Not muted
	}

	tapLedFlag = 0;
	syncLedFlag = 0;
	txLedFlag = 0;
	rxLedFlag = 0;	
	BPM = 80;
	
	state = PInit;
	
	quantizeTrackTicks = 24;
	quantizeTicks = 24;
	
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

//---------------------------------------------------------------------------//
//
//  To process the machine,
//    take the inputs from the system
//    process human interaction hard-codes
//    process the state machine
//    clean up and post output data
//
//---------------------------------------------------------------------------//
void LooperPanel::processMachine( void )
{
	//Do small machines
	if( trackDownButton.serviceRisingEdge() )
	{
		if( viewingTrack > 1 )
		{
			viewingTrack--;
		}
	}
	if( trackUpButton.serviceRisingEdge() )
	{
		if( viewingTrack < ( recordingTrack + 1 ) )
		{
			viewingTrack++;
		}
	}
	if( option1Button.serviceRisingEdge() )
	{
		if( viewingTrack != ( recordingTrack + 1 ) )
		{
			//not on current track, good
			//toggle recordingtrack
			trackMute[ viewingTrack - 1 ] ^= 1;
		}
	}
	if( option2Button.serviceRisingEdge() )
	{
		quantizingTrackFlag.setFlag();
		option2Led.setState( LEDFLASHINGFAST );
		quantizingTrackTimeKeeper.mClear();
	}

	if( songUpButton.serviceRisingEdge() )
	{
		BPM++;
		updateBPMFlag.setFlag();
	}
	
	if( songDownButton.serviceRisingEdge() )
	{
		BPM--;
		updateBPMFlag.setFlag();
	}

	if( quantizeSelector.serviceChanged() )
	{
		uint8_t displayDivisor = 0;
		uint8_t tickDivisor = 0;
		quantizeMessage[0] = ' ';
		quantizeMessage[1] = ' ';
		quantizeMessage[2] = ' ';
		quantizeMessage[3] = ' ';
		
		//Calculate data
		if( quantizeSelector.getState() < 4 )
		{
			//We are in 1/4 domain
			displayDivisor = 4;
			for( int i = 0; i < ( quantizeSelector.getState() ); i++ )
			{
				displayDivisor = displayDivisor * 2;
			}
			tickDivisor = 24 / ( displayDivisor / 4 );
			//Build the display string
			sprintf(quantizeMessage, "%4d", (unsigned int)displayDivisor);
			quantizeMessage[0] = '1';
			quantizeMessage[1] = ' ';
		}
		if( quantizeSelector.getState() > 5 )
		{
			//We are in 1/3 domain
			displayDivisor = 3;
			for( int i = 0; i < ( 9 - quantizeSelector.getState() ); i++ )
			{
				displayDivisor = displayDivisor * 2;
			}
			//tickDivisor = 24 / ( displayDivisor / 3 );
			//Build the display string
			sprintf(quantizeMessage, "%4d", (unsigned int)displayDivisor);
			quantizeMessage[0] = '1';
			quantizeMessage[1] = ' ';
		}
		quantizeMessage[4] = '\0';		
	    
		if( quantizingTrackFlag.getFlag() )
		{
			quantizingTrackTimeKeeper.mClear();
			quantizeTrackTicks = tickDivisor;
		}
		else
		{
			if( quantizeHoldOffFlag.getFlag() == 1 )
			{
				rightDisplay.peekThrough( quantizeMessage, 1500 ); // 'data' type, time in ms to persist
			}
			else
			{
				//Throw away the first reading
				quantizeHoldOffFlag.setFlag();
			}
			quantizeTicks = tickDivisor;
		}
		

	}
	if(( quantizingTrackTimeKeeper.mGet() > 2000 )&&( quantizingTrackFlag.getFlag() == 1 ))
	{
		quantizingTrackFlag.clearFlag();
	}

	//Do main machine
	tickStateMachine();
	
	//Do pure LED operations first
	//System level LEDs
	tapLed.setState(serviceTapLed());
	syncLed.setState(serviceTapLed());
	txLed.setState(serviceTxLed());
	rxLed.setState(serviceRxLed());
	
	//Panel level LEDs
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
	
	//-- Select the correct 7 segment sources here --//

	//Default modes
	leftDisplayMode = 0;
	rightDisplayMode = 0;

	if( ( recordingFlag.getFlag() )||( playingFlag.getFlag() ) )
	{
		leftDisplayMode = 2;
	}
	else
	{
		leftDisplayMode = 1;
	}
	if(viewingTrack == ( recordingTrack + 1 ) )
	{
		rightDisplayMode = 0;
	}
	else
	{
		rightDisplayMode = 1;
	}
	
	if( quantizingTrackFlag.getFlag() == 1 )
	{
		rightDisplayMode = 2;
		
	}
	else
	{
		option2Led.setState( LEDOFF );
		quantizingTrackFlag.clearFlag();
	}
	//Make displays
	leftDisplay.setState( SSON );
	switch( leftDisplayMode )
	{
		case 0:  //Channel is song number
			leftDisplay.setData("8008");
		break;
		case 1:  //Channel is BPM
			sprintf(tempString, "%4d", (unsigned int)BPM);
			leftDisplay.setData(tempString);
		break;
		case 2:  //Channel is playhead
			leftDisplay.setData(tapHeadMessage);
		break;
		default:
			leftDisplay.setState( SSOFF );
		break;
	}
	

	switch( rightDisplayMode )
	{
		case 0:  //display next track to record
			rightDisplay.setState( SSON );
			sprintf(tempString, "%4d", (unsigned int)viewingTrack);
			rightDisplay.setData(tempString);
		break;
		case 1:  //select other track
			rightDisplay.setState( SSFLASHING );
			sprintf(tempString, "%4d", (unsigned int)viewingTrack);
			rightDisplay.setData(tempString);
		break;
		case 2:  //display selecting track quantize
			rightDisplay.setState( SSON );
			rightDisplay.setData(quantizeMessage);
		default:
		break;
	}

	update();
}

void LooperPanel::tickStateMachine()
{
	//***** PROCESS THE LOGIC *****//
	uint8_t incrementTrackTemp = 0;
    //Now do the states.
    PStates nextState = state;
    switch( state )
    {
    case PInit:
		Serial.println("Init state!!!!");
		nextState = PIdle;
		
		break;
	case PIdle:
		//Can't be running, if button pressed move on
		if( playButton.serviceRisingEdge() )
		{
			resetTapHeadFlag.setFlag();
			Serial.print("SHDF: ");
			Serial.println( songHasDataFlag.getFlag() );
			if( songHasDataFlag.getFlag() == 0 )
			{
				recordLed.setState(LEDON);
				playLed.setState(LEDOFF);
				overdubLed.setState(LEDOFF);
				recordingFlag.setFlag();
				nextState = PFirstRecord;
			}
			else
			{
				recordLed.setState(LEDOFF);
				playLed.setState(LEDON);
				overdubLed.setState(LEDOFF);
				playingFlag.setFlag();
				recordingFlag.clearFlag();
				nextState = PPlay;
			}
		}
        break;
	case PFirstRecord:
	    //Came from PIdle
		if( ( playButton.serviceRisingEdge() ) )
		{
			recordLed.setState(LEDOFF);
			playLed.setState(LEDON);
			overdubLed.setState(LEDOFF);
			markLengthFlag.setFlag(); //hold the length
			playingFlag.setFlag();
			recordingFlag.clearFlag();
			nextState = PPlay;
			songHasDataFlag.setFlag();
			incrementTrackTemp = 1;

		}
        break;
	case PPlay:
		if( playButton.serviceRisingEdge() )
		{
			recordLed.setState(LEDOFF);
			playLed.setState(LEDOFF);
			overdubLed.setState(LEDON);
			nextState = POverdub;
			recordingFlag.setFlag();
			
		}
		if( playButton.serviceHoldRisingEdge() )
		{
			nextState = PUndoDecrement;
			

		}
		break;
	case POverdub:
		if( playButton.serviceRisingEdge() )
		{
			recordLed.setState(LEDOFF);
			playLed.setState(LEDON);
			overdubLed.setState(LEDOFF);
			nextState = PPlay;
			recordingFlag.clearFlag();

			incrementTrackTemp = 1;
			
		}
		if( playButton.serviceHoldRisingEdge() )
		{
			nextState = PUndoClearOverdub;
			
			recordingFlag.clearFlag();
			recordLed.setState(LEDOFF);
			playLed.setState(LEDON);
			overdubLed.setState(LEDOFF);
		}
		break;
	case PUndoClearOverdub:
		if( clearTrackFlag.serviceFallingEdge() )
		{
			nextState = PUndoDecrement;
		}
		else
		{
			trackToClear = recordingTrack;
			clearTrackFlag.setFlag();
		}
		break;
	case PUndoDecrement:
		//Decrement count
		if( recordingTrack > 0 )
		{
			if( viewingTrack == recordingTrack + 1 )
			{
				viewingTrack = recordingTrack;
			}
			recordingTrack--;
		}
		nextState = PUndoClearTrack;
		break;
	case PUndoClearTrack:
		if( clearTrackFlag.serviceFallingEdge() )
		{
			nextState = PPlay;
		}
		else
		{
			trackToClear = recordingTrack;
			clearTrackFlag.setFlag();
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
		recordingFlag.clearFlag();
		playingFlag.clearFlag();
	}
	else if( stopButton.serviceHoldRisingEdge() )
	{
		recordLed.setState(LEDFLASHINGFAST);
		trackNum = 1;
		recordingTrack = 0;
		clearSongFlag.setFlag();
		viewingTrack = 1;
		songHasDataFlag.clearFlag();
		nextState = PIdle;
	}

	if( incrementTrackTemp == 1 )
	{
		if( recordingTrack < 15 )
		{
			if( viewingTrack == recordingTrack + 1 )
			{
				viewingTrack = recordingTrack + 2;
			}
			recordingTrack++;
		}
	}
	
    state = nextState;

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
	//Serial.println(tapHeadMessage);
	//Serial.println(inputHead.beats);
	
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

void LooperPanel::timersMIncrement( uint8_t inputValue )
{
	tapButton.buttonDebounceTimeKeeper.mIncrement(inputValue);
	syncButton.buttonDebounceTimeKeeper.mIncrement(inputValue);
	songUpButton.buttonDebounceTimeKeeper.mIncrement(inputValue);
	songDownButton.buttonDebounceTimeKeeper.mIncrement(inputValue);
	trackUpButton.buttonDebounceTimeKeeper.mIncrement(inputValue);
	trackDownButton.buttonDebounceTimeKeeper.mIncrement(inputValue);
	playButton.buttonDebounceTimeKeeper.mIncrement(inputValue);
	stopButton.buttonDebounceTimeKeeper.mIncrement(inputValue);
	option1Button.buttonDebounceTimeKeeper.mIncrement(inputValue);
	option2Button.buttonDebounceTimeKeeper.mIncrement(inputValue);
	option3Button.buttonDebounceTimeKeeper.mIncrement(inputValue);
	option4Button.buttonDebounceTimeKeeper.mIncrement(inputValue);
	
	rightDisplay.peekThroughTimeKeeper.mIncrement(inputValue);

	quantizingTrackTimeKeeper.mIncrement(inputValue);
}