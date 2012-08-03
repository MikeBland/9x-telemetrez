/*
 *  Author - Hank B <gohsthb@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <avr/io.h>
#include <avr/interrupt.h>

// Frsky side is USART0
// 9x side is USART1

#define switch_PORT PORTA
#define switch_DDR DDRA
#define switch_PIN PINA
#define switch_PUE PUEA
#define AIL_sw 1
#define THR_sw 0

#define lowPinDDR DDRA
#define lowPinPORT PORTA
#define lowPinPIN PINA
#define lowPinPUE PUEA
#define IO1 6 // PA6
#define IO2 5 // PA5
#define IO3 4 // PA4
#define IO4 3 // PA3
#define IO5 2 // PA2

#define highPinDDR DDRC
#define highPinPORT PORTC
#define highPinPIN PINC
#define highPinPUE PUEC
#define IO10 5 // PC5
#define IO11 4 // PC4
#define IO13 2 // PC2
#define IO15 0 // PC0

#define pin16DDR DDRB
#define pin16PORT PORTB
#define pin16PIN PINB
#define pin16PUE PUEB
#define IO16 3 // PB3

// uncomment the following line to enable piggybacked Frsky
// output to a bluetooth module
//#define BLUETOOTH

#define F_CPU 1000000


