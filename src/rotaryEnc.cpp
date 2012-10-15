// Rotary encoder decoding
// http://www.rocketnumbernine.com/2010/03/06/decoding-a-rotary-encoder


volatile uint8_t encoderPinValues[2] = {0,0};
volatile uint8_t encoderPosition = 0;

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
    sei(); // reenable interrupts
  }
}

void rotary_encoder_change(uint8_t changedPin, uint8_t value) {
  encoderPinValues[changedPin] = value;
  if((encoderPinValues[0] == encoderPinValues[1]) ^ changedPin)
    encoderPosition++;
  else
    encoderPosition--;
}

void setup() {
  highPinDDR &= ~((1<<IO15)|(1<<IO13)); // make both pins inputs
  highPinPUE |= (1<<IO15)|(1<<IO13); // enable the pullups
  // enable pin change interrupts for both pins
  PCMSK2 |= (1<<PCINT15)|(1<<PCINT13); // individual pins enable
  GIMSK |= (1<<PCIE2); // enable the general interrupt
  sei();
}
