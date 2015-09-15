#ifndef PANEL_H
#define PANEL_H

#include "PanelComponents.h"
#include "sx1509Panel_Hardcode.h"

class Panel
{
public:
	Panel( void );
	void update( void );
	void init( void );
	void toggleFlasherState( void );
	void toggleFastFlasherState( void );

	PanelButton tapButton;
	PanelButton syncButton;
	PanelButton playButton;
	PanelButton stopButton;
	PanelLed recordLed;
	PanelLed playLed;
	PanelLed overdubLed;
	PanelLed tapLed;
	PanelLed syncLed;
	PanelSelector quantizeSelector;
	PanelLed rxLed;
	PanelLed txLed;

	PanelButton songUpButton;
	PanelButton songDownButton;
	PanelButton trackUpButton;
	PanelButton trackDownButton;
	PanelButton option1Button;
	PanelButton option2Button;
	PanelButton option3Button;
	PanelButton option4Button;
	PanelLed option1Led;
	PanelLed option2Led;
	PanelLed option3Led;
	PanelLed option4Led;
	
	sx1509HardCode expanderA;
	
	char tempString[10];  // Will be used with sprintf to create strings
	sSDisplay leftDisplay;
	sSDisplay rightDisplay;

	
	int8_t songNum;
	int8_t trackNum;
protected:
private:
	volatile uint8_t flasherState;
	volatile uint8_t fastFlasherState;

};

#endif // PANEL_H



