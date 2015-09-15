#include "Panel.h"

//This is where PanelComponents are joined to form the custom panel
#define tapButtonPin 4
#define syncButtonPin 5

#define playButtonPin 2
#define stopButtonPin 3
#define recordLedPin A1
#define playLedPin A2
#define overdubLedPin A3
#define tapLedPin A8
#define syncLedPin A9
#define quantizeSelectorPin A0

#define rxLedPin 6
#define txLedPin 7

#define option1ButtonPin 3
#define option2ButtonPin 2
#define option3ButtonPin 0
#define option4ButtonPin 1
#define songUpButtonPin 7
#define songDownButtonPin 6
#define trackUpButtonPin 5
#define trackDownButtonPin 4

#define option1LedPin 8
#define option2LedPin 9
#define option3LedPin 10
#define option4LedPin 11

#define MAXSONG 24
#define MAXTRACK 16

Panel::Panel( void )
{
	songNum = 1;
	trackNum = 1;
	flasherState = 0;
	fastFlasherState = 0;
}

void Panel::init( void )
{
	tapButton.init(tapButtonPin);
	syncButton.init(syncButtonPin);
	playButton.init(playButtonPin);
	stopButton.init(stopButtonPin);
	recordLed.init(recordLedPin, 0, &flasherState, &fastFlasherState);
	playLed.init(playLedPin, 0, &flasherState, &fastFlasherState);
	overdubLed.init(overdubLedPin, 0, &flasherState, &fastFlasherState);
	tapLed.init(tapLedPin, 0, &flasherState, &fastFlasherState);
	syncLed.init(syncLedPin, 0, &flasherState, &fastFlasherState);
	quantizeSelector.init(quantizeSelectorPin, 255, 0 ); //With max and min ranges
	rxLed.init(rxLedPin, 0, &flasherState, &fastFlasherState);
	txLed.init(txLedPin, 0, &flasherState, &fastFlasherState);
	
	songUpButton.init(songUpButtonPin, 1);
	songDownButton.init(songDownButtonPin, 1);
	trackUpButton.init(trackUpButtonPin, 1);
	trackDownButton.init(trackDownButtonPin, 1);
	option1Button.init(option1ButtonPin, 1);
	option2Button.init(option2ButtonPin, 1);
	option3Button.init(option3ButtonPin, 1);
	option4Button.init(option4ButtonPin, 1);
	option1Led.init(option1LedPin, 1, &flasherState, &fastFlasherState);
	option2Led.init(option2LedPin, 1, &flasherState, &fastFlasherState);
	option3Led.init(option3LedPin, 1, &flasherState, &fastFlasherState);
	option4Led.init(option4LedPin, 1, &flasherState, &fastFlasherState);
	
	//SX1509 stuff
	expanderA.pinMode( option1LedPin, OUTPUT, &option1Led );
	expanderA.pinMode( option2LedPin, OUTPUT, &option2Led );
	expanderA.pinMode( option3LedPin, OUTPUT, &option3Led );
	expanderA.pinMode( option4LedPin, OUTPUT, &option4Led );
	expanderA.pinMode( option1ButtonPin, INPUT_PULLUP, &option1Button );
	expanderA.pinMode( option2ButtonPin, INPUT_PULLUP, &option2Button );
	expanderA.pinMode( option3ButtonPin, INPUT_PULLUP, &option3Button );
	expanderA.pinMode( option4ButtonPin, INPUT_PULLUP, &option4Button );
	expanderA.pinMode( songUpButtonPin, INPUT_PULLUP, &songUpButton );
	expanderA.pinMode( songDownButtonPin, INPUT_PULLUP, &songDownButton );
	expanderA.pinMode( trackUpButtonPin, INPUT_PULLUP, &trackUpButton );
	expanderA.pinMode( trackDownButtonPin, INPUT_PULLUP, &trackDownButton );
	expanderA.init( 0x3E );
	
	leftDisplay.init( 0x71, &flasherState, &fastFlasherState );
	rightDisplay.init( 0x30, &flasherState, &fastFlasherState );
	
 	flasherState = 0;
	fastFlasherState = 0;
}

void Panel::update( void )
{
	tapButton.update();
	syncButton.update();
	playButton.update();
	stopButton.update();
	recordLed.update();
	playLed.update();
	overdubLed.update();
	tapLed.update();
	syncLed.update();

	quantizeSelector.update();
	
	rxLed.update();
	txLed.update();

	option1Led.update();
	option2Led.update();
	option3Led.update();
	option4Led.update();

	expanderA.update();
	
	songUpButton.update();
	songDownButton.update();
	trackUpButton.update();
	trackDownButton.update();
	option1Button.update();
	option2Button.update();
	option3Button.update();
	option4Button.update();
	
	leftDisplay.update();
	rightDisplay.update();
}

void Panel::toggleFlasherState( void )
{
	flasherState ^= 0x01;
}

void Panel::toggleFastFlasherState( void )
{
	fastFlasherState ^= 0x01;
}