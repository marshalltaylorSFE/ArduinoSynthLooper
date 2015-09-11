//**********************************************************************//
//  BEERWARE LICENSE
//
//  This code is free for any use provided that if you meet the author
//  in person, you buy them a beer.
//
//  This license block is BeerWare itself.
//
//  Written by:  Marshall Taylor
//  Created:  May 26, 2015
//
//**********************************************************************//
#ifndef PANELSTATEMACHINE_H_INCLUDED
#define PANELSTATEMACHINE_H_INCLUDED

#include "stdint.h"
#include "timeKeeper.h"
#include "PanelComponents.h"

enum PStates {
  PIdle,
  PFirstRecord,
  PPlay,
  POverdub,

};

class PanelStateMachine
{
public:
  //Outputs
  char sSDisplay[4];
  ledState_t tapLED;
  ledState_t syncLED;
  ledState_t recordLED;
  ledState_t playLED;
  ledState_t overdubLED;
  
  //Inputs
  uint8_t tapButton;
  uint8_t syncButton;
  uint8_t songUpButton;
  uint8_t songDownButton;
  uint8_t trackUpButton;
  uint8_t trackDownButton;
  uint8_t playButton;
  uint8_t stopButton;
  
  //Controls
  uint8_t resetTapHead;
  uint8_t markLength;
  uint8_t screenControlTap;
  uint8_t recording;
  uint8_t playing;
  uint8_t trackNum;
  uint8_t recordingTrack;
  uint8_t clearAll;
  
  //State machine stuff  
  PStates state;
  
  PanelStateMachine( void );
  void tick( void );
  
};

#endif