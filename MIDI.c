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
#include "avr-midi/midi.h"

#define LED_1 PORTC2
#define LED_2 PORTC4

#define SYSEX_STARTS_CONTS 0x40
#define SYSEX_ENDS_IN_2 0x60
#define SYSEX_ENDS_IN_3 0x70

#define SYS_COMMON_1 0x50
#define SYS_COMMON_2 0x20
#define SYS_COMMON_3 0x30

#define MIDI_REALTIME 0xF0

#define DDR_SPI DDRB
#define DD_SCK PINB1
#define DD_MOSI PINB2
#define DD_MISO PINB3
#define TINY_SS PINB4

/** LUFA MIDI Class driver interface configuration and state information. This structure is
 *  passed to all MIDI Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_MIDI_Device_t USB_MIDI_Interface =
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


	//THIS DOESNT WORK YET
	//select the tiny select
	PORTB &= ~_BV(TINY_SS);

	while(1){

		//send bogus data
		SPDR = 0;
		/* Wait for reception complete */
		while(!(SPSR & (1<<SPIF)));
		uint8_t val =  SPDR;

		SendUSBMIDICC( &USB_MIDI_Interface, 0, val, 0, 0);
		MIDI_Device_USBTask(&USB_MIDI_Interface);
		USB_USBTask();
	}

	for (;;)
	{
		MIDI_EventPacket_t ReceivedMIDIEvent;
		if (MIDI_Device_ReceiveEventPacket(&USB_MIDI_Interface, &ReceivedMIDIEvent)) {

			//parse incoming data and echo to the hardware midi [for now]
			switch(ReceivedMIDIEvent.Command << 4){
				//length 3 messages
				case MIDI_CC:
				case MIDI_NOTEON:
				case MIDI_NOTEOFF:
				case MIDI_PITCHBEND:
				case MIDI_AFTERTOUCH:
				case SYSEX_STARTS_CONTS:
				case SYSEX_ENDS_IN_3:
				case SYS_COMMON_3:
					midiSendByte(ReceivedMIDIEvent.Data1);
					midiSendByte(ReceivedMIDIEvent.Data2);
					midiSendByte(ReceivedMIDIEvent.Data3);
					break;
					//length 2 messages
				case MIDI_CHANPRESSURE:
				case MIDI_PROGCHANGE:
				case SYSEX_ENDS_IN_2:
				case SYS_COMMON_2:
					midiSendByte(ReceivedMIDIEvent.Data1);
					midiSendByte(ReceivedMIDIEvent.Data2);
					break;
					//length 1 messages
				case MIDI_REALTIME:
				case SYS_COMMON_1:
					midiSendByte(ReceivedMIDIEvent.Data1);
					break;
				default:
					break;
			}

			//indicate that we got a packet
			PORTC ^= _BV(LED_2);
		}
	
		MIDI_Device_USBTask(&USB_MIDI_Interface);
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

	//set up hardware midi
	midiInit(MIDI_CLOCK_16MHZ_OSC, true, false);

	//spi
	/* Set MOSI and SCK output and chip select, all others input */
	DDR_SPI = (1<<DD_MOSI) | (1<<DD_SCK) | _BV(TINY_SS);

	/* Enable SPI, Master, set clock rate fck/16 */
	SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0);


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
	if (!(MIDI_Device_ConfigureEndpoints(&USB_MIDI_Interface))){
		PORTC |= (_BV(PINC2) | _BV(PINC4));
	}
}

/** Event handler for the library USB Unhandled Control Request event. */
void EVENT_USB_Device_UnhandledControlRequest(void)
{
	MIDI_Device_ProcessControlRequest(&USB_MIDI_Interface);
}

uint8_t SendUSBMIDINote(
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

uint8_t SendUSBMIDICC(
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
