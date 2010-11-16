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
#include <util/delay.h>

#define NUM_DIGITAL_INS 4

#define LED_1 PORTC2
#define LED_2 PORTC4

#define SYSEX_STARTS_CONTS 0x40
#define SYSEX_ENDS_IN_2 0x60
#define SYSEX_ENDS_IN_3 0x70

#define SYS_COMMON_1 0x50
#define SYS_COMMON_2 0x20
#define SYS_COMMON_3 0x30

#define TINY_RESET PINB5

#define DDR_SPI DDRB
#define DD_SS PINB0
#define DD_SCK PINB1
#define DD_MOSI PINB2
#define DD_MISO PINB3
#define TINY_SS PINB4

//we have 2 midi devices, the usb one and the serial midi one
MidiDevice midi_device_usb;
MidiDevice midi_device_serial;

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

#include <avr/interrupt.h>

#define MIDI_IN_ISR ISR(USART1_RX_vect)
#define MIDI_IN_GET_BYTE UDR1
#define MIDI_CLOCK_16MHZ_OSC 31

MIDI_IN_ISR {
   uint8_t b = MIDI_IN_GET_BYTE;

   midi_device_input(&midi_device_serial, 1, b, 0, 0);

   if(b & MIDI_STATUSMASK)
      PORTC ^= _BV(LED_1);
}


void midi_send_usb(MidiDevice * device, uint8_t count, uint8_t byte0, uint8_t byte1, uint8_t byte2) {
   //we ignore count because usb midi always sends 4 bytes
   MIDI_EventPacket_t packet;
   packet.CableNumber = 0;
   packet.Command = (byte0 >> 4);
   packet.Data1 = byte0;
   packet.Data2 = byte1;
   packet.Data3 = byte2;
   MIDI_Device_SendEventPacket(&USB_MIDI_Interface, &packet);
   MIDI_Device_Flush(&USB_MIDI_Interface);
}

void midi_init_device_serial(MidiDevice * device) {
   midi_init_device(device);

   uint16_t clockScale = MIDI_CLOCK_16MHZ_OSC;
   UBRR1H = (uint8_t)(clockScale >> 8);
   UBRR1L = (uint8_t)(clockScale & 0xFF);

   // Enable transmitter
   UCSR1B |= _BV(TXEN1);
   //Enable receiver
   //RX Complete Interrupt Enable 
   UCSR1B |= _BV(RXEN1) | _BV(RXCIE1);

   //Set frame format: Async, 8data, 1 stop bit, 1 start bit, no parity
   UCSR1C = _BV(UCSZ11) | _BV(UCSZ10);
}

void midi_send_serial(MidiDevice * device, uint8_t count, uint8_t byte0, uint8_t byte1, uint8_t byte2) {
   uint8_t out[3];
   out[0] = byte0;
   out[1] = byte1;
   out[2] = byte2;
   if (count > 3)
      count = 3;

   uint8_t i;
   for(i = 0; i < count; i++) {
      while ( !( UCSR1A & _BV(UDRE1)) );
      UDR1 = out[i];
   }
}

void midi_merge_usb_to_serial(MidiDevice * device, uint8_t count, uint8_t byte0, uint8_t byte1, uint8_t byte2) {
   midi_send_data(&midi_device_serial, count, byte0, byte1, byte2);
}

void midi_merge_serial_to_usb(MidiDevice * device, uint8_t count, uint8_t byte0, uint8_t byte1, uint8_t byte2) {
   midi_send_data(&midi_device_usb, count, byte0, byte1, byte2);
}

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
   uint8_t i;
   uint8_t digital_in[NUM_DIGITAL_INS];
   bool digital_last[NUM_DIGITAL_INS];

   SetupHardware();

   //init the digital inputs
   for(i = 0; i < NUM_DIGITAL_INS; i++){
      digital_in[i] = 0;
      digital_last[i] = false;
   }

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
      //shift the guys up
      for(i = 0; i < NUM_DIGITAL_INS; i++)
         digital_in[i] = digital_in[i] << 1;

      //read the inputs
      if(PIND & _BV(PIND6))
         digital_in[0] |= 0x1;
      if(PINC & _BV(PINC7))
         digital_in[1] |= 0x1;
      if(PIND & _BV(PIND4))
         digital_in[2] |= 0x1;
      if(PIND & _BV(PIND5))
         digital_in[3] |= 0x1;

      //check the inputs
      for(i = 0; i < NUM_DIGITAL_INS; i++){
         if(digital_in[i] == 0) {
            if(digital_last[i] == true){
               //send on to both midi devices
               midi_send_cc(&midi_device_usb, 15, i, 127);
               midi_send_cc(&midi_device_serial, 15, i, 127);
            }
            digital_last[i] = false;
         } else if (digital_in[i] == 0xFF) {
            if(digital_last[i] == false){
               //send off to both midi devices
               midi_send_cc(&midi_device_usb, 15, i, 0);
               midi_send_cc(&midi_device_serial, 15, i, 0);
            }
            digital_last[i] = true;
         }
      }

      MIDI_EventPacket_t ReceivedMIDIEvent;
      if (MIDI_Device_ReceiveEventPacket(&USB_MIDI_Interface, &ReceivedMIDIEvent)) {
         //to process the usb midi input we first get its packet length and
         //then pass the bytes through our device
         midi_packet_length_t packet_len = midi_packet_length(ReceivedMIDIEvent.Command << 4);
         //TODO SYSEX
         if (packet_len != UNDEFINED)
            midi_device_input(&midi_device_usb, packet_len, 
                  ReceivedMIDIEvent.Data1, ReceivedMIDIEvent.Data2, ReceivedMIDIEvent.Data3);
         //indicate that we got a packet
         PORTC ^= _BV(LED_2);
      }

      //run the processing functions
      midi_process(&midi_device_usb);
      midi_process(&midi_device_serial);

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

   /* Hardware Initialization */
   USB_Init();

   //initialize our midi devices
   midi_init_device(&midi_device_usb);
   midi_init_device_serial(&midi_device_serial);

   //set our output funcs
   midi_device_set_send_func(&midi_device_usb, midi_send_usb);
   midi_device_set_send_func(&midi_device_serial, midi_send_serial);

   //set our catchall callbacks for echoing
   midi_register_catchall_callback(&midi_device_usb, midi_merge_usb_to_serial);
   midi_register_catchall_callback(&midi_device_serial, midi_merge_serial_to_usb);

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


   //set inputs with pullups

   //TIN1
   DDRC &= ~(_BV(PINC7));
   PORTC |= _BV(PINC7);

   //HD1-3
   DDRD &= ~(_BV(PIND6) | _BV(PIND5) | _BV(PIND4));
   PORTD |= (_BV(PIND6) | _BV(PIND5) | _BV(PIND4));
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

