#include "telemetrEZ.h"
#include "externalVariables.h"

//processor initalization
void setup(void) {
    // save some power, turn off unused peripherals
    PRR = (1<<PRTWI)|(1<<PRUSI)|(1<<PRADC);
    ACSRA = (1<<ACD); // analog comparator

    // set up ports
    switch_DDR &= ~((1<<AIL_sw)|(1<<THR_sw)); // switches are inputs
    switch_PUE |= (1<<AIL_sw)|(1<<THR_sw); // enable pull-ups

    lowPinDDR |= (1<<IO1)|(1<<IO2)|(1<<IO3)|(1<<IO4)|(1<<IO5); // all outputs
    lowPinPORT &= ~((1<<IO1)|(1<<IO2)|(1<<IO3)|(1<<IO4)|(1<<IO5)); // all set low
#ifdef DEBUG
    highPinDDR |= (1<<IO10); // output for production test
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
    UCSR0B = (1<<RXCIE0)|(1<<RXEN0)|(1<<TXEN0); // enables the Tx and Rx, and Rx interrupt

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
    UCSR1B = (1<<RXEN1)|(1<<RXCIE1); // enables the Rx, and Rx interrupt

    while(PPMinPIN & (1<<PPMin)); // wait for PPM pin to go low
    UCSR1B |= (1<<TXEN1); // enable 9x side Tx

#ifdef BLUETOOTH
    pin16DDR &= ~(1<<IO16); // disable output to bluetooth
    pin16PORT |= (1<<IO16); // set the output high, pull-ups are on a different register 
                            // this chip, so high-impedance output should not be driven
    pin16PUE &= ~(1<<IO16); // make sure pull-up is off
#endif

#ifdef ROTARYENCODER
    highPinDDR &= ~((1<<IO15)|(1<<IO13)|(1<<IO11)); // make pins inputs
    highPinPUE |= (1<<IO15)|(1<<IO13)|(1<<IO11); // enable the pullups
    // enable pin change interrupts for both pins
    PCMSK2 |= (1<<PCINT15)|(1<<PCINT13); // individual pins enable
    GIMSK |= (1<<PCIE2); // enable the general interrupt
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

