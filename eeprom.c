#include "eeprom.h"
#include "i2c.h"



EEPROM_Response_t writeEEPROM(uint8_t* txBuf, uint16_t txBufSize) {
	uint8_t txbuf[3] = { 0x00, 0x0A, 0x48 };
	uint16_t writeAddr = (*(txBuf) << 8) + *(txBuf + 1);
	if (writeAddr  >= EEPROM_MAX_BLOCK) {
		return EEPROM_ADDR_ERROR;
	}
	if(!(txBufSize < EEPROM_MAX_BLOCK && txBufSize > 0)) {
		return EEPROM_DATA_SIZE_ERROR;
	}
	writeAddr *= 5; //from here we go from desired write block to desired write address
	HAL_StatusTypeDef ret;
	
	EEPROM_BLOCK writeBlockFirst;
	uint16_t prevAddr = EEPROM_MAX_ADDR;
	uint8_t prevBlockTx[6];
	uint8_t readAddr[2] = { txBuf[0], txBuf[1]};
	txBuf += 1;
	txBufSize -= 2; //we leave one of the txbuf additions for inside the while loop
	while (txBufSize != 0) {
		txBuf++;
		readAddr[0] = (writeAddr >> 8) & 0xFF;
		readAddr[1] = (writeAddr) & 0xFF;
		readEEPROMBlock(readAddr, writeBlockFirst.blockData);
		/*TODO:Dont send directly from txbuf, copy address + next data then send that instead*/
		writeBlockFirst.nextAddr = (uint16_t)((writeBlockFirst.blockData[1] << 6) + (writeBlockFirst.blockData[2] >> 2));
		writeBlockFirst.writeCount = ((uint32_t)((writeBlockFirst.blockData[2] & 1) << 16) + (writeBlockFirst.blockData[3] << 8) + writeBlockFirst.blockData[4]);
		writeBlockFirst.blockUsed = ((writeBlockFirst.blockData[2] & 2) >> 1);
		
		if (!writeBlockFirst.blockUsed && (writeBlockFirst.writeCount < EEPROM_BLOCK_WRITE_LIMIT)) {
			/*TODO FIND OUT HOW TO MAKE THIS WHOLE PART SEQUENTIAL*/
			
			ret = HAL_I2C_Master_Transmit(&hi2c1, EEPROM_ADDR_WRITE, (txBuf), 3, 50);
			/*TODO: Figure out logic for next addr and rebuilding write count and marking block as used*/
			
			/*TODO FIND OUT HOW TO MAKE THIS WHOLE PART SEQUENTIAL*/
			if (ret != HAL_OK) { 
				return EEPROM_ERROR;
			}	
			prevBlockTx[0] = (prevAddr >> 8) & 0xFF;
			prevBlockTx[1] = (prevAddr) & 0xFF; 
			if (prevAddr != EEPROM_MAX_ADDR) {
				ret = HAL_I2C_Master_Transmit(&hi2c1, EEPROM_ADDR_WRITE, prevBlockTx, 6, 50);
			}
			prevBlockTx[2] = (writeAddr >> 6) & 0xFF;
			prevBlockTx[3] = ((writeAddr << 2) & 0xFF) + 0x02 + (((writeBlockFirst.writeCount + 0x01) >> 16) & 0xFF); 
			prevBlockTx[4] = (writeBlockFirst.writeCount >> 8) & 0xFF;
			prevBlockTx[5] = writeBlockFirst.writeCount & 0xFF;
			
			txBufSize--;
			prevAddr = writeAddr + 1;
			if (writeBlockFirst.nextAddr != EEPROM_MAX_ADDR) {		
				writeAddr = writeBlockFirst.nextAddr;
			} else if (writeAddr + 5 != EEPROM_MAX_ADDR) {
				writeAddr += 5;	
			} else {
				writeAddr = 0;
			}
			
			
		} else {//This block is in use!
			
			if (writeAddr + 5 != EEPROM_MAX_ADDR) {
				writeAddr += 0x05;
			} else {
				writeAddr = 0;
			}
			
			
		}
		
		
	}
		
	/*Do this one last time for the last block in the writing sequence*/
	prevBlockTx[0] = ((prevAddr >> 8) & 0xFF);
	prevBlockTx[1] = ((prevAddr) & 0xFF); 
	if (prevAddr != EEPROM_MAX_ADDR) {
		ret = HAL_I2C_Master_Transmit(&hi2c1, EEPROM_ADDR_WRITE, prevBlockTx, 6, 50);
	}
	prevBlockTx[0] = (writeAddr >> 8) & 0xFF;
	prevBlockTx[1] = (writeAddr) & 0xFF; 
	prevBlockTx[2] = 0x40;
	writeBlockFirst.writeCount += 1;
	prevBlockTx[3] = 0x00 + 0x02 + ((writeBlockFirst.writeCount >> 16) & 0xFF); 
	prevBlockTx[4] = ((writeBlockFirst.writeCount & 0xFF00) >> 8) & 0xFF;
	prevBlockTx[5] = (writeBlockFirst.writeCount & 0xFF);
	ret = HAL_I2C_Master_Transmit(&hi2c1, EEPROM_ADDR_WRITE, prevBlockTx, 6, 50);
	
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