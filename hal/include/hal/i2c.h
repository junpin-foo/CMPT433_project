/* i2c.h 
 * 
 * This file declares functions for initializing and cleaning up the I2C interface, 
 * opening an I2C bus, and performing 16-bit register read/write operations.
 * 
 * These methods are taken from class guide: I2C Guide
 */

#ifndef _I2C_H_
#define _I2C_H_

#include <stdint.h>
#include <stddef.h>

void Ic2_initialize(void);
void Ic2_cleanUp(void);
int init_i2c_bus(const char* bus, int address);
void write_i2c_reg16(int i2c_file_desc, uint8_t reg_addr, uint16_t value);
uint16_t read_i2c_reg16(int i2c_file_desc, uint8_t reg_addr);
void write_i2c_reg8(int i2c_file_desc, uint8_t reg_addr, uint8_t value);
uint8_t read_i2c_reg8(int i2c_file_desc, uint8_t reg_addr);
void read_i2c_burst(int i2c_file_desc, uint8_t reg_addr, uint8_t *buffer, int length);

#endif