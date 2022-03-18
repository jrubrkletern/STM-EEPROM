#include "eeprom.h"
#include "i2c.h"



EEPROM_Response_t writeEEPROM(uint8_t* txBuf, uint16_t txBufSize) {
	uint16_t writeAddr = (*(txBuf) << 8) + *(txBuf + 0x01);
	if (writeAddr  >= EEPROM_MAX_BLOCK) {
		return EEPROM_ADDR_ERROR;
	}
	if (!(txBufSize < EEPROM_MAX_BLOCK && txBufSize > 0)) {
		return EEPROM_DATA_SIZE_ERROR;
	}
	writeAddr *= 5; //from here we go from desired write block to desired write address
	HAL_StatusTypeDef ret;
	uint16_t startAddr = writeAddr;
	
	EEPROM_BLOCK writeBlock, prevBlock;
	uint16_t prevAddr = EEPROM_MAX_ADDR;
	txBuf += 2;
	
	uint16_t bytesWritten = 0;
	uint8_t currentTx[3] = { 0 };
	while (txBufSize != 0) {
		
		currentTx[0] = (writeAddr >> 8) & 0xFF;
		currentTx[1] = (writeAddr) & 0xFF;
		EEPROM_Response_t eepromResponse;
		eepromResponse = readEEPROMBlock(currentTx, writeBlock.blockData);
		if(eepromResponse != EEPROM_OK) {
				startAddr /= 5;
				txBuf -= (bytesWritten + 2);
				*txBuf = (startAddr >> 8) & 0xFF;
				*(txBuf + 1) = startAddr & 0xFF;
				return EEPROM_ERROR;
		}
		writeBlock.nextAddr = (uint16_t)((writeBlock.blockData[1] << 6) + (writeBlock.blockData[2] >> 2));
		writeBlock.writeCount = ((uint32_t)((writeBlock.blockData[2] & 1) << 16) + (writeBlock.blockData[3] << 8) + writeBlock.blockData[4]);
		writeBlock.blockUsed = ((writeBlock.blockData[2] & 2) >> 1);
		writeBlock.addr = writeAddr;
		if (!writeBlock.blockUsed && (writeBlock.writeCount < EEPROM_BLOCK_WRITE_LIMIT)) {
			
			currentTx[2] = *(txBuf);
			txBuf++;
			ret = HAL_I2C_Master_Transmit(&hi2c1, EEPROM_ADDR_WRITE, currentTx, 3, 250);
			bytesWritten++;
			txBufSize--;
			
			if (ret != HAL_OK) {
				startAddr /= 5;
				txBuf -= (bytesWritten + 2);
				*txBuf = (startAddr >> 8) & 0xFF;
				*(txBuf + 1) = startAddr & 0xFF;
				return EEPROM_ERROR;
			}	
			
			prevBlock.blockData[0] = (prevAddr >> 8) & 0xFF;
			prevBlock.blockData[1] = (prevAddr) & 0xFF; 
			
			prevBlock.blockData[2] = (writeBlock.addr >> 6) & 0xFF;
			writeBlock.writeCount++;
			prevBlock.blockData[3] = ((writeBlock.addr << 2) & 0xFF) + 0x02 + (((writeBlock.writeCount + 0x01) >> 16) & 0xFF); 
			prevBlock.blockData[4] = (writeBlock.writeCount >> 8) & 0xFF;
			prevBlock.blockData[5] = writeBlock.writeCount & 0xFF;
			if (bytesWritten > 1) {//First block written to doesnt have a preceding one!
				ret = HAL_I2C_Master_Transmit(&hi2c1, EEPROM_ADDR_WRITE, prevBlock.blockData, 6, 50);
			}
			if (bytesWritten == 1) { //placeholder to do the logic
				startAddr = writeAddr; //our first place that we wrote to, we will note this in txBuf for the user to know where their data starts
			}
			prevAddr = writeAddr + 1;
		
		} 
		
		if (writeAddr + 0x05 != EEPROM_MAX_ADDR) {
			writeAddr += 0x05;	
		} else {
			writeAddr = 0x00;
		}
		if (writeAddr == startAddr && txBufSize != 0) {
			startAddr /= 5;
			txBuf -= (bytesWritten + 0x02);
			*txBuf = (startAddr >> 0x08) & 0xFF;
			*(txBuf + 0x01) = startAddr & 0xFF;
			return EEPROM_SPACE_ERROR;
		}
		
	}
		
	/*Do this one last time for the last block in the writing sequence*/
	prevBlock.blockData[0] = ((prevAddr >> 8) & 0xFF);
	prevBlock.blockData[1] = ((prevAddr) & 0xFF); 
	
	//maybe some of this is excessive 
	writeBlock.addr++;
	prevBlock.blockData[0] = (writeBlock.addr >> 8) & 0xFF;
	prevBlock.blockData[1] = (writeBlock.addr) & 0xFF; 
	prevBlock.blockData[2] = 0xFF;
	writeBlock.writeCount++;
	prevBlock.blockData[3] = 0xFC + 0x02 + ((writeBlock.writeCount >> 16) & 0xFF); 
	prevBlock.blockData[4] = ((writeBlock.writeCount & 0xFF00) >> 8) & 0xFF;
	prevBlock.blockData[5] = (writeBlock.writeCount & 0xFF);
	if (bytesWritten > 1) {
		ret = HAL_I2C_Master_Transmit(&hi2c1, EEPROM_ADDR_WRITE, prevBlock.blockData, 6, 50);
	}
	ret = HAL_I2C_Master_Transmit(&hi2c1, EEPROM_ADDR_WRITE, prevBlock.blockData, 6, 50);
	
	
	startAddr /= 5;
	txBuf -= (bytesWritten + 2);
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

EEPROM_Response_t eraseEEPROM(uint8_t* txBuf) {
	uint16_t eraseAddr = (*(txBuf) << 8) + *(txBuf + 0x01);
	
	if (eraseAddr  >= EEPROM_MAX_BLOCK) {
		return EEPROM_ADDR_ERROR;
	}
	eraseAddr *= 5;
	HAL_StatusTypeDef ret;
	
	EEPROM_BLOCK eraseBlock;
	uint8_t currentTx[2] = { (eraseAddr >> 8) & 0xFF, eraseAddr & 0xFF };
	eraseAddr += 1;
	do {	
		EEPROM_Response_t eepromResponse;
		eepromResponse = readEEPROMBlock(currentTx, eraseBlock.blockData);
		if (eepromResponse != EEPROM_OK) {
			return EEPROM_ERROR;
		}
		eraseBlock.nextAddr = (uint16_t)((eraseBlock.blockData[1] << 6) + (eraseBlock.blockData[2] >> 2));
		eraseBlock.blockUsed = ((eraseBlock.blockData[2] & 2) >> 1);
		eraseBlock.writeCount = ((uint32_t)((eraseBlock.blockData[2] & 1) << 16) + (eraseBlock.blockData[3] << 8) + eraseBlock.blockData[4]);
		
		if (eraseBlock.blockUsed && (eraseBlock.writeCount < EEPROM_BLOCK_WRITE_LIMIT)) {
			eraseBlock.blockData[0] = (eraseAddr >> 8) & 0xFF;
			eraseBlock.blockData[1] = eraseAddr & 0xFF;
			
			eraseBlock.blockData[2] = 0xFF;
			eraseBlock.writeCount++;
			eraseBlock.blockData[3] = 0xFC + ((eraseBlock.writeCount >> 16) & 0xFF); 
			eraseBlock.blockData[4] = (eraseBlock.writeCount >> 8) & 0xFF;
			eraseBlock.blockData[5] = (eraseBlock.writeCount & 0xFF);
			//erase it
			ret = HAL_I2C_Master_Transmit(&hi2c1, EEPROM_ADDR_WRITE, eraseBlock.blockData, 6, 100);
			if (ret != HAL_OK) {
				return EEPROM_ERROR;
			}
		}
		
			
		currentTx[0] = (eraseBlock.nextAddr >> 6) & 0xFF;
		currentTx[1] = (eraseBlock.nextAddr << 2) & 0xFF;
		eraseAddr = eraseBlock.nextAddr + 1;
	} while (eraseBlock.nextAddr != EEPROM_MAX_ADDR);
	
	
	
	return EEPROM_OK;
}

EEPROM_Response_t readEEPROM(uint8_t* txBuf, uint8_t* rxBuf, uint16_t rxBufSize) {
	uint16_t readAddr = (*(txBuf) << 8) + *(txBuf + 0x01);
	if (readAddr  >= EEPROM_MAX_BLOCK) {
		return EEPROM_ADDR_ERROR;
	}
	if (rxBufSize == 0) {
		return EEPROM_DATA_SIZE_ERROR;
	}
	readAddr *= 5;
	HAL_StatusTypeDef ret;
	
	EEPROM_BLOCK readBlock;
	uint8_t currentTx[2] = { (readAddr >> 8) & 0xFF, readAddr & 0xFF };
	
	do {	
		EEPROM_Response_t eepromResponse;
		eepromResponse = readEEPROMBlock(currentTx, readBlock.blockData);
		if (eepromResponse != EEPROM_OK) {
			return EEPROM_ERROR;
		}
		readBlock.nextAddr = (uint16_t)((readBlock.blockData[1] << 6) + (readBlock.blockData[2] >> 2));
		*rxBuf = readBlock.blockData[0];
		rxBuf++;
		rxBufSize--;
		currentTx[0] = (readBlock.nextAddr >> 6) & 0xFF;
		currentTx[1] = readBlock.nextAddr & 0xFF; 
	} while (readBlock.nextAddr != EEPROM_MAX_ADDR && (rxBufSize != 0));
	//have some sort of safe cycle checking in case we get back to the beginning maybe? unless size argument saves it
	
	
	return EEPROM_OK;
}

EEPROM_Response_t wipeEEPROM(void) {
	
	return EEPROM_OK;
}
EEPROM_Response_t formatEEPROM(void) {
	uint8_t txBuf[64] = { 0 };
	for(int i = 0; i < 64; i ++){
		txBuf[i] = 0;
	}
	uint16_t writeAddr = 0;
	for (int i = 0; i < 256; i++) {
		HAL_I2C_Master_Transmit(&hi2c1, EEPROM_ADDR_WRITE, txBuf, 66, 1000);
		writeAddr += 64;
		txBuf[0] = (writeAddr >> 8) & 0xFF;
		txBuf[1] = writeAddr & 0xFF;
	}
	
	return EEPROM_OK;
}