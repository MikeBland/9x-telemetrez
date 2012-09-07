#include "telemetrEZ.h"
#include "externalVariables.h"

ISR(TIMER0_COMPA_vect) {
    systemMillis++;
    lowPinPORT ^= (1<<IO1);  // so we know the program is still running
}

ISR(TIMER1_CAPT_vect) { // track changes to PPM stream
#ifdef CLOCK_ADJUST
    static uint16_t startTime;

    if(TCCR1B & (1<<ICES1)) {
        // pin is high, just passed the end of a sync pulse
        PPMpulseTime = ICR1 - startTime; // get time of pulse
        TCCR1B &= ~(1<<ICES1);  // next capture will be on falling edge
        flags.ppmReady = 1; // signal main
    } else {
        // pin is low, just passed the start of a sync pulse
        startTime = ICR1; // get time for start of pulse
        TCCR1B |= (1<<ICES1);  // next capture will be on falling edge
    }
    TIFR |= (1<<ICF1); // clear the flag
#endif
    lastPPMchange = systemMillis;
}
