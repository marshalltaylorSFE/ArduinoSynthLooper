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

//**Timers and stuff**************************//
#include "timerModule32.h"
#define MAXINTERVAL 2000000 //Max TimerClass interval
#include "timeKeeper.h"
//**Panels and stuff**************************//
#include <Wire.h>
#include "Panel.h"
//**Seven Segment Display*********************//
#include "Wire.h"
//#include "s7sWrapper.h"
//**Midi time keeping*************************//
#include "TimeCode.h"

//**Timers and stuff**************************//
IntervalTimer myTimer;

//HOW TO OPERATE
//  Make TimerClass objects for each thing that needs periodic service
//  pass the interval of the period in ticks
//  Set MAXINTERVAL to the max foreseen interval of any TimerClass
//  Set MAXTIMER to overflow number in the header.  MAXTIMER + MAXINTERVAL
//    cannot exceed variable size.

TimerClass32 midiPlayTimer( 1000 );
TimerClass32 midiRecordTimer( 1000 );
TimerClass32 panelUpdateTimer(10000);
uint8_t debugLedStates = 1;

TimerClass32 ledToggleTimer( 333000 );
uint8_t ledToggleState = 0;
TimerClass32 ledToggleFastTimer( 100000 );
uint8_t ledToggleFastState = 0;

//TimerClass32 processSMTimer( 50000 );

TimerClass32 debounceTimer(5000);

TimerClass32 debugTimer(2000000);

//tick variable for interrupt driven timer1
uint32_t usTicks = 0;
uint8_t usTicksMutex = 1; //start locked out

//**Panel State Machine***********************//
#include "looperPanel.h"
LooperPanel myLooperPanel;

//**Midi time keeping*************************//
BeatCode playHead;
// tap LED stuff
BeatCode tapHead;
TimerClass32 tapTempoPulseTimer( 0 );
uint32_t loopLength = 0xFFFFFFFF;

// MIDI things
#include <MIDI.h>
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, midiA);

#include "midiDB.h"
MidiSong currentSong;

MicroLL rxNoteList;
MicroLL noteOnInList;
MicroLL noteOnOutList[16];

uint8_t rxLedFlag = 0;

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
	rxLedFlag = 1;
	MidiEvent tempEvent;
	tempEvent.timeStamp = tapHead.getQuantizedPulses( myLooperPanel.quantizeTicks );
	if(tempEvent.timeStamp >= loopLength )
	{
		tempEvent.timeStamp = tempEvent.timeStamp - loopLength;
	}
	tempEvent.eventType = 0x90;
	tempEvent.channel = channel;
	tempEvent.value = pitch;
	tempEvent.data = velocity;
		
	rxNoteList.pushObject( tempEvent );
	
	midiA.sendNoteOn(pitch, velocity, channel);
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
	rxLedFlag = 1;
	
	MidiEvent tempEvent;
	tempEvent.timeStamp = tapHead.getQuantizedPulses( myLooperPanel.quantizeTicks );
	Serial.print("*****loopLength at ");
	Serial.println(loopLength);	
	if(tempEvent.timeStamp >= loopLength )
	{
		tempEvent.timeStamp = tempEvent.timeStamp - loopLength;
	}
	if(loopLength == 0xFFFFFFFF)
	{
		if( tempEvent.timeStamp != 0 ) //Shorten all first loop notes
		{
			tempEvent.timeStamp--;
		}
	}
	tempEvent.eventType = 0x80;
	tempEvent.channel = channel;
	tempEvent.value = pitch;
	tempEvent.data = 0;
	Serial.print("*****NoteOFF at ");
	Serial.println(tempEvent.timeStamp);
	rxNoteList.pushObject( tempEvent );
	
	midiA.sendNoteOff(pitch, velocity, channel);
	
}


// -----------------------------------------------------------------------------
void setup() 
{
	//Initialize serial:
	Serial.begin(9600);
	delay(2000);
	Serial.println("Program Started");
	
	//Init panel.h stuff
	myLooperPanel.init();
	
	// initialize IntervalTimer
	myTimer.begin(serviceUS, 1);  // serviceMS to run every 0.001 seconds
	
	//Go to fresh state
	myLooperPanel.reset();
	
	//Update the panel
	myLooperPanel.update();
	
	//Update the BPM
	tapTempoPulseTimer.setInterval( tapTempoTimerMath( myLooperPanel.BPM ) );
	
	//Connect MIDI handlers
	midiA.setHandleNoteOn(handleNoteOn);  // Put only the name of the function
	midiA.setHandleNoteOff(handleNoteOff);
	
	// Initiate MIDI communications, listen to all channels
	midiA.begin(MIDI_CHANNEL_OMNI);
	//midiA.turnThruOn();
	midiA.turnThruOff();
	
}

void loop()
{
//**Copy to make a new timer******************//  
//   msTimerA.update(usTicks);
	midiPlayTimer.update(usTicks);
	midiRecordTimer.update(usTicks);
	ledToggleTimer.update(usTicks);
	ledToggleFastTimer.update(usTicks);
	panelUpdateTimer.update(usTicks);
	debounceTimer.update(usTicks);
    tapTempoPulseTimer.update(usTicks);
	debugTimer.update(usTicks);
	
	//**Copy to make a new timer******************//  
	//  if(msTimerA.flagStatus() == PENDING)
	//  {
	//    digitalWrite( LEDPIN, digitalRead(LEDPIN) ^ 1 );
	//  }
	
	//**Debounce timer****************************//  
	if(debounceTimer.flagStatus() == PENDING)
	{
		myLooperPanel.timersMIncrement(5);
	
	}
		
	//**Process the panel and state machine***********//  
	if(panelUpdateTimer.flagStatus() == PENDING)
	{
		//Provide inputs
		if( currentSong.txLedFlagService() )
		{
			myLooperPanel.setTxLed();
		}
		if( rxLedFlag == 1 )
		{
			rxLedFlag = 0;
			myLooperPanel.setRxLed();
		}
		myLooperPanel.setTapHeadMessage(tapHead);
		//Tick the machine
		myLooperPanel.processMachine();
		
		//Deal with outputs
		if( myLooperPanel.resetTapHeadFlag.serviceRisingEdge() )
		{
			myLooperPanel.resetTapHeadFlag.clearFlag();
			tapHead.zero();
			Serial.println("Zero'd head");
		}
		if( myLooperPanel.markLengthFlag.serviceRisingEdge() )
		{
			myLooperPanel.markLengthFlag.clearFlag();
			loopLength = tapHead.getQuantizedPulses( myLooperPanel.quantizeTrackTicks );
			Serial.print("************************");
			Serial.println(loopLength);
		}
		if( myLooperPanel.clearSongFlag.serviceRisingEdge() )
		{
			myLooperPanel.clearSongFlag.clearFlag();
			currentSong.clear();
			loopLength = 0xFFFFFFFF;
		}
		if( myLooperPanel.clearTrackFlag.serviceRisingEdge() )
		{
			currentSong.clearTrack( myLooperPanel.trackToClear );
			myLooperPanel.clearTrackFlag.clearFlag();
		}
		if( myLooperPanel.updateBPMFlag.serviceRisingEdge() )
		{
			myLooperPanel.updateBPMFlag.clearFlag();
			//Update the BPM
			tapTempoPulseTimer.setInterval( tapTempoTimerMath( myLooperPanel.BPM ) );
		}
		currentSong.setRecordingTrack( myLooperPanel.getRecordingTrack() );
	}
	

	//**Tap Tempo Pulse Timer*********************//  
	if(tapTempoPulseTimer.flagStatus() == PENDING)
	{
		if( tapHead.pulses == 0 )
		{
			myLooperPanel.setTapLed();
		}

		tapHead.beatClockPulse();

		
		if( tapHead.getTotalPulses() >= loopLength )
		{
			//Hardcoded, can't exceed 24 (1 beat) pulses
			uint32_t residualPulses = tapHead.getTotalPulses() - loopLength;
			tapHead.zero();
			tapHead.pulses = residualPulses;
			
			currentSong.reset();
			
		}

	
	}
	if(midiRecordTimer.flagStatus() == PENDING)
	{
		//
		listIdemNumber_t unservicedNoteCount = rxNoteList.listLength();
		//Get a note for this round
		MidiEvent tempNote;
		if( unservicedNoteCount > 0 )
		{
			tempNote = *rxNoteList.readObject( unservicedNoteCount - 1 );
			if( tempNote.eventType == 0x90 )//We got a new note-on
			{
				//Search for the note on.  If found, do nothing, else write
				if( noteOnInList.seekObjectbyNoteValue( tempNote ) == -1 )
				{
					//note not found.  record
					noteOnInList.pushObject( tempNote );
					if( myLooperPanel.recordingFlag.getFlag() )
					{
						currentSong.recordNote( tempNote );
					}
					rxNoteList.dropObject( unservicedNoteCount - 1 );
				}
				else
				{
					//Was found
					//do nothing
				}
			}
			else if( tempNote.eventType == 0x80 )
			{
				//Congratulations! It's a note off!
				//Search for the note on.  If found, do nothing, else write
				int8_t tempSeekDepth = noteOnInList.seekObjectbyNoteValue( tempNote );
				if( tempSeekDepth == -1 )
				{
					//not found.
					//Do nothing
					
				}
				else
				{
					//Was found.  Time for note off actions
					Serial.print("Dropping ");
					Serial.println( tempSeekDepth );
					noteOnInList.dropObject( tempSeekDepth );
					if( myLooperPanel.recordingFlag.getFlag() )
					{
						Serial.print("tempnote ");
						Serial.println(tempNote.timeStamp);
						currentSong.recordNote( tempNote );
					}
					rxNoteList.dropObject( unservicedNoteCount - 1 );
				}				
			}
			else
			{
				//Destroy the crappy data!
				rxNoteList.dropObject( unservicedNoteCount );
			}
		}
		//else we no new data!
		else
		{
			//If this is the first of a record, save all note-ons 
			if( myLooperPanel.recordingFlag.serviceRisingEdge() )
			{
				MidiEvent * tempNotePtr;
				//Look for note ons
				if( noteOnInList.listLength() )
				{
					for( int i = noteOnInList.listLength(); i > 0; i-- )
					{
						//Record all the note-ons
						tempNotePtr = noteOnInList.readObject( i - 1 );
						if( myLooperPanel.songHasDataFlag.getFlag() == 0 )
						{
							//If song has no data, change the time to zero
							tempNotePtr->timeStamp = 0;
						}
						currentSong.recordNote( *tempNotePtr );
					}
				}
				
			}
		}
	}
	
	if(midiPlayTimer.flagStatus() == PENDING)
	{
		//Send midi data OUT
		if( myLooperPanel.playingFlag.getFlag() )
		{
			uint8_t tempEnabledArray[16];
			myLooperPanel.getTrackMute( tempEnabledArray );
			currentSong.setPlayEnabled( tempEnabledArray );
			MidiEvent noteToPlay;
			//If it is time to play the returned note,
			if( currentSong.getNextNote( noteToPlay, tapHead.getTotalPulses() ) == 1 )
			{
				uint8_t channelMatrixRev = noteToPlay.channel - 1;
				int8_t tempSeekDepth2;
				switch( noteToPlay.eventType )
				{
				case 0x90: //Note on
				//Search for the note on.  If found, do nothing, else write
					if( noteOnOutList[channelMatrixRev].seekObjectbyNoteValue( noteToPlay ) == -1 )
					{
						//note not found.
						noteOnOutList[channelMatrixRev].pushObject( noteToPlay );
						midiA.sendNoteOn( noteToPlay.value, noteToPlay.data, noteToPlay.channel );
					}
					else
					{
						//Was found
						//Do nothing
					}
					//Serial.println("Note On");
					break;
				case 0x80: //Note off
					//Search for the note on.  If found, do nothing, else write
					tempSeekDepth2 = noteOnOutList[channelMatrixRev].seekObjectbyNoteValue( noteToPlay );
					if( tempSeekDepth2 == -1 )
					{
						//not found.
						//Do nothing
						
					}
					else
					{
						//Was found.  Time for note off actions
						noteOnOutList[channelMatrixRev].dropObject( tempSeekDepth2 );
						midiA.sendNoteOff( noteToPlay.value, noteToPlay.data, noteToPlay.channel );
					}
					//Serial.println("Note Off");
					break;
				default:
					break;
				}			
			}
			
		}
		else if( myLooperPanel.playingFlag.serviceFallingEdge() )
		{
			//Turn off all other notes
			MidiEvent * tempNotePtr;
			//Look for note ons
			for( int channeli = 1; channeli < 17; channeli++ )
			{
				if( noteOnOutList[channeli].listLength() )
				{
					for( int i = noteOnOutList[channeli].listLength(); i > 0; i-- )
					{
						//Play all the note-offs
						tempNotePtr = noteOnOutList[channeli].readObject( i - 1 );
						midiA.sendNoteOff( tempNotePtr->value, 0, tempNotePtr->channel );
						//Destroy the event
						noteOnOutList[channeli].dropObject( i - 1 );
					}
				}
			}
			
		}
	}
	
	//**Fast LED toggling of the panel class***********//  
	if(ledToggleFastTimer.flagStatus() == PENDING)
	{
		myLooperPanel.toggleFastFlasherState();
		
	}

	//**LED toggling of the panel class***********//  
	if(ledToggleTimer.flagStatus() == PENDING)
	{
		myLooperPanel.toggleFlasherState();
		
	}
	//**Debug timer*******************************//  
	if(debugTimer.flagStatus() == PENDING)
	{
		Serial.print("\n\nrxNoteList\n");
		rxNoteList.printfMicroLL();
		Serial.print("\n\nnoteOnInList\n");
		noteOnInList.printfMicroLL();
		Serial.print("\n\nnoteOnOutLists\n");
		for( int i = 0; i < 16; i++ )
		{
			Serial.print("i = ");
			Serial.print( i + 1 );
			noteOnOutList[i].printfMicroLL();
		}
		Serial.print("\n\ncurrentSong\n");
		currentSong.track[0].printfMicroLL();
	
	}
	midiA.read();

	
}

void serviceUS(void)
{
  uint32_t returnVar = 0;
  if(usTicks >= ( MAXTIMER + MAXINTERVAL ))
  {
    returnVar = usTicks - MAXTIMER;

  }
  else
  {
    returnVar = usTicks + 1;
  }
  usTicks = returnVar;
  usTicksMutex = 0;  //unlock
}

uint32_t tapTempoTimerMath( uint16_t BPMInput )
{
	uint32_t returnVar = 0;
	
	returnVar = 2500000 /( (uint32_t)BPMInput );
	return returnVar;
}