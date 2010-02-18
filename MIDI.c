/*
             LUFA Library
     Copyright (C) Dean Camera, 2009.
	  Modified by Alex norman 2010 for the MIDI MONSTER project
              
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
 *  Main source file for the MIDI demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "MIDI.h"

#define LED_1 PORTC2
#define LED_2 PORTC4
bool on;

/** LUFA MIDI Class driver interface configuration and state information. This structure is
 *  passed to all MIDI Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_MIDI_Device_t Keyboard_MIDI_Interface =
	{
		.Config =
			{
				.StreamingInterfaceNumber = 1,

				.DataINEndpointNumber      = MIDI_STREAM_IN_EPNUM,
				.DataINEndpointSize        = MIDI_STREAM_EPSIZE,
				.DataINEndpointDoubleBank  = false,

				.DataOUTEndpointNumber     = MIDI_STREAM_OUT_EPNUM,
				.DataOUTEndpointSize       = MIDI_STREAM_EPSIZE,
				.DataOUTEndpointDoubleBank = false,
			},
	};

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();

	for (;;)
	{
		MIDI_EventPacket_t ReceivedMIDIEvent;
		if (MIDI_Device_ReceiveEventPacket(&Keyboard_MIDI_Interface, &ReceivedMIDIEvent))
		{
			MIDI_Device_SendEventPacket(&Keyboard_MIDI_Interface, &ReceivedMIDIEvent);
			PORTC ^= _BV(LED_2);
			/*
			if ((ReceivedMIDIEvent.Command == (MIDI_COMMAND_NOTE_ON >> 4)) && (ReceivedMIDIEvent.Data3 > 0))
			  LEDs_SetAllLEDs(ReceivedMIDIEvent.Data2 > 64 ? LEDS_LED1 : LEDS_LED2);
			else
			  LEDs_SetAllLEDs(LEDS_NO_LEDS);
			  */
		}
	
		MIDI_Device_USBTask(&Keyboard_MIDI_Interface);
		USB_USBTask();
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	DDRC |= (_BV(PINC2) | _BV(PINC4));
	PORTC |= (_BV(PINC2) | _BV(PINC4));
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);
	
	/* Hardware Initialization */
	USB_Init();
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	PORTC &= ~(_BV(LED_1));
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	if (!(MIDI_Device_ConfigureEndpoints(&Keyboard_MIDI_Interface))){
		PORTC |= (_BV(PINC2) | _BV(PINC4));
	}
}

/** Event handler for the library USB Unhandled Control Request event. */
void EVENT_USB_Device_UnhandledControlRequest(void)
{
	MIDI_Device_ProcessControlRequest(&Keyboard_MIDI_Interface);
}

///** Sends a MIDI note change event (note on or off) to the MIDI output jack, on the given virtual cable ID and channel.
 //*
 //*  \param Pitch    Pitch of the note to turn on or off
 //*  \param OnOff    Set to true if the note is on (being held down), or false otherwise
 //*  \param CableID  ID of the virtual cable to send the note change to
 //*  \param Channel  MIDI channel number to send the note change event to
 //*/
//void SendMIDINoteChange(const uint8_t Pitch, const bool OnOff, const uint8_t CableID, const uint8_t Channel)
//{
	///* Wait until endpoint ready for more data */
	//while (!(Endpoint_IsReadWriteAllowed()));
//
	///* Check if the message should be a Note On or Note Off command */
	//uint8_t Command = ((OnOff)? MIDI_COMMAND_NOTE_ON : MIDI_COMMAND_NOTE_OFF);
//
	///* Write the Packet Header to the endpoint */
	//Endpoint_Write_Byte((CableID << 4) | (Command >> 4));
//
	///* Write the Note On/Off command with the specified channel, pitch and velocity */
	//Endpoint_Write_Byte(Command | Channel);
	//Endpoint_Write_Byte(Pitch);
	//Endpoint_Write_Byte(MIDI_STANDARD_VELOCITY);
//
	///* Send the data in the endpoint to the host */
	//Endpoint_ClearIN();
//}
//
//void SendMIDICC(const uint8_t num, const uint8_t val, const uint8_t CableID, const uint8_t Channel)
//{
	///* Wait until endpoint ready for more data */
	//while (!(Endpoint_IsReadWriteAllowed()));
//
	///* Check if the message should be a Note On or Note Off command */
	//uint8_t Command = MIDI_COMMAND_CC;
//
	///* Write the Packet Header to the endpoint */
	//Endpoint_Write_Byte((CableID << 4) | (Command >> 4));
//
	//Endpoint_Write_Byte(Command | Channel);
	//Endpoint_Write_Byte(num);
	//Endpoint_Write_Byte(val);
//
	///* Send the data in the endpoint to the host */
	//Endpoint_ClearIN();
//}
