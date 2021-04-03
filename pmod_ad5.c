/******************************************************************************
 *  Copyright (c) 2016, Xilinx, Inc.
 *  All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1.  Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *
 *  2.  Redistributions in binary form must reproduce the above copyright 
 *      notice, this list of conditions and the following disclaimer in the 
 *      documentation and/or other materials provided with the distribution.
 *
 *  3.  Neither the name of the copyright holder nor the names of its 
 *      contributors may be used to endorse or promote products derived from 
 *      this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION). HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
/******************************************************************************
 *
 * 
 * IOP code (MicroBlaze) for Pmod AD5
 *
 * Operations implemented in this application:
 *   1. Simple, single read, and write to data area
 *
 * Switch configuration is done within this program, Pmod should 
 * be plugged into upper row of connector.
 *
 * The Pmod AD5 is based on AD7193 analog-to-digital converter 
 * https://store.digilentinc.com/pmod-ad5-4-channel-4-8-khz-24-bit-a-d-converter/
 *
 *
 * First, must write to the communications register.
 * The default state of the interface on power-up or after a reset, 
 * is expecting a write operation to the communications register. 
 *
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   Who  Date     Changes
 * ----- --- ------- -----------------------------------------------
 * 
 * 
 *
 * </pre>
 *
 *****************************************************************************/

#include "spi.h"
#include "timer.h"
#include "circular_buffer.h"
#include "xparameters.h"
#include "xil_io.h"

// MAILBOX_WRITE_CMD
#define RESET 			   0x1
#define READ_SINGLE_VALUE  0x3
#define STATUS    		   0x7
#define READ_CONFIGURATION 0x9
#define READ_MODE          0x11

// Log constants
#define LOG_BASE_ADDRESS (MAILBOX_DATA_PTR(4))
#define LOG_ITEM_SIZE sizeof(u32)
#define LOG_CAPACITY  (4000/LOG_ITEM_SIZE)

// continuous read enable = 01011100
//#define CONTINUOUS_READ_EN 0x5C
// continuous read disable = 01011000
//#define CONTINUOUS_READ_DIS 0x58


// read command send to communications register for next operation to be read from data register = 01011000
//#define READ_DATA   0x58


spi device;

/* Register Selection
 *
 *  RS2 RS1 RS0 Register                                              Register Size
 *  0   0   0   Communications register during a write operation      8 bits
 *  0   0   0   Status register during a read operation               8 bits
 *  0   0   1   Mode register                                         24 bits
 *  0   1   0   Configuration register                                24 bits
 *  0   1   1   Data register/data register plus status information   24 bits/32 bits
 *  1   0   0   ID register                                           8 bits
 *  1   0   1   GPOCON register                                       8 bits
 *  1   1   0   Offset register                                       24 bits
 *  1   1   1   Full-scale register                                   24 bits
 */

u8 WriteBuffer[3];



void set_mode(void){
	/*
	* First sends a write command for the Mode Register 0b00001
	* Then sends the 'Power-On/Reset = 0x080060' configuration 
	*/
	WriteBuffer[0] = 0x00;
	spi_transfer(device, (char*)WriteBuffer, NULL, 1);
	WriteBuffer[0] = 0x08;
	WriteBuffer[1] = 0x00;
	WriteBuffer[2] = 0x60;
	spi_transfer(device, (char*)WriteBuffer, NULL, 3);
}

void configure(void){
	/*
	* First sends a write command for the Configure Register 0b00010000
	* Then sends the configuration command to set gain to 1 for ADC input range of (+/-)2.5V
	*  (command = 0x000110 = 0000 0000 0000 0001 0001 0000)
	*  (default = 0x000117 = 0000 0000 0000 0001 0001 0111)
	*  Buffer is still on, so range is (AGND+0.25V to AVDD-0.25V) = (250mV - 3.25mV)
	*/
	WriteBuffer[0] = 0x10;
	spi_transfer(device, (char*)WriteBuffer, NULL, 1);
	WriteBuffer[0] = 0x00;
	WriteBuffer[1] = 0x01;
	WriteBuffer[2] = 0x10;
	spi_transfer(device, (char*)WriteBuffer, NULL, 3);
}

u32 get_configuration(){
	/*
	* Sends a read command for the Configuration Register 01010000 
	* Then returns the configuration
	*/
	WriteBuffer[0] = 0x50;
	u8 Configuration[3];
	spi_transfer(device, (char*)WriteBuffer, NULL, 1);
	spi_transfer(device, NULL, (char*)Configuration, 3);
	u32 v = ((Configuration[0] << 16) + (Configuration[1] << 8) + (Configuration[2]));
    return v;
}

u32 get_mode(){
	/*
	* Sends a read command for the Mode Register 01001000 
	* Then returns the configuration
	*/
	WriteBuffer[0] = 0x48;
	u8 ReadBuffer[3];
	spi_transfer(device, (char*)WriteBuffer, (char*)ReadBuffer, 3);
	u32 v = ((ReadBuffer[0] << 16) + (ReadBuffer[1] << 8) + (ReadBuffer[2]));
    return v;
}


u32 get_status(){
	/*
	* Sends a read command for the Status Register
	* Then returns the status
	*/
	WriteBuffer[0] = 0x40;
	u8 Status[1];
	spi_transfer(device, (char*)WriteBuffer, NULL, 1);
	spi_transfer(device, NULL, (char*)Status, 1);
	
	return Status[0];
}

u32 get_sample(){
	/*
	* Sends a continuous read command for the Data Register 0b01011100
	* (Data register/data register plus status information = 24 bits/32 bits)
	*/
	WriteBuffer[0] = 0x5C;
	spi_transfer(device, (char*)WriteBuffer, NULL, 1);
	u32 raw_data[1];
	spi_transfer(device, NULL, (char*)raw_data, 1);
	return raw_data[0];
}


int main()
{
	int cmd;
	device = spi_open(3, 2, 1, 0);

	// to configure the device
	//configure();

	// Run application
	while(1){
		// wait and store valid command
		while((MAILBOX_CMD_ADDR & 0x01)==0);
		cmd = MAILBOX_CMD_ADDR;

		switch(cmd){

			case RESET:
				MAILBOX_CMD_ADDR = 0x0;
				break;

			case STATUS:
				MAILBOX_DATA(0) = get_status();
				MAILBOX_CMD_ADDR = 0x0;
				break;

			case READ_SINGLE_VALUE:
				// write out reading, reset mailbox
				MAILBOX_DATA(0) = get_sample();
				MAILBOX_CMD_ADDR = 0x0;
				break;

			case READ_CONFIGURATION:
				// write out reading, reset mailbox
				MAILBOX_DATA(0) = get_configuration();
				MAILBOX_CMD_ADDR = 0x0;
				break;

			case READ_MODE:
				// write out reading, reset mailbox
				MAILBOX_DATA(0) = get_mode();
				MAILBOX_CMD_ADDR = 0x0;
				break;

			default:
				// reset command
				MAILBOX_CMD_ADDR = 0x0;
				break;
		}
	}
return(0);
}


