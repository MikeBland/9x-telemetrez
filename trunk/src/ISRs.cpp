#include "telemetrEZ.h"
#include "externalVariables.h"

ISR(TIMER0_COMPA_vect) {
    systemMillis++;
    lowPinPORT ^= (1<<IO1);  // so we know the program is still running
}

ISR(TIMER1_CAPT_vect) { // track changes to PPM stream
    lastPPMchange = systemMillis;

#ifdef CLOCK_ADJUST
    static uint16_t startTime;
    static uint8_t startDelay = 0;

    if(++startDelay > 32 && (sendTo9xEnable)) { // the first several pulses cannot be used for setting the clock
        startDelay = 33;
        if(TCCR1B && (1<<ICES1)) {
            // pin is high, just passed the end of a sync pulse
            if(TIFR && (1<<TOV1)) { // timer overflowed result might be wrong, so skip this one
                TCCR1B &= ~(1<<ICES1);  // next capture will be on falling edge
            } else {
                uint16_t endTime = ICR1;
                PPMpulseTime = endTime - startTime; // get time of pulse
                if((PPMpulseTime < 6500) && (PPMpulseTime > 700)) // check for valid pulse length
                    // Valid pulse will be 100us to 800us
                    flags.ppmReady = 1; // signal main
                TCCR1B &= ~(1<<ICES1);  // next capture will be on falling edge
            }
        } else {
            // pin is low, just passed the start of a sync pulse
            startTime = ICR1; // get time for start of pulse
            TCCR1B |= (1<<ICES1);  // next capture will be on rising edge
        }
    } // end startDelay
    TIFR |= (1<<ICF1); // clear the flag
#endif
}

