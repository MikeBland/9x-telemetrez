/*
 * I2C_bb.h
 *
 *  Created on: May 14, 2012
 *      Author: hank
 */

#ifndef I2C_BB_H_
#define I2C_BB_H_
/** \file
Header file for the bit-banged I2C functions.
*/
/* Macros: */
// Port for the I2C
/** Data direction register for the 2 I2C pins */
#define I2C_DDR DDRA
/** Input register for the I2C pins */
#define I2C_PIN PINA
/** Output register for the I2C pins */
#define I2C_PORT PORTA

// Pins to be used in the bit banging
/** Pin number to use as I2C clock */
#define I2C_CLK 2
/** Pin number to use as I2C data */
#define I2C_DAT 3

/** Macro that sets the data pin high */
#define I2C_DATA_HI() I2C_DDR &= ~( 1 << I2C_DAT );
/** Macro that sets the data pin low */
#define I2C_DATA_LO() I2C_DDR |= ( 1 << I2C_DAT );

/** Macro that sets the clock pin high */
#define I2C_CLOCK_HI() I2C_DDR &= ~( 1 << I2C_CLK );
/** Macro that sets the data pin low */
#define I2C_CLOCK_LO() I2C_DDR |= ( 1 << I2C_CLK );

/* Function Prototypes: */
void I2C_Init(void);
void I2C_Start(void);
void I2C_Stop(void);
void I2C_WriteTo(const uint8_t address);
void I2C_ReadFrom(const uint8_t address);
void I2C_Write(uint8_t data);
uint8_t I2C_Read(const uint8_t acknack);
uint8_t I2C_checkIdle(void);

#endif /* I2C_BB_H_ */
