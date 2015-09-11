//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Fri Nov 26 14:12:01 PST 1999
// Last Modified: Fri Dec  2 13:26:29 PST 1999
// Last Modified: Wed Dec 13 10:33:30 PST 2000 Modified sorting routine
// Last Modified: Tue Jan 22 23:23:37 PST 2002 Allowed reading of meta events
// Last Modified: Tue Nov  4 23:09:19 PST 2003 Adjust noteoff in eventcompare
// Last Modified: Tue Jun 29 09:43:10 PDT 2004 Fixed end-of-track problem
// Last Modified: Sat Dec 17 23:11:57 PST 2005 Added millisecond ticks
// Last Modified: Thu Sep 14 20:07:45 PDT 2006 Added SMPTE ASCII printing
// Last Modified: Tue Apr  7 09:23:48 PDT 2009 Added addMetaEvent.
// Last Modified: Fri Jun 12 22:58:34 PDT 2009 Renamed SigCollection class.
// Last Modified: Mon Jul 26 13:38:23 PDT 2010 Added timing in seconds.
// Last Modified: Tue Feb 22 13:26:40 PST 2011 Added write(ostream).
// Last Modified: Mon Nov 18 13:10:37 PST 2013 Added .printHex function.
// Last Modified: Mon Feb  9 12:22:18 PST 2015 Remove dep. on FileIO class.
// Last Modified: Sat Feb 14 23:40:17 PST 2015 Split out subclasses.
// Last Modified: Wed Feb 18 20:06:39 PST 2015 Added binasc MIDI read/write.
// Last Modified: Thu Mar 19 13:09:00 PDT 2015 Improve Sysex read/write.
// Filename:      midifile/src/MidiFile.cpp
// Website:       http://midifile.sapp.org
// Syntax:        C++11
// vim:           ts=3 expandtab hlsearch
//
// Description:   A class which can read/write Standard MIDI files.
//                MIDI data is stored by track in an array.  This
//                class is used for example in the MidiPerform class.
//
#define MIDIFILE_DEBUG

#include "MidiFile.h"
//#include "Binasc.h"
#include "MemFile.h"
#include "MicroLL.h"
#include "Arduino.h"

//////////////////////////////
//
// MidiFile::MidiFile -- Constuctor.
//



MidiFile::MidiFile(MemFile& input) {
	ticksPerQuarterNote = 120;            // TQP time base of file
	trackCount = 1;                       // # of tracks in file
	theTrackState = TRACK_STATE_SPLIT;    // joined or split
	theTimeState = TIME_STATE_DELTA;      // absolute or delta
	nextEmptyTrack = 0;
	read(input);
}



//////////////////////////////
//
// MidiFile::~MidiFile -- Deconstructor.
//

MidiFile::~MidiFile()
{

}


///////////////////////////////////////////////////////////////////////////
//
// reading functions --
//

//////////////////////////////

void MidiFile::read(MemFile& input)
{
   
   if (input.peek() != 'M')
   {
      // // If the first byte in the input stream is not 'M', then presume that
      // // the MIDI file is in the binasc format which is an ASCII representation
      // // of the MIDI file.  Convert the binasc content into binary content and
      // // then continue reading with this function.
      // stringstream binarydata;
      // Binasc binasc;
      // binasc.writeToBinary(binarydata, input);
      // binarydata.seekg(0, ios_base::beg);
      // if (binarydata.peek() != 'M') {
         // //cerr << "Bad MIDI data input" << endl;
	 // rwstatus = 0;
         // return;
      // } else {
         // rwstatus = read(binarydata);
         // return;
      // }
   }

   //const char* filename = getFilename();

   int    character;
   // char  buffer[123456] = {0};
   long  longdata;
   short shortdata;


   // Read the MIDI header (4 bytes of ID, 4 byte data size,
   // anticipated 6 bytes of data.

   character = input.get();
   if (input.eof()) {
      //cerr << "In file " << filename << ": unexpected end of file." << endl;
      //cerr << "Expecting 'M' at first byte, but found nothing." << endl;
      return;
   } else if (character != 'M') {
      //cerr << "File " << filename << " is not a MIDI file" << endl;
      //cerr << "Expecting 'M' at first byte but got '"
      //     << character << "'" << endl;
      return;
   }

   character = input.get();
   if (input.eof()) {
      //cerr << "In file " << filename << ": unexpected end of file." << endl;
      //cerr << "Expecting 'T' at first byte, but found nothing." << endl;
      return;
   } else if (character != 'T') {
      //cerr << "File " << filename << " is not a MIDI file" << endl;
      //cerr << "Expecting 'T' at first byte but got '"
      //     << character << "'" << endl;
      return;
   }

   character = input.get();
   if (input.eof()) {
      //cerr << "In file " << filename << ": unexpected end of file." << endl;
      //cerr << "Expecting 'h' at first byte, but found nothing." << endl;
      return;
   } else if (character != 'h') {
      //cerr << "File " << filename << " is not a MIDI file" << endl;
      //cerr << "Expecting 'h' at first byte but got '"
      //     << character << "'" << endl;
      return;
   }

   character = input.get();
   if (input.eof()) {
      //cerr << "In file " << filename << ": unexpected end of file." << endl;
      //cerr << "Expecting 'd' at first byte, but found nothing." << endl;
      return;
   } else if (character != 'd') {
      //cerr << "File " << filename << " is not a MIDI file" << endl;
      //cerr << "Expecting 'd' at first byte but got '"
      //     << character << "'" << endl;
      return;
   }

   // read header size (allow larger header size?)
   longdata = MidiFile::readLittleEndian4Bytes(input);
#ifdef MIDIFILE_DEBUG
   Serial.println(longdata);
#endif
   if (longdata != 6) {
      //cerr << "File " << filename
      //     << " is not a MIDI 1.0 Standard MIDI file." << endl;
      //cerr << "The header size is " << longdata << " bytes." << endl;
      return;
   }

   // Header parameter #1: format type
   int type;
   shortdata = MidiFile::readLittleEndian2Bytes(input);
#ifdef MIDIFILE_DEBUG
   Serial.println(shortdata);
#endif
   switch (shortdata) {
      case 0:
         type = 0;
         break;
      case 1:
         type = 1;
         break;
      case 2:    // Type-2 MIDI files should probably be allowed as well.
      default:
         //cerr << "Error: cannot handle a type-" << shortdata
         //     << " MIDI file" << endl;
         return;
   }

   // Header parameter #2: track count
   int tracks;
   shortdata = MidiFile::readLittleEndian2Bytes(input);
   trackCount = shortdata;
#ifdef MIDIFILE_DEBUG
   Serial.println(shortdata);
#endif
   if (type == 0 && shortdata != 1) {
      //cerr << "Error: Type 0 MIDI file can only contain one track" << endl;
      //cerr << "Instead track count is: " << shortdata << endl;
      return;
   } else {
      tracks = shortdata;
   }

   for (int z=0; z<tracks; z++)
   {
       //track init loop.

   }

   // Header parameter #3: Ticks per quarter note
   shortdata = MidiFile::readLittleEndian2Bytes(input);
#ifdef MIDIFILE_DEBUG
   uint16_t twoBytes = (uint16_t)shortdata;
   Serial.print("0x");
   Serial.println(twoBytes,HEX);
#endif
   if (shortdata >= 0x8000) {
      int framespersecond = ((!(shortdata >> 8))+1) & 0x00ff;
      int resolution      = shortdata & 0x00ff;
      switch (framespersecond) {
         case 232:  framespersecond = 24; break;
         case 231:  framespersecond = 25; break;
         case 227:  framespersecond = 29; break;
         case 226:  framespersecond = 30; break;
         default:
               //cerr << "Warning: unknown FPS: " << framespersecond << endl;
               framespersecond = 255 - framespersecond + 1;
               //cerr << "Setting FPS to " << framespersecond << endl;
      }
      // actually ticks per second (except for frame=29 (drop frame)):
      ticksPerQuarterNote = shortdata;

      //cerr << "SMPTE ticks: " << ticksPerQuarterNote << " ticks/sec" << endl;
      //cerr << "SMPTE frames per second: " << framespersecond << endl;
      //cerr << "SMPTE frame resolution per frame: " << resolution << endl;
   }  else {
      ticksPerQuarterNote = shortdata;
   }


   //////////////////////////////////////////////////
   //
   // now read individual tracks:
   //


   for (int i=0; i<tracks; i++)
   {

      // cout << "\nReading Track: " << i + 1 << flush;

      // read track header...

      character = input.get();
      if (input.eof()) {
         //cerr << "In file " << filename << ": unexpected end of file." << endl;
         //cerr << "Expecting 'M' at first byte in track, but found nothing."
         //     << endl;
         return;
      } else if (character != 'M') {
         //cerr << "File " << filename << " is not a MIDI file" << endl;
         //cerr << "Expecting 'M' at first byte in track but got '"
         //     << character << "'" << endl;
         return;
      }

      character = input.get();
      if (input.eof()) {
         //cerr << "In file " << filename << ": unexpected end of file." << endl;
         //cerr << "Expecting 'T' at first byte in track, but found nothing."
         //     << endl;
         return;
      } else if (character != 'T') {
         //cerr << "File " << filename << " is not a MIDI file" << endl;
         //cerr << "Expecting 'T' at first byte in track but got '"
         //     << character << "'" << endl;
         return;
      }

      character = input.get();
      if (input.eof()) {
         //cerr << "In file " << filename << ": unexpected end of file." << endl;
         //cerr << "Expecting 'r' at first byte in track, but found nothing."
         //     << endl;
         return;
      } else if (character != 'r') {
         //cerr << "File " << filename << " is not a MIDI file" << endl;
         //cerr << "Expecting 'r' at first byte in track but got '"
         //     << character << "'" << endl;
         return;
      }

      character = input.get();
      if (input.eof()) {
         //cerr << "In file " << filename << ": unexpected end of file." << endl;
         //cerr << "Expecting 'k' at first byte in track, but found nothing."
         //     << endl;
         return;
      } else if (character != 'k') {
         //cerr << "File " << filename << " is not a MIDI file" << endl;
         //cerr << "Expecting 'k' at first byte in track but got '"
         //     << character << "'" << endl;
         return;
      }

      // Now read track chunk size and throw it away because it is
      // not really necessary since the track MUST end with an
      // end of track meta event, and many MIDI files found in the wild
      // do not correctly give the track size.
      longdata = MidiFile::readLittleEndian4Bytes(input);
#ifdef MIDIFILE_DEBUG
	  Serial.println(longdata);
#endif
	  
     char runningCommand;
     //make a temp event
     int absticks;
     int xstatus = 0;
     // int barline;
     char bytes[2];

	 // process the track
      absticks = 0;
      // barline = 1;
      while( xstatus != 2 )
	  {
         MidiEvent tempEvent;

         longdata = readVLValue(input);
#ifdef MIDIFILE_DEBUG
	     Serial.print("VLValue: ");
		 Serial.println(longdata);
#endif
         absticks += longdata;
		 tempEvent.timeStamp = absticks;
		 //Start of new microcode--
		 //  Ok, the time stamp has been removed.  Now function on command
		 //and store in tempEvent
         xstatus = extractMidiData(input, tempEvent);
         if (xstatus == 0)
		 {
#ifdef MIDIFILE_DEBUG
		    Serial.println("FAILED extract");
#endif
             return;
         }

		 
		 //If tempEvent command == EOT, dump your pants
         // //cout << "command = " << hex << (int)event.data[0] << dec << endl;
         // if (bytes[0] == 0xff && (bytes[1] == 1 ||
             // bytes[1] == 2 || bytes[1] == 3 || bytes[1] == 4)) {
           // // mididata.push_back('\0');
           // // cout << '\t';
           // // for (int m=0; m<event.data[2]; m++) {
           // //    cout << event.data[m+3];
           // // }
           // // cout.flush();
         // } else if (bytes[0] == 0xff && bytes[1] == 0x2f) {
            // // end of track message

            // break;
         // }

		 //If tempEvent command == OTHER, push it
		 switch(i)
		 {
		 case 0:
			track[0].pushObject(tempEvent);
			break;
		 case 1:
			break;
		 default:
			break;
		 }


      }

   }

   theTimeState = TIME_STATE_ABSOLUTE;
   //return 1;
}



//////////////////////////////
//
// MidiFile::extractMidiData -- Extract MIDI data from input
//    stream.  Return value is 0 if failure; otherwise, returns 1.
//

int MidiFile::extractMidiData(MemFile& input, MidiEvent& inputEvent) {

   uint8_t readByte;
   uint8_t firstByte;
   uint8_t byte;

   readByte = input.get();
   if (input.eof()) {
      //cerr << "Error: unexpected end of file." << endl;
      return 0;
   } else {
      firstByte = readByte;
	  inputEvent.eventType = firstByte & 0xF0;
   }

   switch (firstByte & 0xF0) {
      case 0x80:        // note off (2 more bytes)
      case 0x90:        // note on (2 more bytes)
      case 0xA0:        // aftertouch (2 more bytes)
      case 0xB0:        // cont. controller (2 more bytes)
      case 0xE0:        // pitch wheel (2 more bytes)
         byte = input.get();
		 inputEvent.value = byte;
		 byte = input.get();
		 inputEvent.data = byte;
		 //inputEvent.channel = firstByte & 0x0F;
         break;
      case 0xC0:        // patch change (1 more byte)
      case 0xD0:        // channel pressure (1 more byte)
	     byte = input.get();//Meta type
         break;
      case 0xF0:
	  //Meta
	  byte = input.get();//Meta type
 	  readByte = input.get();//Meta length
#ifdef MIDIFILE_DEBUG
	  Serial.println(readByte, HEX);
#endif
	  //Dump the packet
	  for(int i = 0; i < readByte; i++)
	  {
	    input.get();

	  }
	  if(byte == 0x2F)//Do next track
	  {
	    return 2;
	  }

	  break;
      default:
         //cout << "Error reading midifile" << endl;
         //cout << "Command byte was " << (int)runningCommand << endl;
         return 0;
   }
   return 1;
}

//Serial.print("*0x");
//Serial.println(b[i], HEX);


//////////////////////////////
//
// MidiFile::readVLValue -- The VLV value is expected to be unpacked into
//   a 4-byte integer, so only up to 5 bytes will be considered.
//

long MidiFile::readVLValue(MemFile& input) {
   uint8_t b[5] = {0};

   for (int i=0; i<5; i++) {
      b[i] = input.get();
      if (b[i] < 0x80) {
         break;
      }
   }

   return unpackVLV(b[0], b[1], b[2], b[3], b[4]);
}



//////////////////////////////
//
// MidiFile::unpackVLV -- converts a VLV value to an unsigned long value.
// default values: a = b = c = d = e = 0;
//

long MidiFile::unpackVLV(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e) {
   if (e > 0x7f) {
      //cerr << "Error: VLV value was too long" << endl;
      exit(1);
   }

   uint8_t bytes[5] = {a, b, c, d, e};
   int count = 0;
   while (bytes[count] > 0x7f && count < 5) {
      count++;
   }
   count++;

   long output = 0;
   for (int i=0; i<count; i++) {
      output = output << 7;
      output = output | (bytes[i] & 0x7f);
   }

   return output;
}



//////////////////////////////
//
// MidiFile::readLittleEndian4Bytes -- Read four bytes which are in
//      little-endian order (smallest byte is first).  Then flip
//      the order of the bytes to create the return value.
//

uint32_t MidiFile::readLittleEndian4Bytes(MemFile& input) {
   uint8_t buffer[4] = {0};
   input.read((uint8_t*)buffer, 4);
   if (input.eof()) {
      //cerr << "Error: unexpected end of file." << endl;
      exit(1);
   }
   return buffer[3] | (buffer[2] << 8) | (buffer[1] << 16) | (buffer[0] << 24);
}



//////////////////////////////
//
// MidiFile::readLittleEndian2Bytes -- Read two bytes which are in
//       little-endian order (smallest byte is first).  Then flip
//       the order of the bytes to create the return value.
//

uint16_t MidiFile::readLittleEndian2Bytes(MemFile& input) {
   uint8_t buffer[2] = {0};
   input.read((uint8_t*)buffer, 2);
#ifdef MIDIFILE_DEBUG
   Serial.println(buffer[0], HEX);
   Serial.println(buffer[1], HEX);
#endif
   if (input.eof()) {
      //cerr << "Error: unexpected end of file." << endl;
      exit(1);
   }
   return buffer[1] | (buffer[0] << 8);
}



//////////////////////////////
//
// MidiFile::readByte -- Read one byte from input stream.  Exit if there
//     was an error.
//

uint8_t MidiFile::readByte(MemFile& input) {
   uint8_t buffer[1] = {0};
   input.read((uint8_t*)buffer, 1);
   if (input.eof()) {
      //cerr << "Error: unexpected end of file." << endl;
      exit(1);
   }
   return buffer[0];
}

