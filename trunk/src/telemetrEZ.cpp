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
#include "ringbuffer.h"

// *** Buffers ***
#define BufSize 20
volatile uint8_t FrskyRxBuf[BufSize];
volatile uint8_t NinexRxBuf[BufSize];
volatile uint8_t SwitchBuf[4];
volatile uint8_t numPktBytesFrsky = 0;
volatile uint8_t numPktBytes9x = 0;

volatile ring_buffer FrskyTx_RB; // ring buffers for the pass thru
volatile ring_buffer NinexTx_RB;

volatile struct flgRegs {
    uint8_t sendSwitches:1; // trigger main to send switch states
    uint8_t switchto9x:1;   // currently sending switch states to 9x
    uint8_t sendingtoFrsky:1;   // currently sending frsky packet to frsky module
    uint8_t PktReceived9x:1;    // packet received from 9x for here, needs processing
    uint8_t FrskyRxBufferReady:1; // Frsky Packet received from Frsky module, forward to 9x
    uint8_t NinexRxBufferReady:1; // Frsky Packet received from 9x, forward to Frsky module
} flags;

void setup(void);

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

    while(1) {
    // send switch values every 20ms
        if(flags.sendSwitches) {
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
                
    }
}

//processor initalization
void setup(void) {
    // save some power, turn off unused peripherals
    PRR = (1<<PRTWI)|(1<<PRTIM1)|(1<<PRUSI)|(1<<PRADC);
    ACSRA = (1<<ACD); // analog comparator

    // set up ports
    switch_DDR &= ~((1<<AIL_sw)|(1<<THR_sw)); // switches are inputs
    switch_PUE |= (1<<AIL_sw)|(1<<THR_sw); // enable pull-ups

    lowPinDDR |= (1<<IO1)|(1<<IO2)|(1<<IO3)|(1<<IO4)|(1<<IO5);
    lowPinPORT &= ~((1<<IO1)|(1<<IO2)|(1<<IO3)|(1<<IO4)|(1<<IO5));

#ifdef BLUETOOTH
    pin16DDR &= ~(1<<IO16); // disable output to bluetooth
    pin16PORT |= (1<<IO16); // set the output high, pull-ups are on a different register 
                            // this chip, so high-impedance output should not be driven
    pin16PUE &= ~(1<<IO16); // make sure pull-up is off
#endif

    //USART0:
#if F_CPU == 8000000
    UBRR0 = 103; // 9600 baud @ 8MHz
#elif F_CPU == 1000000
    UBRR0 = 12; // 9600 baud @ 1MHz
#endif
    UCSR0A = (1<<U2X0);  // double USART speed
    UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);  // 8-bit asynchronous mode 1 stop bit no parity
    UCSR0D = 0;     // no frame detection
    UCSR0B = (1<<RXCIE0)|(1<<RXEN0)|(1<<TXEN0); // enables the Tx and Rx, and Rx interrupt

    //USART1:
#if F_CPU == 8000000
    UBRR1 = 103; // 9600 baud @ 8MHz
#elif F_CPU == 1000000
    UBRR1 = 12; // 9600 baud @ 1MHz
#endif
    UCSR1A = (1<<U2X1);  // double USART speed
    UCSR1C = (1<<UCSZ11)|(1<<UCSZ10);  // 8-bit asynchronous mode 1 stop bit no parity
    UCSR1D = 0;     // no frame detection
    UCSR1B = (1<<RXEN1)|(1<<TXEN1); // (1<<RXCIE1)| enables the Tx and Rx, and Rx interrupt

    // set up 20ms timer
    TCCR0A = (1<<WGM01); // CTC mode
    OCR0A = 78; // 20ms time out
    TIMSK |= (1<<OCIE0A); // enable interrupt
    TCNT0 = 0;
    TCCR0B = (1<<CS02); // /256 prescaler, start timer0

    sei();
}

// ************************************************
// *** Transmit to 9x ***
// ************************************************
#define TxIDLE 0
#define sendSwitchpacket 1
#define sendFrskypacket 2
ISR(USART1__UDRE_vect) {
    static uint8_t SwitchBuf_count;
    static uint8_t U1TXstate = TxIDLE;

    switch(U1TXstate) {
        case TxIDLE:
            if(flags.switchto9x) { // if switch packet is ready to send
                SwitchBuf_count = 0;
                UDR1 = SwitchBuf[SwitchBuf_count++];
                U1TXstate = sendSwitchpacket;
#ifdef BLUETOOTH
                pin16DDR &= ~(1<<IO16); // disable output to bluetooth
#endif
                break;
            }
            if(NinexTx_RB.empty()) { // if the buffer is empty
                    UCSR1B &= ~(1<<UDRIE1); // disable interrupt
                    break;
            }
#ifdef BLUETOOTH
            pin16DDR |= (1<<IO16); // enable output to bluetooth
#endif
            UDR1 = NinexTx_RB.front(); // load next byte from buffer
            NinexTx_RB.pop(); // remove the byte from the buffer
            U1TXstate = sendFrskypacket;
            break;
        case sendSwitchpacket:
            UDR1 = SwitchBuf[SwitchBuf_count++];
            if(SwitchBuf_count == 4) {
                U1TXstate = TxIDLE; // done sending switch packet
                flags.switchto9x = 0;
            }
            break;
        case sendFrskypacket:
            if(NinexTx_RB.front() == 0x7e) {
                U1TXstate = TxIDLE;
            }
            UDR1 = NinexTx_RB.front(); // load next byte from buffer
            NinexTx_RB.pop(); // remove the byte from the buffer
            break;
    } // switch
}

// ************************************************
// *** Transmit to Frsky ***
// ************************************************
ISR(USART0__UDRE_vect) {
    static uint8_t U0TXstate = TxIDLE;

    switch(U0TXstate) {
        case TxIDLE:
            if(FrskyTx_RB.empty()) { // if the buffer is empty
                    UCSR0B &= ~(1<<UDRIE0); // disable interrupt
                    break;
            }
            UDR0 = FrskyTx_RB.front(); // load next byte from buffer
            FrskyTx_RB.pop(); // remove the byte from the buffer
            U0TXstate = sendFrskypacket;
            break;
        case sendFrskypacket:
            if(FrskyTx_RB.front() == 0x7e) {
                U0TXstate = TxIDLE;
            }
            UDR0 = FrskyTx_RB.front(); // load next byte from buffer
            FrskyTx_RB.pop(); // remove the byte from the buffer
            break;
    } // switch
}

// ************************************************
// *** Receive from Frsky module ***
// ************************************************
// Framework for Frsky Rx and decoding taken from Er9x project
// Receive buffer state machine state defs
#define frskyDataIdle    0
#define frskyDataStart   1
#define frskyDataInFrame 2

#define START_STOP      0x7e

ISR(USART0__RX_vect)
{
  uint8_t stat;
  uint8_t data;

  static uint8_t dataState = frskyDataIdle;

  stat = UCSR0A; // USART control and Status Register 0 A
  data = UDR0; // USART data register 0

  if (stat & ((1 << FE0) | (1 << DOR0) | (1 << UPE0)))
  { // discard buffer and start fresh on any comms error
    flags.FrskyRxBufferReady = 0;
    numPktBytesFrsky = 0;
  } 
  else
  {
    if (flags.FrskyRxBufferReady == 0) // can't get more data if the buffer hasn't been cleared
    {
      switch (dataState) 
      {
        case frskyDataStart:
          if (data == START_STOP) break; // Remain in userDataStart if possible 0x7e,0x7e doublet found.

          FrskyRxBuf[numPktBytesFrsky++] = data;
          dataState = frskyDataInFrame;
          break;

        case frskyDataInFrame:
          if (data == START_STOP) // end of frame detected
          {
            flags.FrskyRxBufferReady = 1;
            FrskyRxBuf[numPktBytesFrsky++] = data;
            dataState = frskyDataIdle;
            break;
          }
          FrskyRxBuf[numPktBytesFrsky++] = data;
          break;

        case frskyDataIdle:
          if (data == START_STOP)
          {
            numPktBytesFrsky = 0;
            FrskyRxBuf[numPktBytesFrsky++] = data;
            dataState = frskyDataStart;
          }
          break;

      } // switch
    } // if (FrskyRxBufferReady == 0)
  }
}

// ************************************************
// *** Receive from 9x ***
// ************************************************
// Framework for Frsky Rx and decoding taken from Er9x project
// Receive buffer state machine state defs
//#define frskyDataIdle    0
//#define frskyDataStart   1
//#define frskyDataInFrame 2
#define nineXDataStart      3
#define nineXDataInFrame    4

//#define START_STOP      0x7e
#define ESCAPE 0x1B
ISR(USART1__RX_vect)
{
  uint8_t stat;
  uint8_t data;
  
  static uint8_t dataState9x = frskyDataIdle;

  stat = UCSR0A; // USART control and Status Register 0 A
  data = UDR0; // USART data register 0

  if (stat & ((1 << FE0) | (1 << DOR0) | (1 << UPE0)))
  { // discard buffer and start fresh on any comms error
    flags.NinexRxBufferReady = 0;
    numPktBytes9x = 0;
  } 
  else
  {
    if ((flags.NinexRxBufferReady == 0) && (flags.PktReceived9x == 0)) // can't get more data if the buffer hasn't been cleared
    {
      switch (dataState9x) 
      {
        case frskyDataStart:
          if (data == START_STOP) break; // Remain in userDataStart if possible 0x7e,0x7e doublet found.

          if (numPktBytes9x < 19)
            NinexRxBuf[numPktBytes9x++] = data;
          dataState9x = frskyDataInFrame;
          break;

        case frskyDataInFrame:
          if (data == START_STOP) // end of frame detected
          {
            flags.NinexRxBufferReady = 1;
            NinexRxBuf[numPktBytes9x++] = data;
            dataState9x = frskyDataIdle;
            break;
          }
          NinexRxBuf[numPktBytes9x++] = data;
          break;

        case frskyDataIdle:
          if (data == START_STOP)
          {
            numPktBytes9x = 0;
            NinexRxBuf[numPktBytes9x++] = data;
            dataState9x = frskyDataStart;
          } else if(data == ESCAPE) {
                numPktBytes9x = 0;
                dataState9x = nineXDataStart;
            }
          break;
        case nineXDataStart:
              if (data == ESCAPE) break; // Remain in userDataStart if possible doublet found.
              NinexRxBuf[numPktBytes9x++] = data;
              dataState9x = nineXDataInFrame;
          break;
        case nineXDataInFrame:
            if (data == ESCAPE) // end of frame detected
          {
            flags.PktReceived9x = 1;
            dataState9x = frskyDataIdle;
            break;
          }
          NinexRxBuf[numPktBytes9x++] = data;
          break;
      } // switch
    } // if (FrskyRxBufferReady == 0)
  }
}

ISR(TIMER0_COMPA_vect) {
    flags.sendSwitches = 1;
    lowPinPORT ^= (1<<IO1);  // so we know the program is still running
}

