#define __EEPROM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#define EEPROM_ADDR 0x50
#define EEPROM_ADDR_WRITE (EEPROM_ADDR << 1)
#define EEPROM_ADDR_READ ((EEPROM_ADDR << 1) + 1)
#define EEPROM_MAX_ADDR 0x4000
#define EEPROM_BLOCK_WRITE_LIMIT 99500
#define EEPROM_MAX_BLOCK 0x0CCC

	typedef enum {
		EEPROM_OK         = 0x00U,
		EEPROM_ERROR      = 0x01U,
		EEPROM_ADDR_ERROR = 0x02U,
		EEPROM_DATA_SIZE_ERROR = 0x03u,
	} EEPROM_Response_t;

	typedef struct EEPROM_STRUCT {
		uint8_t blockData[5];
		uint8_t blockUsed;
		uint16_t addr;
		uint16_t nextAddr;
		uint32_t writeCount;
	} EEPROM_BLOCK;

EEPROM_Response_t readEEPROMBlock(uint8_t* txBuf, uint8_t* rxBuf);
#ifdef __cplusplus
}
#endif

