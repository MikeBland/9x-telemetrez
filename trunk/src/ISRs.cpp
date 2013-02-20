#include "telemetrEZ.h"
#include "externalVariables.h"
#include <util/delay.h>

static volatile uint32_t systemMillis = 0;

ISR(TIMER0_COMPA_vect) {
    systemMillis++;
#ifdef DEBUG
    lowPinPORT ^= (1<<IO_E);  // so we know the program is still running
#endif
}

uint32_t millis(void) {
  uint32_t m;
  uint8_t cSREG = SREG;
  
  cli();
  m = systemMillis;
  SREG = cSREG;
  
  return m;
}

ISR(TIMER1_CAPT_vect) { // track changes to PPM stream
    lastPPMchange = millis();

#ifdef CLOCK_ADJUST
    static uint16_t startTime, endTime;
    static uint8_t startDelay = 0;

    if((++startDelay > 100) && (sendTo9xEnable)) { // the first several pulses cannot be used for setting the clock
        startDelay = 101;
        if(TCCR1B & (1<<ICES1)) {
            // pin is high, just passed the end of a sync pulse
            if(TIFR & (1<<TOV1)) { // timer overflowed result might be wrong, so skip this one
                TCCR1B &= ~(1<<ICES1);  // next capture will be on falling edge
                TIFR |= (1<<ICF1)|(1<<TOV1); // clear the flags
            }  else {  
                endTime = ICR1;
                TCCR1B &= ~(1<<ICES1);  // next capture will be on falling edge
                TIFR |= (1<<ICF1); // clear the flag
                PPMpulseTime = endTime - startTime; // get time of pulse
                if((PPMpulseTime < 6500ul) && (PPMpulseTime > 700ul)) // check for valid pulse length
                    // Valid pulse will be 100us to 800us
                    ppmReady = 1; // signal main
            }
        } else {
            // pin is low, just passed the start of a sync pulse
            startTime = ICR1; // get time for start of pulse
            TCCR1B |= (1<<ICES1);  // next capture will be on rising edge
            TIFR |= (1<<ICF1); // clear the flag
        }
    } // end startDelay
#endif
}

