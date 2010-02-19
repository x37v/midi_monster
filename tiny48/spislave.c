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

#include "spislave.h"
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <stdbool.h>

#include <LUFA/Version.h>
#include <LUFA/Common/Common.h>
//#include <LUFA/Drivers/Peripheral/SPI.h>

#define DDR_SPI DDRB
#define DD_MISO PINB4

int main(void)
{
	SetupHardware();
	uint8_t cnt = 0;

	while(1) {
		SPDR = cnt;
		while(!(SPSR & (1<<SPIF)));
	}

}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	//set up slave
	
	/* Set MISO output, all others input */
	DDR_SPI = (1<<DD_MISO);
	/* Enable SPI */
	SPCR = (1<<SPE);

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

}

