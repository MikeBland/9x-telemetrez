#include "telemetrEZ.h"
#include "externalVariables.h"

static void Rx_Frsky_D(void);
void Rx_Frsky_XJT(void);

extern uint32_t millis(void);

ISR(USART1__RX_vect)
{
    if(flags2.ModuleMode)
        Rx_Frsky_D();
    else
        Rx_Frsky_XJT();
}

#define frskyDataIdle    0
#define frskyDataStart   1
#define frskyDataInFrame 2

#define START_STOP      0x7e
// ************************************************
// *** Receive from Frsky XJT module ***
// ************************************************
void Rx_Frsky_XJT(void)
{
    // packet processing, and Rx reset will be handled by main
    uint8_t stat;
    uint8_t data;
		uint8_t reset = 0 ;

    static uint8_t dataState = frskyDataIdle;

    if ( flags2.resetRx )
		{
			reset = 1 ;
      flags2.resetRx = 0;
		}
		else
		{
			stat = UCSR1A; // USART control and Status Register 0 A
  	  data = UDR1; // USART data register 0
    	if ( (stat & ((1 << FE1) | (1 << DOR1) | (1 << UPE1))) )
			{
				reset = 1 ;
			}
		}

    if ( reset )
    { // discard buffer and start fresh on any comms error
        flags.FrskyRxBufferReady = 0;
        numPktBytesFrsky = 0;
        dataState = frskyDataIdle;
    } 
    else
    {
				switch (dataState) 
      {
        case frskyDataInFrame:
          FrskyRxBuf[numPktBytesFrsky] = data;
					if ( numPktBytesFrsky < BufSize - 1 )
					{
						numPktBytesFrsky += 1 ;
					}
					break;

        case frskyDataIdle:
          if (data == START_STOP)
          {
            numPktBytesFrsky = 0;
            FrskyRxBuf[numPktBytesFrsky++] = data;
            dataState = frskyDataInFrame;
            // start 4ms timeout
            flags2.InPacket = 1;
            XJTPacketEnd = millis() + 4;
          }
          break;

      } // switch
    }

}

// ************************************************
// *** Receive from Frsky D series module ***
// ************************************************
// Framework for Frsky Rx and decoding taken from Er9x project
// Receive buffer state machine state defs
void Rx_Frsky_D(void)
{
  uint8_t stat;
  uint8_t data;

  static uint8_t dataState = frskyDataIdle;

  stat = UCSR1A; // USART control and Status Register 0 A
  data = UDR1; // USART data register 0

  if (stat & ((1 << FE1) | (1 << DOR1) | (1 << UPE1)))
  { // discard buffer and start fresh on any comms error
    flags.FrskyRxBufferReady = 0;
    numPktBytesFrsky = 0;
    dataState = frskyDataIdle;
  } 
  else
  {
    if (flags.FrskyRxBufferReady == 0) // can't get more data if the buffer hasn't been cleared
    {
      switch (dataState) 
      {
        case frskyDataStart:
          if (data == START_STOP) break; // Remain in userDataStart if possible 0x7e,0x7e doublet found.

          FrskyRxBuf[numPktBytesFrsky++] = data;
          dataState = frskyDataInFrame;
          break;

        case frskyDataInFrame:
          if (data == START_STOP) // end of frame detected
          {
            flags.FrskyRxBufferReady = 1;
            FrskyRxBuf[numPktBytesFrsky++] = data;
            dataState = frskyDataIdle;
            break;
          }
          FrskyRxBuf[numPktBytesFrsky++] = data;
          break;

        case frskyDataIdle:
          if (data == START_STOP)
          {
            numPktBytesFrsky = 0;
            FrskyRxBuf[numPktBytesFrsky++] = data;
            dataState = frskyDataStart;
          }
          break;

      } // switch
    } // if (FrskyRxBufferReady == 0)
  }
}

// ************************************************
// *** Receive from 9x ***
// ************************************************
// Framework for Frsky Rx and decoding taken from Er9x project
// Receive buffer state machine state defs
//#define frskyDataIdle    0
//#define frskyDataStart   1
//#define frskyDataInFrame 2
#define nineXDataStart      3
#define nineXDataInFrame    4

//#define START_STOP      0x7e
#define ESCAPE 0x1B
ISR(USART0__RX_vect)
{
  uint8_t stat;
  uint8_t data;
  
  static uint8_t dataState9x = frskyDataIdle;

  stat = UCSR0A; // USART control and Status Register 0 A
  data = UDR0; // USART data register 0

  if (stat & ((1 << FE0) | (1 << DOR0) | (1 << UPE0)))
  { // discard buffer and start fresh on any comms error
    flags.NinexRxBufferReady = 0;
    numPktBytes9x = 0;
  } 
  else
  {
    if ((flags.NinexRxBufferReady == 0) && (flags.PktReceived9x == 0)) // can't get more data if the buffer hasn't been cleared
    {
      switch (dataState9x) 
      {
        case frskyDataStart:
          if (data == START_STOP) break; // Remain in userDataStart if possible 0x7e,0x7e doublet found.

          if (numPktBytes9x < 19)
            NinexRxBuf[numPktBytes9x++] = data;
          dataState9x = frskyDataInFrame;
          break;

        case frskyDataInFrame:
          if (data == START_STOP) // end of frame detected
          {
            flags.NinexRxBufferReady = 1;
            NinexRxBuf[numPktBytes9x++] = data;
            dataState9x = frskyDataIdle;
            break;
          }
          NinexRxBuf[numPktBytes9x++] = data;
          break;

        case frskyDataIdle:
          if (data == START_STOP)
          {
            numPktBytes9x = 0;
            NinexRxBuf[numPktBytes9x++] = data;
            dataState9x = frskyDataStart;
          } else if(data == ESCAPE) {
                numPktBytes9x = 0;
                dataState9x = nineXDataStart;
            }
          break;
        case nineXDataStart:
              if (data == ESCAPE) break; // Remain in userDataStart if possible doublet found.
              NinexRxBuf[numPktBytes9x++] = data;
              dataState9x = nineXDataInFrame;
          break;
        case nineXDataInFrame:
            if (data == ESCAPE) // end of frame detected
          {
            flags.PktReceived9x = 1;
            dataState9x = frskyDataIdle;
            break;
          }
          NinexRxBuf[numPktBytes9x++] = data;
          break;
      } // switch
    } // if (FrskyRxBufferReady == 0)
  }
}

