//Header
#ifndef LOOPERPANEL_H_INCLUDED
#define LOOPERPANEL_H_INCLUDED


#include "stdint.h"
#include "timeKeeper.h"
#include "PanelComponents.h"
#include "TimeCode.h"
#include "Panel.h"
#include "flagMessaging.h"

enum PStates
{
	PInit,
	PIdle,
	PFirstRecord,
	PPlay,
	POverdub,
	PUndoClearOverdub,
	PUndoDecrement,
	PUndoClearTrack

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
	void setTapHeadMessage( BeatCode & );
	void getTrackMute( uint8_t* );
	uint8_t getRecordingTrack( void );

	void timersMIncrement( uint8_t );

	
	//Flags coming in from the system

	uint8_t tapLedFlag;
	uint8_t syncLedFlag;
	uint8_t rxLedFlag;
	uint8_t txLedFlag;
	
	//Internal - and going out to the system - Flags
	MessagingFlag clearSongFlag;
	MessagingFlag updateBPMFlag;
	MessagingFlag recordingFlag;
	MessagingFlag playingFlag;
	MessagingFlag songHasDataFlag;
	MessagingFlag resetTapHeadFlag;
	MessagingFlag markLengthFlag;
	MessagingFlag lengthEstablishedFlag;
	MessagingFlag clearTrackFlag;
	MessagingFlag sendPanicFlag;
	
	//  ..and data.
	uint8_t trackMute[16];
	uint16_t BPM;
	uint8_t quantizeTrackTicks;
	uint8_t quantizeNoteLengthTicks;
	uint8_t quantizeTicks;
	uint8_t leftDisplayMode;
	uint8_t rightDisplayMode;
	uint8_t trackToClear;
	

private:
	//Internal Flags
	MessagingFlag quantizingTrackFlag;
	MessagingFlag quantizingNoteLengthFlag;
	MessagingFlag quantizeHoldOffFlag;
	//  ..and data
	char tapHeadMessage[5];
	char quantizeMessage[10];
	uint8_t viewingTrack;
	uint8_t recordingTrack;
	

	//State machine stuff  
	PStates state;


	TimeKeeper quantizingTrackTimeKeeper;
	TimeKeeper quantizingNoteLengthTimeKeeper;
	TimeKeeper recordingTapTimeKeeper;
};

#endif