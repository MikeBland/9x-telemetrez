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

    lowPinDDR |= (1<<IO_E)|(1<<IO_D)|(1<<IO_C)|(1<<IO_B)|(1<<IO_A); // all outputs
    lowPinPORT &= ~((1<<IO_E)|(1<<IO_D)|(1<<IO_C)|(1<<IO_B)|(1<<IO_A)); // all set low
#ifdef DEBUG
    highPinDDR |= (1<<IO_J); // output for production test
#endif
    PPMinPUE |= (1<<PPMin); // enable internal pullup
    PPMinDDR &= ~(1<<PPMin); // PPM input pin

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

// send packet to module, then >25ms later check for a response
    UDR0 = 0x7E;
    while( !(UCSR1A & (1<<UDRE1)) ); // wait for buffer to empty
    UDR0 = 0xF9;
    for(uint8_t i=0; i<8; i++) {
        while( !(UCSR1A & (1<<UDRE1)) ); // wait for buffer to empty
        UDR0 = 0;
    }
    while( !(UCSR1A & (1<<UDRE1)) ); // wait for buffer to empty
    UDR0 = 0x7E;
    // check for 0x7E received within 30ms
    // set up ms timer
    TCCR0A = (1<<WGM01); // CTC mode
    TIMSK |= (1<<OCIE0A); // enable interrupt
    TCNT0 = 0;
    OCR0A = 235; // 1ms time out
    TCCR0B = (1<<CS02)|(1<<CS00); // /1024 prescaler, start timer0
    TIFR |= (1<<OCF0A); // clear the flag
    flags2.ModuleMode = 0; // set flag to X mode
    while( !(TIFR & (1<<OCF0A)) ) { // until the timer overflows ~30ms
        if(UCSR1A & (1<<RXC1)) { // if a data byte is received
            uint8_t data = UDR1;
            if(data == 0x7E)
                flags2.ModuleMode = 1; // set flag for D mode
        }
    }
    if(!flags2.ModuleMode) { // set baud rate for X type module
        //USART0:  9x side USART
        UBRR0H = UBRRH_XJT_VALUE;
        UBRR0L = UBRRL_XJT_VALUE;
#if USE_2X_XJT
        UCSR0A = (1<<U2X0);  // double USART speed
#else
        UCSR0A = 0;  // single USART speed
#endif
        UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);  // 8-bit asynchronous mode 1 stop bit no parity
        UCSR0D = 0;     // no frame detection
        UCSR0B = (1<<RXCIE0)|(1<<RXEN0); //|(1<<TXEN0); // enables the Tx and Rx, and Rx interrupt

        //USART1: Frsky side USART
        UCSR1B = 0; // disable Tx and Rx
        UBRR1H = UBRRH_XJT_VALUE;
        UBRR1L = UBRRL_XJT_VALUE;
#if USE_2X_XJT
        UCSR1A = (1<<U2X1);  // double USART speed
#else
        UCSR1A = 0;  // single USART speed
#endif
        UCSR1C = (1<<UCSZ11)|(1<<UCSZ10);  // 8-bit asynchronous mode 1 stop bit no parity
        UCSR1D = 0;     // no frame detection
        UCSR1B = (1<<RXEN1)|(1<<RXCIE1)|(1<<TXEN1); // enables the Rx, and Rx interrupt

    } else {
        //USART0:  9x side USART
        UBRR0H = UBRRH_D_VALUE;
        UBRR0L = UBRRL_D_VALUE;
#if USE_2X_D
        UCSR0A = (1<<U2X0);  // double USART speed
#else
        UCSR0A = 0;  // single USART speed
#endif
        UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);  // 8-bit asynchronous mode 1 stop bit no parity
        UCSR0D = 0;     // no frame detection
        UCSR0B = (1<<RXCIE0)|(1<<RXEN0); //|(1<<TXEN0); // enables the Tx and Rx, and Rx interrupt
        //USART1: Frsky side (is already set for 9600
    }

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
  
    // set up ms timer
    TCCR0A = (1<<WGM01); // CTC mode
    TIMSK |= (1<<OCIE0A); // enable interrupt
    TCNT0 = 0;

    OCR0A = 125; // 1ms time out
    TCCR0B = (1<<CS01)|(1<<CS00); // /64 prescaler, start timer0

    sei();
}


