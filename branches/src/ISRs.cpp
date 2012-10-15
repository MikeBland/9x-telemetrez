#include "telemetrEZ.h"
#include "externalVariables.h"
#include <util/delay.h>

ISR(TIMER0_COMPA_vect) {
    systemMillis++;
#ifdef DEBUG
    lowPinPORT ^= (1<<IO1);  // so we know the program is still running
#endif
}

ISR(TIMER1_CAPT_vect) { // track changes to PPM stream
    lastPPMchange = systemMillis;

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

#ifdef ROTARYENCODER
void rotary_encoder_change(uint8_t changedPin, uint8_t value) {
  encoderPinValues[changedPin] = value;
  if((encoderPinValues[0] == encoderPinValues[1]) ^ changedPin)
    encoderPosition++;
  else
    encoderPosition--;
}

ISR(PCINT2_vect, ISR_NOBLOCK) { // leaves other interrupts enabled
  if(!intStarted) {
    intStarted = 1; // flag for any additional interrupts
    _delay_ms(1); // this will debounce the encoder inputs
    cli(); // disable other interrupts
    uint8_t pin0 = (highPinPIN & (1<<IO15));
    uint8_t pin1 = (highPinPIN & (1<<IO13));
    if(pin0 != encoderPinValues[0]) {
      rotary_encoder_change(0, pin0);
    } else if(pin1 != encoderPinValues[1]) {
      rotary_encoder_change(1, pin1);
    }
    intStarted = 0;
    sei(); // reenable interrupts
  }
}
#endif

