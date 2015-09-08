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

//**Panels and stuff**************************//

#include "Panel.h"
#include "PanelComponents.h"

//Panel related variables
Panel myPanel;

//**Timers and stuff**************************//
#include "timerModule32.h"

//Globals
IntervalTimer myTimer;

#define MAXINTERVAL 2000000 //Max TimerClass interval

//**Panel State Machine***********************//
#include "panelStateMachine.h"
PanelStateMachine panelSM;


//**Midi time keeping*************************//
#include "TimeCode.h"
BeatCode playHead;
// tap LED stuff
uint16_t BPM = 80;
BeatCode tapHead;
TimerClass32 tapTempoPulseTimer( 0 );
uint32_t loopLength = 0xFFFFFFFF;

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
#include "timeKeeper.h"



//tick variable for interrupt driven timer1
uint32_t usTicks = 0;
uint8_t usTicksMutex = 1; //start locked out

//**Seven Segment Display*********************//
// Here we'll define the I2C address of our S7S. By default it
//  should be 0x71. This can be changed, though.
#include "Wire.h"
const byte s7sAddress = 0x71;
char tempString[10];  // Will be used with sprintf to create strings

// MIDI things
#include <MIDI.h>
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, midiA);

#include "MicroLL.h"
MicroLL track1( 1000 );
MidiEvent * playBackNote;

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
	if( panelSM.recording == 1 )
	{
		MidiEvent tempEvent;
		tempEvent.timeStamp = tapHead.getTotalPulses();
		tempEvent.eventType = 0x90;
		tempEvent.channel = 1;
		tempEvent.value = pitch;
		tempEvent.data = velocity;
		track1.pushObject( tempEvent );
	}
	midiA.sendNoteOn(pitch, velocity, channel);
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
	if( panelSM.recording == 1 )
	{
		MidiEvent tempEvent;
		tempEvent.timeStamp = tapHead.getTotalPulses();
		tempEvent.eventType = 0x80;
		tempEvent.channel = 1;
		tempEvent.value = pitch;
		tempEvent.data = 0;
		track1.pushObject( tempEvent );
	}
	midiA.sendNoteOff(pitch, velocity, channel);
	
}


// -----------------------------------------------------------------------------
void setup() 
{
  //Initialize serial:
  Serial.begin(9600);

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
  
  Wire.begin();  // Initialize hardware I2C pins
  // Clear the display, and then turn on all segments and decimals
  clearDisplayI2C();  // Clears display, resets cursor
  setBrightnessI2C(255);  // High brightness
    // Magical sprintf creates a string for us to send to the s7s.
  //  The %4d option creates a 4-digit integer.
  sprintf(tempString, "%4d", (unsigned int)8888);
  // This will output the tempString to the S7S
  s7sSendStringI2C(tempString);

  
  //Update the BPM
  tapTempoPulseTimer.setInterval( tapTempoTimerMath( BPM ) );

  midiA.setHandleNoteOn(handleNoteOn);  // Put only the name of the function
  midiA.setHandleNoteOff(handleNoteOff);
  // Initiate MIDI communications, listen to all channels
  midiA.begin(MIDI_CHANNEL_OMNI);
  //midiA.turnThruOn();
  midiA.turnThruOff();
  
  playBackNote = track1.startObjectPtr;
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
		s7sSendStringI2C(tempString);
		
		myPanel.update();
		//Check for new data  ( does myPanel.memberName.newData == 1? )
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
			playBackNote = track1.startObjectPtr;
			track1.printfMicroLL();
		}
		
		//Send midi data OUT
		if( panelSM.playing == 1 )
		{
		    //track1.printfMicroLL();
			//while(1);
			//if we're looking at the null note, move on
			if( playBackNote != &track1.nullObject )
			{
				//Serial.println("Real object");
				//If the pulse count is greater than the next note, play it
				if( tapHead.getTotalPulses() >= playBackNote->timeStamp )
				{
					Serial.println(playBackNote->timeStamp);
					Serial.println(playBackNote->eventType);
					Serial.println(playBackNote->data);
					Serial.println(playBackNote->channel);
					//while(1);
					switch( playBackNote->eventType )
					{
					case 0x90: //Note on
						midiA.sendNoteOn( playBackNote->value, playBackNote->data, 1 );
						Serial.println("Note On");
						break;
					case 0x80: //Note off
						midiA.sendNoteOff( playBackNote->value, playBackNote->data, 1 );
						Serial.println("Note Off");
						break;
					default:
						break;
					}
					// move the note
					if( playBackNote->nextObject != &track1.nullObject )
					{
						playBackNote = playBackNote->nextObject;
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
			// track1.pushObject( tempEvent );
		}

	}
	
	if(debugTimerClass.flagStatus() == PENDING)
	{

	}
	
	//**LED toggling of the panel class***********//  
	if(ledToggleFastTimer.flagStatus() == PENDING)
	{
	ledToggleFastState = ledToggleFastState ^ 0x01;
	
	}
	if(ledToggleTimer.flagStatus() == PENDING)
	{
	ledToggleState = ledToggleState ^ 0x01;
	
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

// This custom function works somewhat like a serial.print.
//  You can send it an array of chars (string) and it'll print
//  the first 4 characters in the array.
void s7sSendStringI2C(String toSend)
{
  Wire.beginTransmission(s7sAddress);
  for (int i=0; i<4; i++)
  {
    Wire.write(toSend[i]);
  }
  Wire.endTransmission();
}

// Send the clear display command (0x76)
//  This will clear the display and reset the cursor
void clearDisplayI2C()
{
  Wire.beginTransmission(s7sAddress);
  Wire.write(0x76);  // Clear display command
  Wire.endTransmission();
}

// Set the displays brightness. Should receive byte with the value
//  to set the brightness to
//  dimmest------------->brightest
//     0--------127--------255
void setBrightnessI2C(byte value)
{
  Wire.beginTransmission(s7sAddress);
  Wire.write(0x7A);  // Set brightness command byte
  Wire.write(value);  // brightness data byte
  Wire.endTransmission();
}

// Turn on any, none, or all of the decimals.
//  The six lowest bits in the decimals parameter sets a decimal 
//  (or colon, or apostrophe) on or off. A 1 indicates on, 0 off.
//  [MSB] (X)(X)(Apos)(Colon)(Digit 4)(Digit 3)(Digit2)(Digit1)
void setDecimalsI2C(byte decimals)
{
  Wire.beginTransmission(s7sAddress);
  Wire.write(0x77);
  Wire.write(decimals);
  Wire.endTransmission();
}

uint32_t tapTempoTimerMath( uint16_t BPMInput )
{
	uint32_t returnVar = 0;
	
	returnVar = 2500000 /( (uint32_t)BPMInput );
	return returnVar;
}
  



