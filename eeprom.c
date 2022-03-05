#include "eeprom.h"
#include "i2c.h"



EEPROM_Response_t writeEEPROM(uint8_t* txBuf, uint16_t txBufSize) {
	uint8_t txbuf[3] = { 0x00, 0x0A, 0x48 };
	uint16_t writeAddr = (*(txBuf) << 8) + *(txBuf + 0x01);
	if (writeAddr  >= EEPROM_MAX_BLOCK) {
		return EEPROM_ADDR_ERROR;
	}
	if(!(txBufSize < EEPROM_MAX_BLOCK && txBufSize > 0)) {
		return EEPROM_DATA_SIZE_ERROR;
	}
	writeAddr *= 5; //from here we go from desired write block to desired write address
	HAL_StatusTypeDef ret;
	uint16_t startAddr = writeAddr;
	
	EEPROM_BLOCK writeBlock, prevBlock;
	uint16_t prevAddr = EEPROM_MAX_ADDR;
	txBuf += 2;
	txBufSize -= 2; //we leave one of the txbuf additions for inside the while loop
	uint16_t iterations = 0;
	uint8_t currentTx[3] = {0};
	while (txBufSize != 0) {
		
		currentTx[0] = (writeAddr >> 8) & 0xFF;
		currentTx[1] = (writeAddr) & 0xFF;
		readEEPROMBlock(currentTx, writeBlock.blockData);
		
		writeBlock.nextAddr = (uint16_t)((writeBlock.blockData[1] << 6) + (writeBlock.blockData[2] >> 2));
		writeBlock.writeCount = ((uint32_t)((writeBlock.blockData[2] & 1) << 16) + (writeBlock.blockData[3] << 8) + writeBlock.blockData[4]);
		writeBlock.blockUsed = ((writeBlock.blockData[2] & 2) >> 1);
		writeBlock.addr = writeAddr;
		if (!writeBlock.blockUsed && (writeBlock.writeCount < EEPROM_BLOCK_WRITE_LIMIT)) {
			
			currentTx[2] = *(txBuf);
			txBuf++;
			ret = HAL_I2C_Master_Transmit(&hi2c1, EEPROM_ADDR_WRITE, currentTx, 3, 50);
			iterations++;
			txBufSize--;
			
			if (ret != HAL_OK) {
				txBuf -= (iterations + 2);
				*txBuf = (startAddr >> 8) & 0xFF;
				*(txBuf + 1) = startAddr & 0xFF;
				return EEPROM_ERROR;
			}	
			
			prevBlock.blockData[0] = (prevAddr >> 8) & 0xFF;
			prevBlock.blockData[1] = (prevAddr) & 0xFF; 
			if (iterations > 1) {//First block written to doesnt have a preceding one!
				ret = HAL_I2C_Master_Transmit(&hi2c1, EEPROM_ADDR_WRITE, prevBlock.blockData, 6, 50);
			}
			prevBlock.blockData[2] = (writeBlock.addr >> 6) & 0xFF;
			prevBlock.blockData[3] = ((writeBlock.addr << 2) & 0xFF) + 0x02 + (((writeBlock.writeCount + 0x01) >> 16) & 0xFF); 
			prevBlock.blockData[4] = (writeBlock.writeCount >> 8) & 0xFF;
			prevBlock.blockData[5] = writeBlock.writeCount & 0xFF;
			
			if(iterations == 1) { //placeholder to do the logic
				startAddr = writeAddr; //our first place that we wrote to, we will note this in txBuf for the user to know where their data starts
			}
			prevAddr = writeAddr + 1;
			if (writeBlock.nextAddr != EEPROM_MAX_ADDR) {		//likely dont want it to follow the previous path explicitly, considering removing this
				writeAddr = writeBlock.nextAddr;
			} else if (writeAddr + 5 != EEPROM_MAX_ADDR) {
				writeAddr += 5;	
			} else {
				writeAddr = 0;
			}	
		} 
		else {//This block is in use, increment to the next one
			
			if (writeAddr + 5 != EEPROM_MAX_ADDR) {
				writeAddr += 0x05;
			} else {
				writeAddr = 0;
			}	
			if (writeAddr == startAddr) {
				txBuf -= (iterations + 2);
				*txBuf = (startAddr >> 8) & 0xFF;
				*(txBuf + 1) = startAddr & 0xFF;
				return EEPROM_SPACE_ERROR;
			}
			
		}
		
	}
		
	/*Do this one last time for the last block in the writing sequence*/
	prevBlock.blockData[0] = ((prevAddr >> 8) & 0xFF);
	prevBlock.blockData[1] = ((prevAddr) & 0xFF); 
	if (prevAddr != EEPROM_MAX_ADDR) {
		ret = HAL_I2C_Master_Transmit(&hi2c1, EEPROM_ADDR_WRITE, prevBlock.blockData, 6, 50);
	}
	prevBlock.blockData[0] = (writeBlock.addr >> 8) & 0xFF;
	prevBlock.blockData[1] = (writeBlock.addr) & 0xFF; 
	prevBlock.blockData[2] = 0x40;
	writeBlock.writeCount += 1;
	prevBlock.blockData[3] = 0x00 + 0x02 + ((writeBlock.writeCount >> 16) & 0xFF); 
	prevBlock.blockData[4] = ((writeBlock.writeCount & 0xFF00) >> 8) & 0xFF;
	prevBlock.blockData[5] = (writeBlock.writeCount & 0xFF);
	ret = HAL_I2C_Master_Transmit(&hi2c1, EEPROM_ADDR_WRITE, prevBlock.blockData, 6, 50);
	
	txBuf -= (iterations + 2);
	*txBuf = (startAddr >> 8) & 0xFF;
	*(txBuf + 1) = startAddr & 0xFF;
	return EEPROM_OK;
}

EEPROM_Response_t readEEPROMBlock(uint8_t* txBuf, uint8_t* rxBuf) {
	if ((*(txBuf) << 8) + *(txBuf + 1)  >= EEPROM_MAX_ADDR) {
		return EEPROM_ADDR_ERROR;
	}
	HAL_StatusTypeDef ret;
	
	
	ret = HAL_I2C_Master_Transmit(&hi2c1, EEPROM_ADDR_READ, txBuf, 2, 50);
	if (ret != HAL_OK) {
		return EEPROM_ERROR;
	}
	ret = HAL_I2C_Master_Receive(&hi2c1, EEPROM_ADDR_READ, rxBuf, 5, 150);
	if (ret == HAL_OK) {
		return EEPROM_OK;
	}
	
	return EEPROM_ERROR;
}