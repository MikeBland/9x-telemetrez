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
volatile uint8_t SwitchBuf[5];
volatile uint8_t numPktBytesFrsky = 0;
volatile uint8_t numPktBytes9x = 0;
uint32_t sendSwitchesCount;
volatile uint32_t lastPPMchange = 0;
volatile uint16_t PPMpulseTime;
volatile uint32_t systemMillis = 0;
uint32_t reenableTimer;
volatile uint8_t sendTo9xEnable = 0; // It is ok to send packets to the 9x
volatile uint8_t ppmReady = 0;

volatile ring_buffer FrskyTx_RB; // ring buffers for the pass thru
volatile ring_buffer NinexTx_RB;

volatile flgRegs flags;
uint8_t clockUpdateCount=0;

#ifdef ROTARYENCODER
// variables for the rotary encoder
volatile uint8_t encoderPinValues[] = {1,1};
volatile uint8_t encoderPosition = 0;
volatile uint8_t intStarted=0;
#endif
#ifdef DEBUG
uint32_t ProdTestMillis = 0;
const uint32_t ProdTestInterval = 25;
const uint32_t ProdTestMax = 1000;
#endif

extern void setup(void);
void rotary_encoder_change(uint8_t changedPin, uint8_t value);

int main() {
    CCP = 0xD8; // Unlock protected IO signature
    CLKPR = 0; // run at 8MHz

    flags.sendSwitches = 1; // the very first thing it will do
                            // is send the switch states to the 9x
    setup();
#ifdef EEPROM
      I2C_Init();  // start I2C bus on pins IO_B and IO_A
#endif
//    WDTCSR |= (1<<WDP3)|(1<<WDP0);  // set 64ms timeout for watchdog
//    WDTCSR |= (1<<WDE);  // enable the watchdog

    // these never change, so they can be initalized here
    SwitchBuf[0] = 0x1B; // switches escape character
    SwitchBuf[1] = 1; // number of bytes in switch packet

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
#ifdef ROTARYENCODER
	    if(flags.sendEncoder) {
	      SwitchBuf[3] = encoderPosition;
	      if( highPinPIN & (1<<IO_I)) // test switch
		SwitchBuf[4] = 0; // button not pressed
	      else
		SwitchBuf[4] = 1;
	    }
#endif
            flags.switchto9x = 1; // signal that a switch packet is ready
            UCSR0B |= (1<<UDRIE0); // enable interrupt to send bytes
            sei(); // enable interrupts
        }
    // check if ppm stream is active, stop if PPM lost
        if(((lastPPMchange + 6) < systemMillis) && (PPMinPIN & (1<<PPMin)) ) { 
            // it has been > 30 ms since last change and the PPM pin is high
            // if the 9x is in simulator or student mode the PPM line will be low
            // stop Tx to 9x
            UCSR0B &= ~((1<<TXEN0)|(1<<UDRIE0)); // turn off Tx to 9x
            // 
            lowPinPORT |= (1<<IO_C); // this pin will go high if it thinks the 9x is being programmed
            sendTo9xEnable = 0; // disable sending to 9x side
            NinexTx_RB.clear(); // clear the buffer
            reenableTimer = systemMillis + 3000ul;
        } else {
            if(!sendTo9xEnable) {
                // ppm is back, reenable Tx to 9x
                if(reenableTimer < systemMillis || !flags.Startup) { // wait 15 seconds before sending to 9x again
                    sendTo9xEnable = 1;
                    flags.Startup = 1;
                    UCSR0B |= (1<<TXEN0)|(1<<UDRIE0); // reenable the Tx
                    sendSwitchesCount = systemMillis + 3;
		    UDR0 = 0xFF; // send a byte to set up transmit complete flag
#ifdef DEBUG
                    lowPinPORT &= ~(1<<IO_C);
#endif
                } // end if reenableTimer
            } // end if !sendTo9xEnable
        }
            
#ifdef CLOCK_ADJUST
    // calibrate internal oscillator from PPM sync pulse
    // NOTE: osccal register has a much wider adjustment range than I thought
    // the oscillator should change ~33kHz per LSB
        if(ppmReady) {
            cli();
            ppmReady = 0;
            sei();

            if(++clockUpdateCount == 32) { // don't change the clock so often
                clockUpdateCount = 0; // reset counter
                //PPMpulseTime @ 1MHZ pulse time is equal to the pulse length in us
                cli(); // protect from changing in interrupt
                uint16_t pulse = PPMpulseTime;
                sei();
                uint8_t error = (pulse / 8) % 50;

                if((error > 1) && (error < 49)) { 
                    lowPinPORT |= (1<<IO_C);
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
#ifdef DEBUG
                    lowPinPORT &= ~(1<<IO_C);
#endif
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
                    UCSR0B |= (1<<UDRIE0); // enable interrupt to send bytes
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
                UCSR1B |= (1<<UDRIE1); // enable interrupt to send bytes
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
#ifdef DEBUG
        lowPinPORT ^= (1<<IO_D); // timing test for main
	if(!flags.ProdTest) {
	  if(systemMillis > ProdTestMillis) {
	      ProdTestMillis += ProdTestInterval;
	      highPinPORT ^= (1<<IO_J);
	      if(ProdTestMillis > ProdTestMax) {
		    flags.ProdTest = 1;
            highPinPORT &= ~(1<<IO_J);
          }
	  }
	}
#endif 
#ifdef ROTARYENCODER
    // check if rotary encoder has been moved
    // this will run at least every 5ms
    uint8_t pin0 = (highPinPIN & (1<<IO_G));
    uint8_t pin1 = (highPinPIN & (1<<IO_H));
    if(pin0 != encoderPinValues[0]) {
      rotary_encoder_change(0, pin0);
    } else if(pin1 != encoderPinValues[1]) {
      rotary_encoder_change(1, pin1);
    }
	if(!flags.sendEncoder) {
      if(systemMillis > ProdTestMillis) {
    	  if(encoderPosition != 0) { // if the encoder was moved it must be attached
    	    flags.sendEncoder = 1;
    	    SwitchBuf[1] = 3; // number of bytes in switch and encoder packet
          }
	  } else {
        encoderPosition = 0;
      }
	}
#endif
    // sleep to save energy here
        // by default sleep mode is idle
        MCUCR |= (1<<SE); // enable sleep
        __asm__ __volatile__ ( "sleep" "\n\t" :: );
        MCUCR &= ~(1<<SE); // disable sleep */
    } // end while(1)
} // end main

#ifdef ROTARYENCODER
void rotary_encoder_change(uint8_t changedPin, uint8_t value) {
  encoderPinValues[changedPin] = value;
  if((encoderPinValues[0] == encoderPinValues[1]) ^ changedPin)
    encoderPosition++;
  else
    encoderPosition--;
}
#endif

