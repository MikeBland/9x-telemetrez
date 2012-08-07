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

    // these never change, so they can be initalized here
    SwitchBuf[0] = 0x1B; // switches escape character
    SwitchBuf[1] = 0x01; // number of bytes in packet
    SwitchBuf[3] = 0x1B;

    sendSwitchesCount = systemMillis + 3;
    lastPPMchange = systemMillis + 10; // 50ms into the future

    while(1) {
    // send switch values every 20ms
        if(sendSwitchesCount < systemMillis) {
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
        if((lastPPMchange + 4) < systemMillis) { // it has been > 20 ms since last change
            cli(); // stop interrupts
            // stop Tx and Rx
            UCSR0B &= ~((1<<RXEN0)|(1<<TXEN0)|(1<<UDRIE0)|(1<<RXCIE0)); // disable interrupts
            UCSR1B &= ~((1<<RXEN1)|(1<<TXEN1)|(1<<UDRIE1)|(1<<RXCIE1)); // turn off Tx and Rx
            // need power cycle after programming to come back
            while(1); // endless loop
        }
    // calibrate internal oscillator from PPM sync pulse
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

            if(++clockUpdateCount == 8) { // don't change the clock so often
                //PPMpulseTime @ 1MHZ pulse time is equal to the pulse length in us
                uint8_t error = s_Filt % 50;
                if(error != 25) { // because if it is 25 we don't know which way to make the correction
                    if(error > 25) { // clock is running slow
                        if(OSCCAL0 < (255-error)) // don't want to wrap around
                            OSCCAL0 += 50 - error; // increasing OSCCAL will speed up the oscillator
                    } else {
                        // clock is fast so slow if down
                        if(OSCCAL0 > error) // don't want to wrap around
                          OSCCAL0 -= error;
                    }
                }
            }
        }
    // forward packet to 9x
        if(flags.FrskyRxBufferReady) { 
            if(NinexTx_RB.bytesFree() > 19) {  // wait for buffer to have free space
                for(int i=0; i < numPktBytesFrsky; i++) { // add new packet to Tx buffer
                  NinexTx_RB.push(FrskyRxBuf[i]);
                }
                cli();
                UCSR1B |= (1<<UDRIE1); // enable interrupt to send bytes
                flags.FrskyRxBufferReady = 0; // Signal Rx buffer is ok to receive
                sei(); // enable interrupts
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

