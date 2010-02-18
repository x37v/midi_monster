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
		if (MIDI_Device_ReceiveEventPacket(&Keyboard_MIDI_Interface, &ReceivedMIDIEvent)) {
			//just send it back for now
			MIDI_Device_SendEventPacket(&Keyboard_MIDI_Interface, &ReceivedMIDIEvent);
			//indicate that we got a packet
			PORTC ^= _BV(LED_2);
		}
	
		MIDI_Device_USBTask(&Keyboard_MIDI_Interface);
		USB_USBTask();
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	//set up LEDs
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

uint8_t SendMIDINote(
		USB_ClassInfo_MIDI_Device_t * midi_device,
		const uint8_t pitch, 
		const bool on, 
		const uint8_t channel, 
		const uint8_t velocity, 
		const uint8_t cable_id)
{
	MIDI_EventPacket_t packet;

	uint8_t command = MIDI_COMMAND_NOTE_OFF;
	if(on)
		command = MIDI_COMMAND_NOTE_ON;

	packet.Command = (command >> 4);
	packet.CableNumber = cable_id;

	/* Write the Note On/Off command with the specified channel, pitch and velocity */
	packet.Data1 = command | channel;
	packet.Data2 = pitch;
	packet.Data3 = velocity;

	return MIDI_Device_SendEventPacket(midi_device, &packet);
}

uint8_t SendMIDICC(
		USB_ClassInfo_MIDI_Device_t * midi_device,
		const uint8_t num, 
		const uint8_t val, 
		const uint8_t channel,
		const uint8_t cable_id) {
	MIDI_EventPacket_t packet;

	uint8_t command = MIDI_COMMAND_CC;

	packet.Command = (command >> 4);
	packet.CableNumber = cable_id;
	packet.Data1 = command | channel;
	packet.Data2 = num;
	packet.Data3 = val;

	return MIDI_Device_SendEventPacket(midi_device, &packet);
}
