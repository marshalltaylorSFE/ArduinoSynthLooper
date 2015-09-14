//Header
#ifndef LOOPERPANEL_H_INCLUDED
#define LOOPERPANEL_H_INCLUDED


#include "stdint.h"
#include "timeKeeper.h"
#include "PanelComponents.h"
#include "TimeCode.h"
#include "Panel.h"


enum PStates
{
	PInit,
	PIdle,
	PFirstRecord,
	PPlay,
	POverdub,

};

class LooperPanel : public Panel
{
public:
	LooperPanel( void );
	void setTapLed( void );
	ledState_t serviceTapLed( void );
	void setSyncLed( void );
	ledState_t serviceSyncLed( void );
	void setTxLed( void );
	ledState_t serviceTxLed( void );
	void setRxLed( void );
	ledState_t serviceRxLed( void );
	void reset( void );
	//State machine stuff  
	void processMachine( void );
	void tickStateMachine( void );
	uint8_t serviceResetTapHead( void );
	uint8_t serviceMarkLength( void );
	uint8_t serviceSongClearRequest( void );
	uint8_t serviceBPMUpdateRequest( void );
	void setTapHeadMessage( BeatCode & );
	void getTrackMute( uint8_t* );
	uint8_t getRecordingTrack( void );
	uint8_t recording;
	uint8_t playing;
	uint16_t BPM;

private:
	//State machine stuff  
	PStates state;

	//Controls
	uint8_t resetTapHeadFlag;
	uint8_t markLengthFlag;
	uint8_t screenControlTap;
	//uint8_t trackNum;
	uint8_t viewingTrack;
	uint8_t recordingTrack;
	uint8_t songClearRequestFlag;
	uint8_t BPMUpdateRequestFlag;
	uint8_t trackMute[16];
	char tapHeadMessage[5];
	
	uint8_t tapLedFlag;
	uint8_t syncLedFlag;
	uint8_t rxLedFlag;
	uint8_t txLedFlag;
	
};

#endif