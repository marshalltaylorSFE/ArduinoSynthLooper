#ifndef PANEL_H
#define PANEL_H

#include "PanelComponents.h"
#include <Arduino.h>
//#include "timeKeeper.h"

class Panel
{
public:
  Panel( void );
  void update( void );
  void init( void );
  void timersMIncrement( uint8_t );
  
  PanelButton tapButton;
  PanelButton syncButton;
  PanelButton songUpButton;
  PanelButton songDownButton;
  PanelButton trackUpButton;
  PanelButton trackDownButton;
  PanelButton playButton;
  PanelButton stopButton;
  PanelLed recordLed;
  PanelLed playLed;
  PanelLed overdubLed;
  PanelLed tapLed;
  PanelLed syncLed;
  PanelSelector quantizeSelector;
  
protected:
private:
};

#endif // PANEL_H



