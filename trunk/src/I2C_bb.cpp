/*
 * I2C_bb.cpp
 *
 *  Created on: May 14, 2012
 *      Author: hank
 */
#include <avr/io.h>
#include <avr/iotn1634.h> // need for avr-gcc 4.7.0
#ifndef F_CPU
#define F_CPU 8000000
#endif
#include <util/delay.h>
#include "I2C_bb.h"

/** This function initalizes the I2C bus on the pins specified.
* It sets the pins as high impedance with no pull-ups enabled.
* Once this function has been called the user program is free
* to use the I2C bus.
*/
void I2C_Init(void) {
  // set pins to input mode
  I2C_PUE |= (1<<I2C_DAT) | (1<<I2C_CLK); // enable pullups
  I2C_PORT &= ~((1<<I2C_DAT) | (1<<I2C_CLK)); // make sure the output is low
  I2C_CLOCK_HI();
  I2C_DATA_HI();
}
/** This function will check if the bus is in an idle state.
 * If both I2C_CLK and I2C_DAT are high the function will return
 * true (1).  Otherwise it returns false (0).
 */
uint8_t I2C_checkIdle(void) {
	if(I2C_PIN && ((1<<I2C_CLK)|(1<<I2C_DAT)))
		return 1;
	else
		return 0;
}
/** This function puts a start condition on the I2C bus.
* No error checking is done in this function.  The user
* program should make sure this function is only called
* when the I2C bus is in the idle state.
*/
void I2C_Start(void) {
    // both pins read high, so bus is in idle
    I2C_DATA_LO();
    _delay_us(5);
    I2C_CLOCK_LO();
    _delay_us(5);
}

/** This function puts a stop condition on the I2C bus.
*/
void I2C_Stop(void) {
  I2C_CLOCK_HI();
  _delay_us(5);
  I2C_DATA_HI();
}

/** call this function with the device address that needs to
* receive the data.  This function should be called when you
* want to write data to a device on the I2C bus.
*/
void I2C_WriteTo(const uint8_t address) {
  I2C_Write(address<<1);
}

/** call this function with the device address that needs to
* send the data.  This function should be called when you
* want to receive data from a device on the I2C bus.
*/
void I2C_ReadFrom(const uint8_t address) {
  I2C_Write((address<<1) | 0x01);
}

/** Use this function after calling I2C_WriteTo.  This function
* sends 1 data byte to the receiving device on the I2C bus.
*/
void I2C_Write(uint8_t data) {
  for(uint8_t i=8; i != 0; i--) {
    if(data & 0x80) {
      I2C_DATA_HI();
    } else {
      I2C_DATA_LO();
    }
    data = data << 1;
    I2C_CLOCK_HI();
    _delay_us(2);
    I2C_CLOCK_LO();
    //_delay_us(3);
  }
  I2C_CLOCK_HI();
  //_delay_us(1);
  while(!(I2C_PIN & (1<<I2C_CLK))); // wait for slave to release clock line
  //_delay_us(1);
  I2C_CLOCK_LO();
  I2C_DATA_LO();  // set bus back to active condition
}

/** This function will read 1 byte of data from the I2C bus.
* And then respond with either an ack or a nack.  The return value
* is the byte that was read from the bus.  If you want to ack the byte
* of data acknack needs to be true (!=0).  Call this function after
* calling I2C_ReadFrom.
*/
uint8_t I2C_Read(const uint8_t acknack) {
  uint8_t data = 0;

  I2C_DATA_HI(); // allow data to come in
  for(uint8_t i=8; i != 0; i--) {
    data = data<<1; //shift the bits into place
    I2C_CLOCK_HI();
    _delay_us(1);
    if(I2C_PIN & (1<<I2C_DAT))
      data |= 1;
    //_delay_us(1);
    I2C_CLOCK_LO();
    //_delay_us(2);
  }
  if(acknack) {
    I2C_DATA_LO();  // send ack
  }
  I2C_CLOCK_HI();
  _delay_us(2);
  I2C_CLOCK_LO();
  I2C_DATA_LO();  // send ack
  return data;
}

