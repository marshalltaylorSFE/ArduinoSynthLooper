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
#include "s7sWrapper.h"
//**Midi time keeping*************************//
#include "TimeCode.h"
//**Panel State Machine***********************//
#include "panelStateMachine.h"

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

TimerClass32 processSMTimer( 50000 );

TimerClass32 debounceTimer(5000);

//tick variable for interrupt driven timer1
uint32_t usTicks = 0;
uint8_t usTicksMutex = 1; //start locked out

//**Panels and stuff**************************//
Panel myPanel;

//**Seven Segment Display*********************//
// Here we'll define the I2C address of our S7S. By default it
//  should be 0x71. This can be changed, though.
char tempString[10];  // Will be used with sprintf to create strings
S7sObject leftDisplay( 0x71 );
S7sObject rightDisplay( 0x30 );

//**Panel State Machine***********************//
PanelStateMachine panelSM;

//**Midi time keeping*************************//
BeatCode playHead;
// tap LED stuff
uint16_t BPM = 80;
BeatCode tapHead;
TimerClass32 tapTempoPulseTimer( 0 );
uint32_t loopLength = 0xFFFFFFFF;

// MIDI things
#include <MIDI.h>
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, midiA);

#include "MicroLL.h"
MicroLL track[16];
MidiEvent * playBackNote[16];

uint8_t rxLedState = 0;
uint8_t txLedState = 0;

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
	rxLedState = 1;
	if( panelSM.recording == 1 )
	{
		MidiEvent tempEvent;
		tempEvent.timeStamp = tapHead.getTotalPulses();
		tempEvent.eventType = 0x90;
		tempEvent.channel = channel;
		tempEvent.value = pitch;
		tempEvent.data = velocity;
		track[panelSM.recordingTrack].pushObject( tempEvent );
	}
	midiA.sendNoteOn(pitch, velocity, channel);
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
	rxLedState = 1;

	if( panelSM.recording == 1 )
	{
		MidiEvent tempEvent;
		tempEvent.timeStamp = tapHead.getTotalPulses();
		tempEvent.eventType = 0x80;
		tempEvent.channel = channel;
		tempEvent.value = pitch;
		tempEvent.data = 0;
		track[panelSM.recordingTrack].pushObject( tempEvent );
	}
	midiA.sendNoteOff(pitch, velocity, channel);
	
}


// -----------------------------------------------------------------------------
void setup() 
{
	//Initialize serial:
	Serial.begin(9600);
	Serial.println("Program Started");
	
	//Init panel.h stuff
	myPanel.init();
	
	// initialize IntervalTimer
	myTimer.begin(serviceUS, 1);  // serviceMS to run every 0.001 seconds
	
	//Debug setting of random states and stuff
	
	//Set all LED off
	myPanel.recordLed.setState(LEDOFF);
	myPanel.playLed.setState(LEDOFF);
	myPanel.overdubLed.setState(LEDOFF);
	myPanel.tapLed.setState(LEDOFF);
	myPanel.syncLed.setState(LEDOFF);
	
	myPanel.update();
	
	//while(1);
	
	//debugTimeKeeper.mClear();
	
	leftDisplay.clear();  // Clears display, resets cursor
	leftDisplay.setBrightness(255);  // High brightness
	//sprintf(tempString, "%4d", (unsigned int)8888);
	leftDisplay.SendString("    ");
	
	rightDisplay.clear();  // Clears display, resets cursor
	rightDisplay.setBrightness(255);  // High brightness
	//sprintf(tempString, "%4d", (unsigned int)9999);
	rightDisplay.SendString("    ");
	
	
	//Update the BPM
	tapTempoPulseTimer.setInterval( tapTempoTimerMath( BPM ) );
	
	midiA.setHandleNoteOn(handleNoteOn);  // Put only the name of the function
	midiA.setHandleNoteOff(handleNoteOff);
	// Initiate MIDI communications, listen to all channels
	midiA.begin(MIDI_CHANNEL_OMNI);
	//midiA.turnThruOn();
	midiA.turnThruOff();
	
	for( int i = 0; i < 16; i++ )
	{
		playBackNote[i] = track[i].startObjectPtr;
	}
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
    processSMTimer.update(usTicks);	 
    tapTempoPulseTimer.update(usTicks);
	
	//**Copy to make a new timer******************//  
	//  if(msTimerA.flagStatus() == PENDING)
	//  {
	//    digitalWrite( LEDPIN, digitalRead(LEDPIN) ^ 1 );
	//  }
	
	//**Debounce timer****************************//  
	if(debounceTimer.flagStatus() == PENDING)
	{
		myPanel.timersMIncrement(5);
	
	}
		
	//**Update the panel LEDs and stuff***********//  
	if(panelUpdateTimer.flagStatus() == PENDING)
	{
		//Pass the state machine data
		// external now: myPanel.tapLed.setState(panelSM.tapLED);
		myPanel.syncLed.setState(panelSM.syncLED);
		myPanel.recordLed.setState(panelSM.recordLED);
		myPanel.playLed.setState(panelSM.playLED);
		myPanel.overdubLed.setState(panelSM.overdubLED);
		
		if( panelSM.resetTapHead == 1 )
		{
			tapHead.zero();
			panelSM.resetTapHead = 0;
		}
		if( panelSM.markLength == 1 )
		{
			loopLength = tapHead.getTotalPulses();
			panelSM.markLength = 0;
		}
		if( panelSM.clearAll == 1 )
		{
			for( int i = 0; i < 16; i++ )
			{	
				track[i].clear();
			}
			panelSM.clearAll = 0;
		}
		if( panelSM.screenControlTap == 1 )
		{
			char tempStringShort[4];
			sprintf(tempString, "%4d", (unsigned int)( tapHead.beats ));
			tempStringShort[3] = tempString[3];
			sprintf(tempString, "%4d", (unsigned int)( tapHead.bars ));
			tempStringShort[2] = tempString[3];
			sprintf(tempString, "%4d", (unsigned int)( tapHead.eightBars ));
			tempStringShort[1] = tempString[3];
			tempStringShort[0] = tempString[2];
			for( int i = 0; i < 4; i++)
			{
				tempString[i] = tempStringShort[i];
			}
		}
		else
		{
			sprintf(tempString, "%4d", (unsigned int)( BPM ));
		}
		// This will output the tempString to the S7S
		leftDisplay.SendString(tempString);
		sprintf(tempString, "%4d", (unsigned int)panelSM.trackNum);
		rightDisplay.SendString(tempString);
		
		//Also service the rx/tx leds
		if( rxLedState == 1)
		{
			rxLedState = 0;
			myPanel.rxLed.setState( LEDOFF );
		}
		else
		{
			myPanel.rxLed.setState( LEDON );
		}
		if( txLedState == 1)
		{
			txLedState = 0;
			myPanel.txLed.setState( LEDOFF );
		}
		else
		{
			myPanel.txLed.setState( LEDON );
		}
		
		//Check for new data  ( does myPanel.memberName.newData == 1? )
		myPanel.update();

		uint8_t tempStatus = 0;
		tempStatus |= myPanel.tapButton.newData;
		tempStatus |= myPanel.syncButton.newData;
		tempStatus |= myPanel.songUpButton.newData;
		tempStatus |= myPanel.songDownButton.newData;
		tempStatus |= myPanel.trackUpButton.newData;
		tempStatus |= myPanel.trackDownButton.newData;
		tempStatus |= myPanel.playButton.newData;
		tempStatus |= myPanel.stopButton.newData;
		tempStatus |= myPanel.quantizeSelector.newData;
		
		// If new, do something fabulous
		
		if( tempStatus )
		{
			panelSM.tapButton = myPanel.tapButton.getState();
			panelSM.syncButton = myPanel.syncButton.getState();
			
			if( myPanel.songUpButton.newData == 1 )
			{
				panelSM.songUpButton = myPanel.songUpButton.getState();
				if( myPanel.songUpButton.getState() == 1)
				{
					BPM++;
				}
			}
			
			if( myPanel.songDownButton.newData == 1 )
			{
				panelSM.songDownButton = myPanel.songDownButton.getState();
				if( myPanel.songDownButton.getState() == 1)
				{
					BPM--;
				}
			}

			panelSM.trackUpButton = myPanel.trackUpButton.getState();
			panelSM.trackDownButton = myPanel.trackDownButton.getState();
			if( myPanel.playButton.newData == 1 )
			{
				panelSM.playButton = myPanel.playButton.getState();
			}
			
			if( myPanel.stopButton.newData == 1 )
			{
				panelSM.stopButton = myPanel.stopButton.getState();
				if( myPanel.stopButton.getState() == 2)//BUTTONHOLD )
				{
					loopLength = 0xFFFFFFFF;
				}
			}

			if( myPanel.quantizeSelector.newData == 1 )
			{
				myPanel.quantizeSelector.getState();
			}
			//Update the BPM
			tapTempoPulseTimer.setInterval( tapTempoTimerMath( BPM ) );
			
		}
	}

	//**State machine process timer***************//  
	if(processSMTimer.flagStatus() == PENDING)
	{
		panelSM.tick();
		

	}
	
	//**Tap Tempo Pulse Timer*********************//  
	if(tapTempoPulseTimer.flagStatus() == PENDING)
	{
		if( tapHead.pulses == 0 )
		{
			myPanel.tapLed.setState(LEDON);
		}
		else
		{
			myPanel.tapLed.setState(LEDOFF);
		}
		tapHead.beatClockPulse();

		
		if( tapHead.getTotalPulses() > loopLength )
		{
			//Hardcoded, can't exceed 24 (1 beat) pulses
			uint32_t residualPulses = tapHead.getTotalPulses() - loopLength;
			tapHead.zero();
			tapHead.pulses = residualPulses;
			for( int i = 0; i < 16; i++ )
			{
				playBackNote[i] = track[i].startObjectPtr;
				track[i].printfMicroLL();
			}
		}
		
		//Send midi data OUT
		if( panelSM.playing == 1 )
		{
			for( int i = 0; i < 16; i++ )
			{
				//track[0].printfMicroLL();
				//while(1);
				//if we're looking at the null note, move on
				if( playBackNote[i] != &track[i].nullObject )
				{
					//Serial.println("Real object");
					//If the pulse count is greater than the next note, play it
					if( tapHead.getTotalPulses() >= playBackNote[i]->timeStamp )
					{
						Serial.println(playBackNote[i]->timeStamp);
						Serial.println(playBackNote[i]->eventType);
						Serial.println(playBackNote[i]->data);
						Serial.println(playBackNote[i]->channel);
						//while(1);
						txLedState = 1;
						switch( playBackNote[i]->eventType )
						{
						case 0x90: //Note on
							midiA.sendNoteOn( playBackNote[i]->value, playBackNote[i]->data, playBackNote[i]->channel );
							Serial.println("Note On");
							break;
						case 0x80: //Note off
							midiA.sendNoteOff( playBackNote[i]->value, playBackNote[i]->data, playBackNote[i]->channel );
							Serial.println("Note Off");
							break;
						default:
							break;
						}
						// move the note
						if( playBackNote[i]->nextObject != &track[i].nullObject )
						{
							playBackNote[i] = playBackNote[i]->nextObject;
						}
				
					}
					
				}
				else
				{
					Serial.println("Null object");
				}
				// MidiEvent tempEvent;
				// tempEvent.timeStamp = tapHead.getTotalPulses();
				// tempEvent.eventType = NoteOn;
				// tempEvent.channel = channel;
				// tempEvent.value = pitch;
				// tempEvent.data = velocity;
				// track[0].pushObject( tempEvent );
			}
			
		}
		

	}
	
	if(debugTimerClass.flagStatus() == PENDING)
	{

	}
	
	//**Fast LED toggling of the panel class***********//  
	if(ledToggleFastTimer.flagStatus() == PENDING)
	{
		myPanel.toggleFastFlasherState();
		
	}

	//**LED toggling of the panel class***********//  
	if(ledToggleTimer.flagStatus() == PENDING)
	{
		myPanel.toggleFlasherState();
		
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