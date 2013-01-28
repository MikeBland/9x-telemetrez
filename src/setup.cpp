#include "telemetrEZ.h"
#include "externalVariables.h"

static void CheckConnections(void);

//processor initalization
void setup(void) {
    // save some power, turn off unused peripherals
    PRR = (1<<PRTWI)|(1<<PRUSI)|(1<<PRADC);
    ACSRA = (1<<ACD); // analog comparator

    CheckConnections();
    // set up ports
    switch_DDR &= ~((1<<AIL_sw)|(1<<THR_sw)); // switches are inputs
    switch_PUE |= (1<<AIL_sw)|(1<<THR_sw); // enable pull-ups

    lowPinDDR |= (1<<IO_E)|(1<<IO_D)|(1<<IO_C)|(1<<IO_B)|(1<<IO_A); // all outputs
    lowPinPORT &= ~((1<<IO_E)|(1<<IO_D)|(1<<IO_C)|(1<<IO_B)|(1<<IO_A)); // all set low
#ifdef DEBUG
    highPinDDR |= (1<<IO_J); // output for production test
#endif
    PPMinPUE |= (1<<PPMin); // enable internal pullup
    PPMinDDR &= ~(1<<PPMin); // PPM input pin

    //USART0:  9x side USART
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
#if USE_2X
    UCSR0A = (1<<U2X0);  // double USART speed
#else
    UCSR0A = 0;  // single USART speed
#endif
    UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);  // 8-bit asynchronous mode 1 stop bit no parity
    UCSR0D = 0;     // no frame detection
    UCSR0B = (1<<RXCIE0)|(1<<RXEN0); //|(1<<TXEN0); // enables the Tx and Rx, and Rx interrupt

    //USART1: Frsky side USART
    UBRR1H = UBRRH_VALUE;
    UBRR1L = UBRRL_VALUE;
#if USE_2X
    UCSR1A = (1<<U2X1);  // double USART speed
#else
    UCSR1A = 0;  // single USART speed
#endif
    UCSR1C = (1<<UCSZ11)|(1<<UCSZ10);  // 8-bit asynchronous mode 1 stop bit no parity
    UCSR1D = 0;     // no frame detection
    UCSR1B = (1<<RXEN1)|(1<<RXCIE1)|(1<<TXEN1); // enables the Rx, and Rx interrupt

#ifdef BLUETOOTH
    pinFDDR |= (1<<IO_F); // disable output to bluetooth
    pinFPORT |= (1<<IO_F); // set the output high, pull-ups are on a different register 
                            // this chip, so high-impedance output should not be driven
    pinFPUE &= ~(1<<IO_F); // make sure pull-up is off
#endif

#ifdef ROTARYENCODER
    highPinDDR &= ~((1<<IO_G)|(1<<IO_H)|(1<<IO_I)); // make pins inputs
    highPinPUE |= (1<<IO_G)|(1<<IO_H)|(1<<IO_I); // enable the pullups
    highPinPORT |= (1<<IO_G)|(1<<IO_H)|(1<<IO_I);
#endif
  
    // set up 20ms timer
    TCCR0A = (1<<WGM01); // CTC mode
    TIMSK |= (1<<OCIE0A); // enable interrupt
    TCNT0 = 0;

    OCR0A = 156; // 5ms time out
    TCCR0B = (1<<CS02); // /256 prescaler, start timer0

    // set up timer 1 for input capture
    TIMSK |= (1<<ICIE1); // enable interrupt
    TCCR1B |= (1<<CS10); // start timer 1:1, interrupt on falling edge

    sei();
}


/* This function checks if the connectors are plugged in backward to the Tez */
void CheckConnections(void) {
  connectorCheck = 0;
  //setup and check AIL switch_DDR
  // if AIL switch bad then connectorCheck=1
  // setup and check THR switch_DDR
  // if THR switch bad then connectorCheck=2
  // if both are bad then connectorCheck=3
}


