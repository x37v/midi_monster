/*
             LUFA Library
     Copyright (C) Dean Camera, 2009.
	  Modified by Alex Norman 2010 for the MIDI MONSTER project
              
  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com
*/

/*
  Copyright 2009  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, and distribute this software
  and its documentation for any purpose and without fee is hereby
  granted, provided that the above copyright notice appear in all
  copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Header file for AudioOutput.c.
 */
 
#ifndef _AUDIO_OUTPUT_H_
#define _AUDIO_OUTPUT_H_

	/* Includes: */
		#include <avr/io.h>
		#include <avr/wdt.h>
		#include <avr/power.h>
		#include <stdbool.h>

		#include "Descriptors.h"
				
		#include <LUFA/Version.h>
		#include <LUFA/Drivers/USB/USB.h>
		#include <LUFA/Drivers/USB/Class/MIDI.h>
		
	/* Function Prototypes: */
		void SetupHardware(void);
		
		void EVENT_USB_Device_Connect(void);
		void EVENT_USB_Device_Disconnect(void);
		void EVENT_USB_Device_ConfigurationChanged(void);
		void EVENT_USB_Device_UnhandledControlRequest(void);

		uint8_t SendMIDINote(
				USB_ClassInfo_MIDI_Device_t * midi_device,
				const uint8_t pitch, 
				const bool on, 
				const uint8_t channel, 
				const uint8_t velocity, 
				const uint8_t cable_id);

		uint8_t SendMIDICC(
				USB_ClassInfo_MIDI_Device_t * midi_device,
				const uint8_t num, 
				const uint8_t val, 
				const uint8_t channel,
				const uint8_t cable_id);

		/* Macros: */
		/** MIDI command for a note on (activation) event */
		#define MIDI_COMMAND_NOTE_ON         0x90
		
		/** MIDI command for a note off (deactivation) event */
		#define MIDI_COMMAND_NOTE_OFF        0x80
		
		#define MIDI_COMMAND_CC         0xB0
		
		/** Standard key press velocity value used for all note events, as no pressure sensor is mounted */
		#define MIDI_STANDARD_VELOCITY       64
		
#endif
