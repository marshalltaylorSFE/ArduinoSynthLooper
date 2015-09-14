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

TimerClass32 debugTimerClass( 333000 );
TimerClass32 panelUpdateTimer(10000);
uint8_t debugLedStates = 1;

TimerClass32 ledToggleTimer( 333000 );
uint8_t ledToggleState = 0;
TimerClass32 ledToggleFastTimer( 100000 );
uint8_t ledToggleFastState = 0;

//TimerClass32 processSMTimer( 50000 );

TimerClass32 debounceTimer(5000);

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

uint8_t rxLedFlag = 0;

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
	rxLedFlag = 1;
	if( myLooperPanel.recording == 1 )
	{
		MidiEvent tempEvent;
		tempEvent.timeStamp = tapHead.getTotalPulses();
		tempEvent.eventType = 0x90;
		tempEvent.channel = channel;
		tempEvent.value = pitch;
		tempEvent.data = velocity;
		
		currentSong.recordNote( tempEvent );
	}
	midiA.sendNoteOn(pitch, velocity, channel);
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
	rxLedFlag = 1;

	if( myLooperPanel.recording == 1 )
	{
		MidiEvent tempEvent;
		tempEvent.timeStamp = tapHead.getTotalPulses();
		tempEvent.eventType = 0x80;
		tempEvent.channel = channel;
		tempEvent.value = pitch;
		tempEvent.data = 0;
		
		currentSong.recordNote( tempEvent );
	}
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
	debugTimerClass.update(usTicks);
	ledToggleTimer.update(usTicks);
	ledToggleFastTimer.update(usTicks);
	panelUpdateTimer.update(usTicks);
	debounceTimer.update(usTicks);
    tapTempoPulseTimer.update(usTicks);
	
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
		if( myLooperPanel.serviceResetTapHead() )
		{
			tapHead.zero();
		}
		if( myLooperPanel.serviceMarkLength() )
		{
			loopLength = tapHead.getTotalPulses();
		}
		if( myLooperPanel.serviceSongClearRequest() )
		{
			currentSong.clear();
			loopLength = 0xFFFFFFFF;
		}
		if( myLooperPanel.serviceBPMUpdateRequest() )
		{
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
		
		//Send midi data OUT
		if( myLooperPanel.playing == 1 )
		{
			uint8_t tempEnabledArray[16];
			myLooperPanel.getTrackMute( tempEnabledArray );
			currentSong.setPlayEnabled( tempEnabledArray );
			
			MidiEvent noteToPlay;
			//If it is time to play the returned note,
			if( currentSong.getNextNote( noteToPlay, tapHead.getTotalPulses() ) == 1 )
			{
				switch( noteToPlay.eventType )
				{
				case 0x90: //Note on
					midiA.sendNoteOn( noteToPlay.value, noteToPlay.data, noteToPlay.channel );
					//Serial.println("Note On");
					break;
				case 0x80: //Note off
					midiA.sendNoteOff( noteToPlay.value, noteToPlay.data, noteToPlay.channel );
					//Serial.println("Note Off");
					break;
				default:
					break;
				}				
			}
			
		}
		

	}
	
	if(debugTimerClass.flagStatus() == PENDING)
	{

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