# STM-EEPROM

I2C Drivers for the Rohm BR24T128-W. The driver handles wear-leveling and implements non-contiguous storage.

## User Routines:
  ### writeEEPROM(uint8_t*, uint16_t)
    Used to write data to the EEPROM chip to the address specified. If successful the where the data begins is returned in the beginning of the unsigned 8 bit buffer given to the function. If data was successfully written at the initial desired location, the values in the input buffer will not change.
  ### readEEPROM(uint8_t*)
    Used to read data starting at an address, the driver will follow the link for the specified address until a null terminating next address pointer is detected.
  ### eraseEEPROM(uint8_t*)
    Used to erase data starting at an address, the driver will follow the link for the specified address freeing up all addresses until a null terminating next address pointer is     detected.
How to use:
  
