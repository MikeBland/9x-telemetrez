#include "telemetrEZ.h"
#include "externalVariables.h"

ISR(TIMER0_COMPA_vect) {
    systemMillis++;
    lowPinPORT ^= (1<<IO1);  // so we know the program is still running
}

ISR(TIMER1_CAPT_vect) { // track changes to PPM stream
    static uint16_t startTime;

    if(PPMinPIN & (1<<PPMin)) {
        // pin is high, just passed the start of a sync pulse
        startTime = ICR1; // get time for start of pulse
        TCCR1B &= ~(1<<ICES1);  // next capture will be on falling edge
    } else {
        // pin is high, just passed the end of a sync pulse
        PPMpulseTime = ICR1 - startTime; // get time of pulse
        TCCR1B |= (1<<ICES1);  // next capture will be on falling edge
        flags.ppmReady = 1; // signal main
    }
    TIFR |= (1<<ICF1); // clear the flag
    lastPPMchange = systemMillis;
}

