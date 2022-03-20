# STM-EEPROM

I2C Drivers for the Rohm BR24T128-W. The driver handles wear-leveling and implements non-contiguous storage.

## User Routines:
  ### writeEEPROM(uint8_t*, uint16_t)
    Used to write data to the EEPROM chip to the address specified. If successful the where the data begins is returned in the beginning of the unsigned 8 bit buffer given to the function. If data was successfully written at the initial desired location, the values in the input buffer will not change.
  ### readEEPROM(uint8_t*, uint8_t*, uint16_t)
    Used to read data starting at an address, the driver will follow the link for the specified address until a null terminating next address pointer is detected.
  ### eraseEEPROM(uint8_t*)
    Used to erase data starting at an address, the driver will follow the link for the specified address freeing up all addresses until a null terminating next address pointer is detected.
  ### wipeEEPROM(void)
    Used to erase and free up all blocks in the EEPROM.
  ### formatEEPROM(void)
    Used to wipe every block clean of all data and write/pointer info, ideally for initial use of the EEPROM chip. Checks can be made to the final 4 addresses in the EEPROM to see if      
    formatting is needed.
    
How to use:
  Format EEPROM if needed, then  use the listed routines to read write and erase data on the chip.
