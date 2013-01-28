// externalVariables.h

#ifndef EXT_VARS
#define EXT_VARS

extern volatile uint8_t FrskyRxBuf[];
extern volatile uint8_t NinexRxBuf[];
extern volatile uint8_t SwitchBuf[];
extern volatile uint8_t numPktBytesFrsky;
extern volatile uint8_t numPktBytes9x;
extern volatile uint32_t systemMillis;
extern uint32_t sendSwitchesCount;
extern volatile uint32_t lastPPMchange;
extern volatile uint16_t PPMpulseTime;
extern volatile uint8_t sendTo9xEnable;

extern volatile ring_buffer FrskyTx_RB; // ring buffers for the pass thru
extern volatile ring_buffer NinexTx_RB;

extern volatile flgRegs flags;
extern volatile uint8_t ppmReady;
extern uint8_t connectorCheck;

#ifdef ROTARYENCODER
extern volatile uint8_t encoderPinValues[];
extern volatile uint8_t encoderPosition;
extern volatile uint8_t intStarted;
#endif

#endif
