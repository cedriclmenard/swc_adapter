/* Ford-Sony Head Unit Steering Wheel Controls Integration

Copyright (C) 2016  Cedric Leblond Menard

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <avr/io.h>
#include <avr/interrupt.h>

// Control values
#define MENU 0
#define VOL_UP 1
#define VOL_DWN	2
#define SEEK_UP	3
#define SEEK_DWN 4
#define MUTE 5
#define DISPLAY 6
#define BAND_ESCAPE 7

#define NO_STATE 255
#define TRUE 255
#define FALSE 0

// Lookup table for output resistance value
const uint8_t lookupOutput[8] = {
	3,		// MENU
	41,		// VOL_UP
	61,		// VOL_DWN
	20,		// SEEK_UP
	29,		// SEEK_DWN
	9,		// MUTE
	14,		// DISPLAY
	160		// BAND_ESCAPE
};

volatile uint8_t check_flag = FALSE;
volatile uint8_t curr_state = NO_STATE;	// No state as 255
volatile uint8_t new_state = TRUE;
volatile uint8_t n_state = 0;		// Number of

static inline uint8_t selectOutput(int32_t input) {
	uint8_t output;
	switch (input) {
		case 4:			// Example case
		output = lookupOutput[MENU];
		// Fallthrough
		case 10:
		output = lookupOutput[VOL_UP];
		// Fallthrough
		//case 20:
	}
  return output;   // To remove
}

void spi_init_master() {
  // Set MOSI and SCLK to output
  DDRB |= (1<<PORTB0) | (1<<PORTB2);
  //SPCR |= (1<<SPE) | (1<<MSTR)

  // Set to three wire mode
  USICR |= (1<<USIWM0);
  USICR &= ~_BV(USIWM1);

  // Set clock output to Timer0
  USICR |= _BV(USICS0);
  USICR &= ~_BV(USICS1);
}

void timer1_init() {
  //
  TIMSK |= _BV(OCIE1A);
  // Initialize timer
  TCCR1 |= _BV(CS13) | _BV(CS12);
  TCCR1 &= ~_BV(CS11);
  TCCR1 &= ~_BV(CS10);
  TCCR1 |= _BV(CTC1);

  // Set counter to 0
  TCNT1 = 0;

  OCR1A = 4;  // Count to 4, which is 5 values
  // @ 2ms per tick -> 10ms for 5 counts
}

void adc_init() {
  ADMUX |= _BV(0b0011);		// Set channel to ADC3
	ADMUX |= _BV(ADLAR);		// Set left adjust (8-bit is enough)
	ADCSRA |= _BV(ADEN);		// Set enable ADC
}

/* Start ADC conversion blockingly (wait for end of
conversion) */
static inline void acquire_adc() {
	ADCSRA |= _BV(ADSC);
	while (ADSC);

}

ISR(TIMER1_COMPA_vect) {		// Set to 10 ms flag interval
	check_flag = 255;
}

int main() {
	// MARK: Setup
  cli();    // Stop interrupts
  spi_init_master();
  timer1_init();
	adc_init();
  sei();    // Set interrupts

	// MARK: Main loop
  while (1) {
		if (check_flag) {
			check_flag = 0;

			// Read ADC here
			acquire_adc();
			new_state = selectOutput(ADCL);
			if (curr_state == new_state) {
				n_state++;
				if (n_state >= 4) {
					// Set SPI output to curr_state
          USIDR = curr_state;
				}
			} else {
				curr_state = new_state;
				n_state = 1;
			}
		}
	}
}
