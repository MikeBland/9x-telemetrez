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

    // check for PPM signal low before continuing
    // this makes sure the m64 is running
    PPMinPUE |= (1<<PPMin); // enable internal pullup
    PPMinDDR &= ~(1<<PPMin); // PPM input pin
    TCCR0A = (1<<WGM01); // CTC mode
    flags.ppmReady = 0;
    do {
        while(PPMinPIN & (1<<PPMin)); // wait for pin to go low
      TCNT0 = 0;
      // set up 10ms timer
#if F_CPU == 8000000
      OCR0A = 78; // 10ms time out
      TCCR0B = (1<<CS02)|(1<<CS00); // /1024 prescaler, start timer0
#elif F_CPU == 1000000
      OCR0A = 156; // 10ms time out
      TCCR0B = (1<<CS02); // /64 prescaler, start timer0
#endif
      while( !(TIFR & (1<<OCF0A)) ); // wait 10ms
      if(PPMinPIN & (1<<PPMin))
          flags.ppmReady = 1; // exit loop if pin is still low
      TCCR0B = 0; // stop timer
      TIFR |= OCF0A; // clear interrupt flag
    } while(!(flags.ppmReady));


#ifdef BLUETOOTH
    pin16DDR &= ~(1<<IO16); // disable output to bluetooth
    pin16PORT |= (1<<IO16); // set the output high, pull-ups are on a different register 
                            // this chip, so high-impedance output should not be driven
    pin16PUE &= ~(1<<IO16); // make sure pull-up is off
#endif

    //USART0:
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

    //USART1:
    UBRR1H = UBRRH_VALUE;
    UBRR1L = UBRRL_VALUE;
#if USE_2X
    UCSR1A = (1<<U2X1);  // double USART speed
#else
    UCSR1A = 0;  // single USART speed
#endif
    UCSR1C = (1<<UCSZ11)|(1<<UCSZ10);  // 8-bit asynchronous mode 1 stop bit no parity
    UCSR1D = 0;     // no frame detection
    UCSR1B = (1<<RXEN1)|(1<<TXEN1)|(1<<RXCIE1); // enables the Tx and Rx, and Rx interrupt

    // set up 20ms timer
    TCCR0A = (1<<WGM01); // CTC mode
    TIMSK |= (1<<OCIE0A); // enable interrupt
    TCNT0 = 0;
#if F_CPU == 8000000
    OCR0A = 156; // 5ms time out
    TCCR0B = (1<<CS02); // /256 prescaler, start timer0
#elif F_CPU == 1000000
    OCR0A = 78; // 5ms time out
    TCCR0B = (1<<CS01)|(1<<CS00); // /64 prescaler, start timer0
#endif

    // set up timer 1 for input capture
    TIMSK |= (1<<ICIE1); // enable interrupt
    TCCR1B |= (1<<ICES1)|(1<<CS10); // rising edge interrupt, start timer

    sei();
}

