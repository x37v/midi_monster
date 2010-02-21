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
#include "avr-bytequeue/bytequeue.h"
#include <util/delay.h>

#define MIDIIN_QUEUE_SIZE 64

#define LED_1 PORTC2
#define LED_2 PORTC4

#define SYSEX_STARTS_CONTS 0x40
#define SYSEX_ENDS_IN_2 0x60
#define SYSEX_ENDS_IN_3 0x70

#define SYS_COMMON_1 0x50
#define SYS_COMMON_2 0x20
#define SYS_COMMON_3 0x30

#define MIDI_REALTIME 0xF0

#define TINY_RESET PINB5

#define DDR_SPI DDRB
#define DD_SS PINB0
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
volatile byteQueue_t midiin_queue;
uint8_t _midiin_queue_data[MIDIIN_QUEUE_SIZE];

MIDI_IN_ISR {
	uint8_t b = MIDI_IN_GET_BYTE;
	//just throw it on the queue
	//TODO: test for fail?
	byteQueueEnqueue(&midiin_queue, b);
	if(b & MIDI_STATUSMASK)
		PORTC ^= _BV(LED_1);
}

typedef enum {
	MIDI_MSG_2_BYTES,
	MIDI_MSG_3_BYTES,
	MIDI_MSG_INVALID
} HWMidiMsgType;

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	int8_t midi_bytes_left = -1;
	MIDI_EventPacket_t hardwareToUSBmidiPacket;
	HWMidiMsgType midiInType = MIDI_MSG_INVALID;

	//init the byte queue
	byteQueueInit(&midiin_queue, _midiin_queue_data, MIDIIN_QUEUE_SIZE);
	SetupHardware();

	sei();

#if 0
	//THIS DOESNT WORK YET
	while(1){
		while(!(SPSR & (1<<SPIF))){ ; }
		uint8_t val = SPDR;
		SendUSBMIDICC( &USB_MIDI_Interface, 1, val & 0x7F, 1, 0);
		MIDI_Device_USBTask(&USB_MIDI_Interface);
		USB_USBTask();

		//indicate that we got a packet
		PORTC ^= _BV(LED_2);
	}
	

	while(1){
		//select the tiny select
		PORTB &= ~(_BV(TINY_SS));
		_delay_ms(1);
		_delay_ms(50);

		//send bogus data
		SPDR = 1;
		/* Wait for reception complete */
		while(!(SPSR & (1<<SPIF))){
			;
		}
		uint8_t val = SPDR;

		SendUSBMIDICC( &USB_MIDI_Interface, 1, val & 0x7F, 1, 0);
		MIDI_Device_USBTask(&USB_MIDI_Interface);
		USB_USBTask();

		//indicate that we got a packet
		PORTC ^= _BV(LED_2);

		//select the tiny select
		PORTB |= _BV(TINY_SS);
		_delay_ms(50);
	}
#endif

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

		//check for hardware midi input
		byteQueueIndex_t size = byteQueueLength(&midiin_queue);
		if(size > 0){
			uint8_t index;
			for(index = 0; index < size; index++) {
				uint8_t b = byteQueueGet(&midiin_queue, index);
				//if it is a realtime message we send it immediately
				if(b & 0xF0 == MIDI_REALTIME) {
					if(b == MIDI_TC_QUATERFRAME ||
							b == MIDI_SONGPOSITION ||
							b == MIDI_SONGSELECT) {
						//NOT IMPLEMENTED
					} else {
						MIDI_EventPacket_t realtimePacket;
						realtimePacket.CableNumber = 0;
						realtimePacket.Command = (MIDI_REALTIME >> 4);
						realtimePacket.Data1 = b;
						realtimePacket.Data2 = 0;
						realtimePacket.Data3 = 0;
						MIDI_Device_SendEventPacket(&USB_MIDI_Interface, &realtimePacket);
						MIDI_Device_Flush(&USB_MIDI_Interface);
					}
				} else if(b & MIDI_STATUSMASK){
					hardwareToUSBmidiPacket.Data1 = b;
					hardwareToUSBmidiPacket.Command = (b >> 4);
					hardwareToUSBmidiPacket.CableNumber = 0;
					//if it is a status byte
					switch(b & 0xF0){
						case MIDI_CC:
						case MIDI_NOTEON:
						case MIDI_NOTEOFF:
						case MIDI_AFTERTOUCH:
						case MIDI_PITCHBEND:
							midi_bytes_left = 2;
							midiInType = MIDI_MSG_3_BYTES;
							break;
						case MIDI_PROGCHANGE:
						case MIDI_CHANPRESSURE:
							midi_bytes_left = 1;
							midiInType = MIDI_MSG_2_BYTES;
							break;
						default:
							//SYSEX NOT IMPLEMENTED
							midiInType = MIDI_MSG_INVALID;
							midi_bytes_left = 0;
							break;
					}
				} else if(midi_bytes_left > 0 && midiInType != MIDI_MSG_INVALID) {
					switch(midiInType){
						case MIDI_MSG_3_BYTES:
							if(midi_bytes_left == 2) {
								hardwareToUSBmidiPacket.Data2 = b;
							} else if(midi_bytes_left == 1) {
								//send
								hardwareToUSBmidiPacket.Data3 = b;
								MIDI_Device_SendEventPacket(&USB_MIDI_Interface, &hardwareToUSBmidiPacket);
								MIDI_Device_Flush(&USB_MIDI_Interface);
							} else {
								//error
								midiInType = MIDI_MSG_INVALID;
							}
							midi_bytes_left--;
							break;
						case MIDI_MSG_2_BYTES:
							if(midi_bytes_left == 1) {
								//send
								hardwareToUSBmidiPacket.Data2 = b;
								hardwareToUSBmidiPacket.Data3 = 0;
								MIDI_Device_SendEventPacket(&USB_MIDI_Interface, &hardwareToUSBmidiPacket);
								MIDI_Device_Flush(&USB_MIDI_Interface);
							} else {
								//error
								midiInType = MIDI_MSG_INVALID;
							}
							midi_bytes_left--;
							break;
						default:
							midi_bytes_left = 0;
							//ERROR
							break;
					}
				} 
			}
			byteQueueRemove(&midiin_queue, size);
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
	midiInit(MIDI_CLOCK_16MHZ_OSC, true, true);

	/* Hardware Initialization */
	USB_Init();

	//spi
	//PRR0 &= ~(_BV(PRSPI));
	///* Set SS, MOSI, SCK, TINY_SS, TINY_RESET to output, all others input */
	//DDR_SPI = _BV(DD_SS) | _BV(DD_SCK) | _BV(DD_MOSI) | _BV(DD_SCK) | _BV(TINY_SS) | _BV(TINY_RESET) ;
	
	DDR_SPI = _BV(DD_SS) | _BV(TINY_RESET) | _BV(DD_MISO);

	//reset the tiny
	PORTB &= ~(_BV(TINY_RESET));
	_delay_ms(2);
	//PORTB |= _BV(TINY_RESET);

	/* Enable SPI, Master, set clock rate fck/64 */
	//SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR1);
	//SPCR = (1<<SPE);
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
