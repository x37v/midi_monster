//midi for avr chips,
//Copyright 2008 Alex Norman
//writen by Alex Norman with help from the Atmega16 manual (usart) and
//http://www.borg.com/~jglatt/tech/midispec.htm
//
//This file is part of avr-midi.
//
//avr-midi is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//avr-midi is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with avr-midi.  If not, see <http://www.gnu.org/licenses/>.
//

#include "midi.h"
#include <avr/io.h>
#include <avr/interrupt.h>

void midiSendByte(uint8_t inByte){
#if 0
	// Wait for empty transmit buffer
	while ( !(UCSRA & _BV(UDRE)) );
	UDR = inByte;
#endif

	// Wait for empty transmit buffer
	while ( !( UCSR1A & _BV(UDRE1)) );
	UDR1 = inByte;
}

void midiInit(uint16_t clockScale, bool out, bool in){

	// Set baud rate
	UBRR1H = (uint8_t)(clockScale >> 8);
	UBRR1L = (uint8_t)(clockScale & 0xFF);

#if 0
	// Enable transmitter
	if(out)
		UCSRB |= _BV(TXEN);
	if(in) {
		//Enable receiver
		//RX Complete Interrupt Enable  (user must provide routine)
		UCSRB |= _BV(RXEN) | _BV(RXCIE);
	}
	//Set frame format: Async, 8data, 1 stop bit, 1 start bit, no parity
	//needs to have URSEL set in order to write into this reg
	UCSRC = _BV(URSEL) | _BV(UCSZ1) | _BV(UCSZ0);
#endif

	// Enable transmitter
	if(out)
		UCSR1B |= _BV(TXEN1);
	if(in) {
		//Enable receiver
		//RX Complete Interrupt Enable  (user must provide routine)
		UCSR1B |= _BV(RXEN1) | _BV(RXCIE1);
	}

	//Set frame format: Async, 8data, 1 stop bit, 1 start bit, no parity
	UCSR1C =  _BV(UCSZ11) | _BV(UCSZ10);
}

#ifndef MIDI_BASIC

void midiSendCC(uint8_t chan, uint8_t num, uint8_t val){
	//CC Status: 0xB0 to 0xBF where the low nibble is the MIDI channel.
	midiSendByte(MIDI_CC | (chan & MIDI_CHANMASK));
	//CC Data: Controller Num, Controller Val
	midiSendByte(num & 0x7F);
	midiSendByte(val & 0x7F);
}

void midiSendNoteOn(uint8_t chan, uint8_t num, uint8_t vel){
	midiSendByte(MIDI_NOTEON | (chan & MIDI_CHANMASK));
	//Note Data: Note Num, Note Velocity
	midiSendByte(num & 0x7F);
	midiSendByte(vel & 0x7F);
}

void midiSendNoteOff(uint8_t chan, uint8_t num, uint8_t vel){
	midiSendByte(MIDI_NOTEOFF | (chan & MIDI_CHANMASK));
	//Note Data: Note Num, Note Velocity
	midiSendByte(num & 0x7F);
	midiSendByte(vel & 0x7F);
}

void midiSendAfterTouch(uint8_t chan, uint8_t note_num, uint8_t amt){
	midiSendByte(MIDI_AFTERTOUCH | (chan & MIDI_CHANMASK));
	midiSendByte(note_num & 0x7F);
	midiSendByte(amt & 0x7F);
}

//XXX does this work right?
//amt in range -0x2000, 0x1fff
//uAmt should be in range..
//0x0000 to 0x3FFF
void midiSendPitchBend(uint8_t chan, int16_t amt){
	uint16_t uAmt;
	//check range
	if(amt > 0x1fff){
		uAmt = 0x3FFF;
	} else if(amt < -0x2000){
		uAmt = 0;
	} else {
		uAmt = amt + 0x2000;
	}
	midiSendByte(MIDI_PITCHBEND | (chan & MIDI_CHANMASK));
	midiSendByte(uAmt & 0x7F);
	midiSendByte((uAmt >> 7) & 0x7F);
}

void midiSendProgramChange(uint8_t chan, uint8_t num){
	midiSendByte(MIDI_PROGCHANGE | (chan & MIDI_CHANMASK));
	midiSendByte(num & 0x7F);
}

void midiSendChannelPressure(uint8_t chan, uint8_t amt){
	midiSendByte(MIDI_CHANPRESSURE | (chan & MIDI_CHANMASK));
	midiSendByte(amt & 0x7F);
}

void midiSendClock(void){
	midiSendByte(MIDI_CLOCK);
}

void midiSendTick(void){
	midiSendByte(MIDI_TICK);
}

void midiSendStart(void){
	midiSendByte(MIDI_START);
}

void midiSendContinue(void){
	midiSendByte(MIDI_CONTINUE);
}

void midiSendStop(void){
	midiSendByte(MIDI_STOP);
}

void midiSendActiveSense(void){
	midiSendByte(MIDI_ACTIVESENSE);
}

void midiSendReset(void){
	midiSendByte(MIDI_RESET);
}

void midiTCQuaterFrame(uint8_t time){
	midiSendByte(MIDI_TC_QUATERFRAME);
	midiSendByte(time & 0x7F);
}

//XXX is this right?
void midiSendSongPosition(uint16_t pos){
	midiSendByte(MIDI_SONGPOSITION);
	midiSendByte(pos & 0x7F);
	midiSendByte((pos >> 7) & 0x7F);
}

void midiSendSongSelect(uint8_t song){
	midiSendByte(MIDI_SONGSELECT);
	midiSendByte(song & 0x7F);
}

void midiSendTuneRequest(void){
	midiSendByte(MIDI_TUNEREQUEST);
}

void midiInitMergeState(midiMergeState_t * state){
	state->count = 0;
	state->lastStatus = 0;
	state->lastReturn = true;
}

bool midiMerge(uint8_t inByte, midiMergeState_t * state){
	bool ret = false;
	if(inByte & MIDI_STATUSMASK){
		//if it is realtime it is only a one inByte message, should go through but
		//doesn't affect the current state 
		//(>= 0xF8, .. it'll always be less than 0xFF)
		if(inByte >= 0xF8){
			ret = state->lastReturn;
		} else if(inByte == MIDI_TUNEREQUEST || inByte == SYSEX_END){
			//tune request is just 1 inByte
			//sysex end indicates that we're done with a sysex dump
			state->lastStatus = 0;
			ret = true;
		} else {
			if(inByte >= 0xF0)
				state->lastStatus = inByte;
			else 
				state->lastStatus = inByte & ~MIDI_CHANMASK;
			state->count = 0;
		}
	} else {
		switch(state->lastStatus){
			case 0:
				//XXX this should never happen!
				break;
			//one data byte messages
			case MIDI_CHANPRESSURE:
			case MIDI_PROGCHANGE:
			case MIDI_SONGSELECT:
				ret = true;
				state->lastStatus = 0;
				break;
			//two data byte messages
			case MIDI_CC:
			case MIDI_NOTEON:
			case MIDI_NOTEOFF:
			case MIDI_AFTERTOUCH:
			case MIDI_PITCHBEND:
			case MIDI_SONGPOSITION:
			case MIDI_TC_QUATERFRAME:
				if(state->count > 0){
					state->lastStatus = 0;
					ret = true;
				} else
					state->count += 1;
				break;
			default:
				break;
		};
	}
	//store the last return value
	state->lastReturn = ret;
	midiSendByte(inByte);
	return ret;
}

#endif
