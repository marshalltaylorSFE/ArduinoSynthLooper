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
#include "Arduino.h"
//**Timers and stuff**************************//
#include "timerModule32.h"

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
//Globals
uint32_t maxTimer = 60000000;
uint32_t MaxInterval = 2000000;

TimerClass32 midiOutputTimer( 1000 );
TimerClass32 midiInputTimer( 1000 );
TimerClass32 panelUpdateTimer(10000);
uint8_t debugLedStates = 1;

TimerClass32 ledToggleTimer( 333000 );
uint8_t ledToggleState = 0;
TimerClass32 ledToggleFastTimer( 100000 );
uint8_t ledToggleFastState = 0;

//TimerClass32 processSMTimer( 50000 );

TimerClass32 debounceTimer(5000);

TimerClass32 debugTimer(1000000);

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
//#include <midi_Defs.h>
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, midiA);

#include "midiDB.h"
MidiSong currentSong;

MicroLL rxNoteList;
MicroLL txNoteList;
MicroLL noteOnInList;
MicroLL noteOnPlayList[16];
MicroLL noteOnOutList[16];
MicroLL noteOnRecordList;

uint8_t rxLedFlag = 0;
uint8_t txLedFlag = 0;

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
	rxLedFlag = 1;
	MidiEvent tempEvent;
	tempEvent.timeStamp = tapHead.getTotalPulses();
	while(tempEvent.timeStamp >= loopLength )
	{
		tempEvent.timeStamp = tempEvent.timeStamp - loopLength;
	}
	tempEvent.eventType = 0x90;
	tempEvent.channel = channel;
	tempEvent.value = pitch;
	tempEvent.data = velocity;
		
	rxNoteList.pushObject( tempEvent );
	
	// This send should be taken care of elsewhere
	//midiA.sendNoteOn(pitch, velocity, channel);
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
	rxLedFlag = 1;
	
	MidiEvent tempEvent;
	tempEvent.timeStamp = tapHead.getTotalPulses();
//	Serial.print("*****loopLength at ");
//	Serial.println(loopLength);	
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
//	Serial.print("*****NoteOFF at ");
//	Serial.println(tempEvent.timeStamp);
	rxNoteList.pushObject( tempEvent );
	
	// This send should be taken care of elsewhere
	//midiA.sendNoteOff(pitch, velocity, channel);
	
}

void HandleControlChange(byte channel, byte number, byte value)
{
	midiA.sendControlChange( number, value, channel );
}

	
// -----------------------------------------------------------------------------
void setup() 
{
	tapHead.zero();
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
	midiA.setHandleControlChange(HandleControlChange);
	
	// Initiate MIDI communications, listen to all channels
	midiA.begin(MIDI_CHANNEL_OMNI);
	//midiA.turnThruOn();
	midiA.turnThruOff();
	
}

void loop()
{
//**Copy to make a new timer******************//  
//   msTimerA.update(usTicks);
	midiOutputTimer.update(usTicks);
	midiInputTimer.update(usTicks);
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
		//if( currentSong.txLedFlagService() )
		//{
		//	myLooperPanel.setTxLed();
		//}
		if( txLedFlag == 1 )
		{
			myLooperPanel.setTxLed();
			txLedFlag = 0;
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
//			Serial.println("Zero'd head");
		}
		if( myLooperPanel.markLengthFlag.serviceRisingEdge() )
		{
			myLooperPanel.lengthEstablishedFlag.setFlag();
			myLooperPanel.markLengthFlag.clearFlag();
			loopLength = tapHead.getQuantizedPulses( myLooperPanel.quantizeTrackTicks );
//			Serial.print("************************");
//			Serial.println(loopLength);
		}
		if( myLooperPanel.clearSongFlag.serviceRisingEdge() )
		{
			myLooperPanel.clearSongFlag.clearFlag();
			currentSong.clear();
			myLooperPanel.lengthEstablishedFlag.clearFlag();
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
		if( myLooperPanel.sendPanicFlag.serviceRisingEdge() )
		{
			//send panic
			midiA.sendRealTime(MIDI_NAMESPACE::SystemReset);
			myLooperPanel.sendPanicFlag.clearFlag();
		}
		
		currentSong.setRecordingTrack( myLooperPanel.getRecordingTrack() );
		
	}
	
	//Original quantization RHS
	// = tapHead.getQuantizedPulses( myLooperPanel.quantizeTicks );
	//Process the input functions (record and thru)
	if(midiInputTimer.flagStatus() == PENDING)
	{
		//Variable to record the length of the list
		listIdemNumber_t unservicedNoteCount = rxNoteList.listLength();
	
		// For now, chew through all of the rxNoteList
		MidiEvent tempNote;
		MidiEvent startTimeNote;
		int16_t tempSeekDepth;
		while( unservicedNoteCount > 0 )
		{
	
			tempNote = *rxNoteList.readObject( unservicedNoteCount - 1 ); //Reads oldest
			switch( tempNote.eventType )
			{
			case 0x90: //Note on
				//Search for the note on.  If found, do nothing, else write
				if( noteOnInList.seekObjectbyNoteValue( tempNote ) == -1 )
				{
					//note not found.
					//Mark key now on
					noteOnInList.pushObject( tempNote );
					//Put to tx buffer for through
					txNoteList.pushObject( tempNote );
					//record if necessary
					if( myLooperPanel.recordingFlag.getFlag() )
					{
						tempNote.timeStamp = tapHead.getQuantizedPulses( myLooperPanel.quantizeTicks );
						currentSong.recordNote( tempNote );
						noteOnRecordList.pushObject( tempNote );
					}
				}
				else
				{
					//Was found
					//do nothing
				}			
				break;
			case 0x80: //Note off
				//Search for the note on.
				tempSeekDepth = noteOnInList.seekObjectbyNoteValue( tempNote );
				if( tempSeekDepth == -1 )
				{
					//not found.
					//Do nothing
					
				}
				else
				{
					//Was found.  Time for note off actions
					startTimeNote = *noteOnRecordList.readObject( noteOnRecordList.seekObjectbyNoteValue( tempNote ));
					//Remove from key list
					noteOnInList.dropObject( tempSeekDepth );
					//Put to tx buffer
					txNoteList.pushObject( tempNote );
					if( myLooperPanel.recordingFlag.getFlag() )
					{
						//Check for quantization
						if( 1 )
						{
							tempNote.timeStamp = startTimeNote.timeStamp + tapHead.convertLengthInput( (tapHead.getTotalPulses() - startTimeNote.timeStamp ), myLooperPanel.quantizeNoteLengthTicks );
							if( myLooperPanel.lengthEstablishedFlag.getFlag() )
							{
								//do overlength check (also needed at close of record
								while( tempNote.timeStamp >= loopLength )
								{
									tempNote.timeStamp -= loopLength;
								}
							}
						}
						currentSong.recordNote( tempNote );
						//Remove from on recording list
						noteOnRecordList.dropObject( noteOnRecordList.seekObjectbyNoteValue( tempNote ));
					}
				}
				break;
			default:
				break;
			}
			//Note has been fully dealt with, remove from input buffer
			rxNoteList.dropObject( unservicedNoteCount - 1 );
			
			//Get the list length now
			unservicedNoteCount = rxNoteList.listLength();
			
		} //while list exists
		
		//If this is the first of a record, save all note-ons 
		if( myLooperPanel.recordingFlag.serviceRisingEdge() )
		{
			Serial.println("Service rising edge of recording");
			MidiEvent * tempNotePtr;
			//Look for note ons
			if( noteOnInList.listLength() )
			{
				//Go through the entire list here, once, non-removing
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
					//Also save as a current input note on
					noteOnRecordList.pushObject( *tempNotePtr );
					//Now we've saved that the note is
					//on without off and when it started
					//  
				}
			}
			
		}
		//If the recording has just ended,
		if( myLooperPanel.recordingFlag.serviceFallingEdge() )
		{
			Serial.println("Service falling edge of recording");
			//////////////////	Serial.println("NoteOnOutLists:");
			//////////////////	noteOnOutList[0].printfNoteGraph();
			//////////////////	noteOnOutList[1].printfNoteGraph();
			//////////////////	Serial.println("NoteOnPlayLists:");
			//////////////////	noteOnPlayList[0].printfNoteGraph();
			//////////////////	noteOnPlayList[1].printfNoteGraph();
			MidiEvent * tempNotePtr;
			//Look for note ons in the record on list, make up note-offs
			if( noteOnRecordList.listLength() )
			{
				Serial.print("****************");
				noteOnInList.printfNoteGraph();
				//go through entire record on list once
				for( int i = noteOnRecordList.listLength(); i > 0; i-- )
				{
					//Record the corrisponding note offs
					tempNotePtr = noteOnRecordList.readObject( i - 1 );
					// first bring out of range notes in.
					//  HERE IS WHERE A ROLL OVERLAP PREVENTION COULD BE DONE
					//Check for quantization
					if( 1 )
					{
						int32_t tempTime = tapHead.getTotalPulses();
						if( tempTime <= (int32_t)tempNotePtr->timeStamp )
						{
							tempTime += loopLength;
						}
						tempNotePtr->timeStamp = tempNotePtr->timeStamp + tapHead.convertLengthInput( (tempTime - tempNotePtr->timeStamp ), myLooperPanel.quantizeNoteLengthTicks );
						//lengthEstablished SHOULD ALWAYS be set when the rec falls.
						while( tempNotePtr->timeStamp >= loopLength )
						{
							tempNotePtr->timeStamp -= loopLength;
						}
					}	
					//tempNotePtr->timeStamp = loopLength - 1;
					//Change the type
					tempNotePtr->eventType = 0x80;
					tempNotePtr->data = 0;
					//Record
				
					currentSong.recordNote( *tempNotePtr );
					noteOnRecordList.dropObject((listIdemNumber_t)(i-1));
				}
	
			}
			//noteOnRecordList should be purged by now, and note offs placed.
			
		}
		//	if((noteOnInList.listLength())||(rxNoteList.listLength())||(txNoteList.listLength()))
		//	{
		//		Serial.println("\nNoteOnInList:");
		//		noteOnInList.printfMicroLL();	
		//		Serial.println("\nrxNoteList:");	
		//		rxNoteList.printfMicroLL();	
		//		Serial.println("\ntxNoteList:");	
		//		txNoteList.printfMicroLL();	
		//	}
	}

		
	if( midiOutputTimer.flagStatus() == PENDING )
	{
		//1. (priority) flush the lists if requested -- Don't touch noteOnRecordList
		//2. Process the tx list:
		//  Get the note on presency information
		//  If note on, add and send if not present
		//  If note off, remove and send if present
		if( myLooperPanel.playingFlag.serviceFallingEdge() )
		{
			Serial.println("Service falling edge of playing");
			Serial.print("\n\ncurrentSong\n");
			currentSong.track[0].printfNoteGraph();
			currentSong.track[1].printfNoteGraph();
			//Send tx for all notes from the noteOnPlayList -- just what the midiDB has produced
			MidiEvent * tempNotePtr;
			MidiEvent tempNote;
			//Look for note ons
			for( int channeli = 0; channeli < 16; channeli++ )
			{
				if( noteOnPlayList[channeli].listLength() )
				{
					for( int i = noteOnPlayList[channeli].listLength(); i > 0; i-- )
					{
						//Serial.print(i);
						//Serial.print(",");
						//Play all the note-offs
						tempNotePtr = noteOnPlayList[channeli].readObject( i - 1 );
						tempNote = *tempNotePtr;
						tempNote.eventType = 0x80;
						tempNote.data = 0;
						txNoteList.pushObject(tempNote);
						//Destroy the event
						noteOnPlayList[channeli].dropObject( i - 1 );
					}
				}
				//Serial.print("\n");
			}
		}

		uint8_t tempEnabledArray[16];
		myLooperPanel.getTrackMute( tempEnabledArray );
		currentSong.setPlayEnabled( tempEnabledArray );
		
		//If it is time to play the returned note,
		//This section only writes to txNoteList
		MidiEvent noteToPlay;
		if( myLooperPanel.playingFlag.getFlag() )
		{
			if( currentSong.getNextNote( noteToPlay, tapHead.getTotalPulses() ) == 1 )
			{

				//  Get the note on presency information
				int16_t noteOnPlayPresent = noteOnPlayList[noteToPlay.channel - 1].seekObjectbyNoteValue( noteToPlay );
				Serial.print("Note timestamp:");
				Serial.println(noteToPlay.timeStamp);

				switch( noteToPlay.eventType )
				{
				case 0x90: //Note on
				//Search for the note on.  If found, do nothing, else write
					if( noteOnPlayPresent == -1 )
					{
						//note not found.
						//push new note to the list
						noteOnPlayList[noteToPlay.channel - 1].pushObject( noteToPlay );
						txNoteList.pushObject( noteToPlay );
						Serial.println("Pushed Note On to txNoteList");
					}
					else
					{
						//Was found
						//Do nothing
						
					}
					break;
				case 0x80: //Note off, remove and send if present
					//Search for the note off.
					if( noteOnPlayPresent == -1 )
					{
						//Not found
						//Do nothing
						
					}
					else
					{
						//Was found.  Time for note off actions
						noteOnPlayList[noteToPlay.channel - 1].dropObject( noteOnPlayPresent );
						txNoteList.pushObject( noteToPlay );
						Serial.println("Pushed Note Off to txNoteList");
					}
					//Serial.println("Note Off");
					break;
				default:
					break;
				}			
			}
		}// if playing flag == 1

		//Parse the output
		listIdemNumber_t unservicedNoteCount = txNoteList.listLength();

		while( unservicedNoteCount > 0 )
		{
			noteToPlay = *txNoteList.readObject( unservicedNoteCount - 1 );
			
			//  Get the note on presency information
			int16_t noteOnPlayPresent = noteOnPlayList[noteToPlay.channel - 1].seekObjectbyNoteValue( noteToPlay );
			int16_t noteOnOutPresent = noteOnOutList[noteToPlay.channel - 1].seekObjectbyNoteValue( noteToPlay );
			int16_t noteOnInPresent = noteOnInList.seekObjectbyNoteValue( noteToPlay );

			//Serial.println("noteOnPlayPresent");
			//Serial.println(noteOnPlayPresent);
			//Serial.println("noteOnOutPresent");
			//Serial.println(noteOnOutPresent);			
			//Serial.println("noteOnInPresent");
			//Serial.println(noteOnInPresent);
			
			switch( noteToPlay.eventType )
			{
			case 0x90: //Note on
				//Search for the note on.
				if( noteOnOutPresent == -1 )
				{
					//note not found.
					//push new note to the list
					noteOnOutList[noteToPlay.channel - 1].pushObject( noteToPlay );
					midiA.sendNoteOn( noteToPlay.value, noteToPlay.data, noteToPlay.channel );
					Serial.println("Sent note on to midi");
					txLedFlag = 1;
				}
				else
				{
					//Was found
					//Do nothing (or could retrigger)
					midiA.sendNoteOff( noteToPlay.value, 0, noteToPlay.channel );
					midiA.sendNoteOn( noteToPlay.value, noteToPlay.data, noteToPlay.channel );
					Serial.println("retriggered to midi");
					txLedFlag = 1;
					
				}
				break;
			case 0x80: //Note off, remove and send if present
				//Search for the note off.
				if( noteOnOutPresent == -1 )
				{
					//No note on,
					//Do nothing
					
				}
				else if(( noteOnPlayPresent != -1 )||( noteOnInPresent != -1 ))
				{
					//Some input still has asserted key, do nothing
				}
				else
				{
					//No input has asserted key, note exists in output
					//Turn off
					midiA.sendNoteOff( noteToPlay.value, noteToPlay.data, noteToPlay.channel );
					noteOnOutList[noteToPlay.channel - 1].dropObject( noteOnOutPresent );
					Serial.println("Sent note off to midi");
					txLedFlag = 1;
				}
				//Serial.println("Note Off");
				break;
			default:
				break;
			}			
			
			//Note has been fully dealt with, remove from input buffer
			//Also, put in in tx buffer
			txNoteList.dropObject( unservicedNoteCount - 1 );
			
			//Get the length now
			unservicedNoteCount = txNoteList.listLength();
		} //while txNoteList has contents
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
		Serial.println("**************************Debug Service**************************");
		Serial.print("tapHead: ");
		Serial.println(tapHead.getTotalPulses());
		if((noteOnInList.listLength())||(rxNoteList.listLength())||(txNoteList.listLength())||(noteOnOutList[0].listLength())||(noteOnOutList[1].listLength()))
		{
			Serial.println("\nNoteOnInList:");
			noteOnInList.printfMicroLL();	
			Serial.println("\nrxNoteList:");	
			rxNoteList.printfMicroLL();	
			Serial.println("\ntxNoteList:");	
			txNoteList.printfMicroLL();	
			Serial.println("\nnoteOnOutList[0]:");	
			noteOnOutList[0].printfMicroLL();	
			Serial.println("\nnoteOnOutList[1]:");	
			noteOnOutList[1].printfMicroLL();	
			}
/*		if(myLooperPanel.playingFlag.getFlag())
		{
			Serial.println("...Playing");
		}
		else
		{
			Serial.println(".");
		}
		Serial.print("\n\nrxNoteList\n");
		rxNoteList.printfMicroLL();
		Serial.print("\n\nnoteOnInList\n");
		noteOnInList.printfMicroLL();
		Serial.print("\n\nnoteOnPlayLists\n");
		for( int i = 0; i < 16; i++ )
		{
			Serial.print("i = ");
			Serial.print( i + 1 );
			noteOnPlayList[i].printfMicroLL();
		}
		Serial.print("\n\ncurrentSong\n");
		currentSong.track[0].printfMicroLL();
	
*/	}
	midiA.read();

	
}

void serviceUS(void)
{
  uint32_t returnVar = 0;
  if(usTicks >= ( maxTimer + MaxInterval ))
  {
    returnVar = usTicks - maxTimer;

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