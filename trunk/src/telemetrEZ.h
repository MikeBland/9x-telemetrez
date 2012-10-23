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
#ifndef TELEMETREZ_H
#define TELEMETREZ_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include "ringbuffer.h"
#include "I2C_bb.h"

// Frsky side is USART1
// 9x side is USART0

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
#define IO1 6 // PA6 *DEBUG* 20ms toggle output
#define IO2 5 // PA5 *DEBUG* main toggle output
#define IO3 4 // PA4 *DEBUG* goes high is ppm signal is lost
#define IO4 3 // PA3 eeprom
#define IO5 2 // PA2 eeprom

#define highPinDDR DDRC
#define highPinPORT PORTC
#define highPinPIN PINC
#define highPinPUE PUEC
#define IO10 5 // PC5 production test LED
#define IO11 4 // PC4 Rotary Encoder Switch
#define IO13 2 // PC2 Rotary Encoder B
#define IO15 0 // PC0 Rotary Encoder A

#define pin16DDR DDRB
#define pin16PORT PORTB
#define pin16PIN PINB // use this pin as output only, see errata
#define pin16PUE PUEB
#define IO16 3 // PB3 Bluetooth

// uncomment the following line to enable piggybacked Frsky
// output to a bluetooth module
#define BLUETOOTH
// uncomment the following line to use an I2C EEPROM (uses IO4 and IO5)
//#define EEPROM

// rotary encoder A, B get connected to IO13 and IO15, the button gets connected to IO11
#define ROTARYENCODER // enable the use of a rotary encoder

// enables some debugging outputs, basically toggles some pins so you know it is running
// this also includes the production test code to flash an led
#define DEBUG

#define F_CPU 8000000 // CPU speed at run time
#define BAUD 9600  // USART Baud rate
#include <util/setbaud.h> // this will calculate the proper config values

#define PPMinDDR DDRC
#define PPMinPUE PUEC
#define PPMinPIN PINC
#define PPMin 1

#define CLOCK_ADJUST // to use self calibrating clock (uses PPM stream)

struct flgRegs {
    uint8_t sendSwitches:1; // trigger main to send switch states
    uint8_t switchto9x:1;   // currently sending switch states to 9x
    uint8_t PktReceived9x:1;    // packet received from 9x for here, needs processing
    uint8_t FrskyRxBufferReady:1; // Frsky Packet received from Frsky module, forward to 9x
    uint8_t NinexRxBufferReady:1; // Frsky Packet received from 9x, forward to Frsky module
    uint8_t Startup:1; // flag for startup to enable Tx to 9x side
    uint8_t ProdTest:1; // Only run the production test for a short period of time
    uint8_t sendEncoder:1; // if the encoder is connected send it's position
};

#endif
