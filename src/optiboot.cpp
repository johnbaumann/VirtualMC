#include "optiboot.h"

/*
 * The same as do_spm but with disable/restore interrupts state
 * required to succesfull SPM execution
 * 
 * On devices with more than 64kB flash, 16 bit address is not enough,
 * so there is also RAMPZ used in that case.
 */
void do_spm_cli(optiboot_addr_t address, uint8_t command, uint16_t data) {
  uint8_t sreg_save;

  sreg_save = SREG;  // save old SREG value
  asm volatile("cli");  // disable interrupts
  #ifdef RAMPZ
    RAMPZ = (address >> 16) & 0xff;  // address bits 23-16 goes to RAMPZ
    do_spm((address & 0xffff), command, data); // do_spm accepts only lower 16 bits of address
  #else
    do_spm(address, command, data);  // 16 bit address - no problems to pass directly
  #endif
  SREG = sreg_save; // restore last interrupts state
}


// Erase page in FLASH
void optiboot_page_erase(optiboot_addr_t address) {
  do_spm_cli(address, __BOOT_PAGE_ERASE, 0);
}


// Write word into temporary buffer
void optiboot_page_fill(optiboot_addr_t address, uint16_t data) {
  do_spm_cli(address, __BOOT_PAGE_FILL, data);
}


//Write temporary buffer into FLASH
void optiboot_page_write(optiboot_addr_t address) {
  do_spm_cli(address, __BOOT_PAGE_WRITE, 0);
}



/*
 * Higher level functions for reading and writing from flash 
 * See the examples for more info on how to use these functions
 */

// Function to read a flash page and store it in an array (storage_array[])
void optiboot_readPage(const uint8_t allocated_flash_space[], uint8_t storage_array[], uint16_t page, char blank_character)
{
  uint8_t read_character;
  for(uint16_t j = 0; j < SPM_PAGESIZE; j++) 
  {
    read_character = pgm_read_byte(&allocated_flash_space[j + SPM_PAGESIZE*(page-1)]);
    if(read_character != 0 && read_character != 255)
      storage_array[j] = read_character; 
    else
      storage_array[j] = blank_character;   
  }
}


// Function to read a flash page and store it in an array (storage_array[]), but without blank_character
void optiboot_readPage(const uint8_t allocated_flash_space[], uint8_t storage_array[], uint16_t page)
{
  uint8_t read_character;
  for(uint16_t j = 0; j < SPM_PAGESIZE; j++) 
  {
    read_character = pgm_read_byte(&allocated_flash_space[j + SPM_PAGESIZE*(page-1)]);
    if(read_character != 0 && read_character != 255)
      storage_array[j] = read_character;
  }
}


// Function to write data to a flash page
void optiboot_writePage(const uint8_t allocated_flash_space[], uint8_t data_to_store[], uint16_t page)
{
  uint16_t word_buffer = 0; 
       
  // Erase the flash page
  optiboot_page_erase((optiboot_addr_t)(void*) &allocated_flash_space[SPM_PAGESIZE*(page-1)]);
    
  // Copy ram buffer to temporary flash buffer
  for(uint16_t i = 0; i < SPM_PAGESIZE; i++) 
  {
    if(i % 2 == 0) // We must write words
      word_buffer = data_to_store[i];
    else 
    {
      word_buffer += (data_to_store[i] << 8);
      optiboot_page_fill((optiboot_addr_t)(void*) &allocated_flash_space[i + SPM_PAGESIZE*(page-1)], word_buffer);
    }
  }
  
  // Writing temporary buffer to flash
  optiboot_page_write((optiboot_addr_t)(void*) &allocated_flash_space[SPM_PAGESIZE*(page-1)]);
}