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
// The Frsky Rx and decode ISR functions below were taken from the Er9x project

#include "telemetrEZ.h"
#include <avr/wdt.h>

// *** Buffers ***
#define BufSize 20
volatile uint8_t FrskyRxBuf[BufSize];
volatile uint8_t NinexRxBuf[BufSize];
volatile uint8_t SwitchBuf[4];
volatile uint8_t numPktBytesFrsky = 0;
volatile uint8_t numPktBytes9x = 0;
uint32_t sendSwitchesCount;
volatile uint32_t lastPPMchange = 0;
volatile uint32_t PPMpulseTime;
volatile uint32_t systemMillis = 0;
uint8_t sendTo9xEnable = 0; // It is ok to send packets to the 9x

volatile ring_buffer FrskyTx_RB; // ring buffers for the pass thru
volatile ring_buffer NinexTx_RB;

volatile flgRegs flags;
uint8_t clockUpdateCount=0;

extern void setup(void);

int main() {
#if F_CPU == 8000000
    CCP = 0xD8; // Unlock protected IO signature
    CLKPR = 0; // run at 8MHz
#endif
    flags.sendSwitches = 1; // the very first thing it will do
                            // is send the switch states to the 9x
    setup();

//    WDTCSR |= (1<<WDP3)|(1<<WDP0);  // set 64ms timeout for watchdog
//    WDTCSR |= (1<<WDE);  // enable the watchdog

    // these never change, so they can be initalized here
    SwitchBuf[0] = 0x1B; // switches escape character
    SwitchBuf[1] = 0x01; // number of bytes in packet
    SwitchBuf[3] = 0x1B;

    sendSwitchesCount = systemMillis + 3;
    lastPPMchange = systemMillis + 1000; // 5s into the future

    while(1) {
//	wdt_reset(); // reset the watchdog timer
    // send switch values every 20ms
        if(sendTo9xEnable && (sendSwitchesCount < systemMillis)) {
            sendSwitchesCount += 4; // send every 20ms
            uint8_t tmp = 0b11000000; // setup for sending
            if(bit_is_clear(switch_PIN, AIL_sw)) // switch is active
                tmp &= ~(1<<7); // clear the bit
            if(bit_is_clear(switch_PIN, THR_sw)) // switch is active
                tmp &= ~(1<<6); // clear the bit
            while(flags.switchto9x);  // wait for current transmission to complete
            cli();
            flags.sendSwitches = 0;
            // add 0x1B 0x01 tmp to 9x Tx buffer
            SwitchBuf[2] = tmp; // this is the only byte that changes
                                // the others are set before the main loop
            flags.switchto9x = 1; // signal that a switch packet is ready
            UCSR1B |= (1<<UDRIE1); // enable interrupt to send bytes
            sei(); // enable interrupts
        }
    // check if ppm stream is active, stop if PPM lost
        if((lastPPMchange + 6) < systemMillis) { // it has been > 30 ms since last change
            // stop Tx to 9x
            UCSR1B &= ~((1<<TXEN1)|(1<<UDRIE1)); // turn off Tx to 9x
            // need power cycle after programming to come back
            lowPinPORT |= (1<<IO3); // this pin will go high if it thinks the 9x is being programmed
            sendTo9xEnable = 0; // disable sending to 9x side
            NinexTx_RB.clear(); // clear the buffer
        } else {
            // ppm is back, reenable Tx to 9x
            sendTo9xEnable = 1;
            UCSR1B |= (1<<TXEN1); // reenable the Tx
            lowPinPORT &= ~(1<<IO3);
        }
            
    // need to wait until the pulse stream actually start before we try making adjustments
    // that is about 5 seconds with the splash screen enabled
        if(!(flags.captureStarted) && (systemMillis > 1000)) {
            // set up timer 1 for input capture
            TIMSK |= (1<<ICIE1); // enable interrupt
            TCCR1B |= (1<<ICES1)|(1<<CS10); // rising edge interrupt, start timer
            cli();
            flags.captureStarted = 1;
            sei();
        }
#ifdef CLOCK_ADJUST
    // calibrate internal oscillator from PPM sync pulse
    // NOTE: osccal register has a much wider adjustment range than I thought
    // the oscillator should change ~33kHz per LSB
        if(flags.ppmReady) {
            cli();
            flags.ppmReady = 0;
            sei();

            // filter algorithm from Er9x
            static uint16_t t;
            static uint16_t s_Filt;
            s_Filt = (s_Filt/2 + t) & 0xFFFE; //gain of 2 on last result - clear last bit
            t = (t + t) >> 1;
            t = (t + PPMpulseTime) >> 1;
            // end filter algorithm

            if(++clockUpdateCount == 32) { // don't change the clock so often
                clockUpdateCount = 0; // reset counter
                //PPMpulseTime @ 1MHZ pulse time is equal to the pulse length in us
#if F_CPU == 8000000
                uint8_t error = (s_Filt % 50) / 8;
#else
                uint8_t error = s_Filt % 50;
#endif
                if((error > 3) && (error < 47)) { // 3 is approx. 1%
                    if(error != 25) { // because if it is 25 we don't know which way to make the correction
                     if(error > 25) { // clock is running slow
                         if(OSCCAL0 < 255) // don't want to wrap around
                           OSCCAL0++; // increasing OSCCAL will speed up the oscillator
                     } else {
                      // clock is fast so slow if down
                         if(OSCCAL0 > 0) // don't want to wrap around
                           OSCCAL0--;
                     } // end error > 25
                    }   // end error != 25
                } // end error within range
            }   // end clock update
        }   // end flags.ppmReady
#endif

    // forward packet to 9x
        if(flags.FrskyRxBufferReady) {
            if(sendTo9xEnable) {
                if(NinexTx_RB.bytesFree() > 19) {  // wait for buffer to have free space
                    for(int i=0; i < numPktBytesFrsky; i++) { // add new packet to Tx buffer
                      NinexTx_RB.push(FrskyRxBuf[i]);
                    }
                    cli();
                    UCSR1B |= (1<<UDRIE1); // enable interrupt to send bytes
                    flags.FrskyRxBufferReady = 0; // Signal Rx buffer is ok to receive
                    sei(); // enable interrupts
                }
            } else { // cannot send to 9x, just dump the packet
                cli();
                flags.FrskyRxBufferReady = 0; // Signal Rx buffer is ok to receive
                sei();
            }
        }

    // forward packet to Frsky module
        if(flags.NinexRxBufferReady) { 
            if(FrskyTx_RB.bytesFree() > 19) {  // wait for buffer to have free space
                for(int i=0; i < numPktBytes9x; i++) { // add new packet to Tx buffer
                  FrskyTx_RB.push(NinexRxBuf[i]);
                }
                cli();
                UCSR0B |= (1<<UDRIE0); // enable interrupt to send bytes
                flags.NinexRxBufferReady = 0; // Signal Rx buffer is ok to receive
                sei(); // enable interrupts
            }
        }

    // what to do with our packet
        if(flags.PktReceived9x) { 
            // nothing to do yet . . .
            // Should be something to configure the 10 I/Os
            // and packets to set the output state or read an input
            cli();
            flags.PktReceived9x = 0;
            sei();
        }
        lowPinPORT ^= (1<<IO2); // timing test for main
    // sleep to save energy here
        // by default sleep mode is idle
        MCUCR |= (1<<SE); // enable sleep
        __asm__ __volatile__ ( "sleep" "\n\t" :: );
        MCUCR &= ~(1<<SE); // disable sleep */
    } // end while(1)
} // end main

