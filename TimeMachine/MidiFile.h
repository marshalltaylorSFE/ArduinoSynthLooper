//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Fri Nov 26 14:12:01 PST 1999
// Last Modified: Fri Dec  2 13:26:44 PST 1999
// Last Modified: Fri Nov 10 12:13:15 PST 2000 Added some more editing cap.
// Last Modified: Thu Jan 10 10:03:39 PST 2002 Added allocateEvents()
// Last Modified: Mon Jun 10 22:43:10 PDT 2002 Added clear()
// Last Modified: Sat Dec 17 23:11:57 PST 2005 Added millisecond ticks
// Last Modified: Tue Feb  5 11:51:43 PST 2008 Read() set to const char*
// Last Modified: Tue Apr  7 09:23:48 PDT 2009 Added addMetaEvent
// Last Modified: Fri Jun 12 22:58:34 PDT 2009 Renamed SigCollection class
// Last Modified: Thu Jul 22 23:28:54 PDT 2010 Added tick to time mapping
// Last Modified: Thu Jul 22 23:28:54 PDT 2010 Changed _MidiEvent to MidiEvent
// Last Modified: Tue Feb 22 13:26:40 PST 2011 Added write(ostream)
// Last Modified: Mon Nov 18 13:10:37 PST 2013 Added .printHex function.
// Last Modified: Mon Feb  9 14:01:31 PST 2015 Removed FileIO dependency.
// Last Modified: Sat Feb 14 22:35:25 PST 2015 Split out subclasses.
// Filename:      midifile/include/MidiFile.h
// Website:       http://midifile.sapp.org
// Syntax:        C++11
// vim:           ts=3 expandtab
//
// Description:   A class which can read/write Standard MIDI files.
//                MIDI data is stored by track in an array.
//
// Heavily modified Jul 5 2015 by Marshall Taylor
// Removed all references to other headers, now only using MemFile.h
// to fake the memory read/write operations.
//
// Midi data now stored in MicroLL type event lists
//
#ifndef _MIDIFILE_H_INCLUDED
#define _MIDIFILE_H_INCLUDED

#include "MemFile.h"
#include "MicroLL.h"

#define TIME_STATE_DELTA       0
#define TIME_STATE_ABSOLUTE    1

#define TRACK_STATE_SPLIT      0
#define TRACK_STATE_JOINED     1

class MidiFile {
   public:
                MidiFile                  (MemFile& input);
               ~MidiFile                  ();

      // reading/writing functions:
      void       read                      (MemFile& istream);
      int       status                    (void);

      //Event list objects
	  MicroLL track[16];
	  uint8_t nextEmptyTrack;
	  
//*****These will probably need to be filled*****//
      //int       addEvent                  (int aTrack, int aTime, 
      //                                       vector<char>& midiData);
      //int       addEvent                  (MidiEvent& mfevent);
      //int       addMetaEvent              (int aTrack, int aTime, int aType,
      //                                       vector<char>& metaData);
      //int       addMetaEvent              (int aTrack, int aTime, int aType,
      //                                       const char* metaData);
      //int       addPitchBend              (int aTrack, int aTime,
      //                                     int aChannel, double amount);
      //void      erase                     (void);
      //void      clear                     (void);
      //void      clear_no_deallocate       (void);
      //MidiEvent&  getEvent                (int aTrack, int anIndex);



      // static functions:
      uint8_t    readByte                (MemFile& input);
      uint16_t   readLittleEndian2Bytes  (MemFile& input);
      uint32_t    readLittleEndian4Bytes  (MemFile& input);

	  
      int              ticksPerQuarterNote;      // time base of file
      int              trackCount;               // # of tracks in file
      int              theTrackState;            // joined or split
      int              theTimeState;             // absolute or delta
   protected:


   private:
      int        extractMidiData  (MemFile& input, MidiEvent& inputEvent);
      long      readVLValue      (MemFile& inputfile);
      long      unpackVLV        (uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e);

};

#endif /* _MIDIFILE_H_INCLUDED */



