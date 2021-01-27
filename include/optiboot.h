/*------------------------ Optiboot header file ----------------------------|
 |                  																												|
 | June 2015 by Marek Wodzinski, https://github.com/majekw                  |
 | Modified June 2016 by MCUdude, https://github.com/MCUdude                |
 | Released to public domain                                                |
 |                                                                          |
 | This header file gives possibility to use SPM instruction                |
 | from Optiboot bootloader memory.                                         |
 |                                                                          |
 | There are 5 convenient functions available here:                         |
 | * optiboot_page_erase - to erase a FLASH page 						                |
 | * optiboot_page_fill - to put words into temporary buffer			          |
 | * optiboot_page_write - to write contents of temporary buffer into FLASH |																			                            |
 | * optiboot_readPage - higher level function to read a flash page and     |
 | 							          store it in an array           					 					|
 | * optiboot_writePage - higher level function to write content to         |
 |                         a flash page                                     |
 |                                                                          |
 | For some hardcore users, you could use 'do_spm' as raw entry to          |
 | bootloader spm function.                                                 |
 |-------------------------------------------------------------------------*/


#ifndef _OPTIBOOT_H_
#define _OPTIBOOT_H_  1

#include <avr/boot.h>
#include "Arduino.h"


/* 
 * Main 'magic' function - enter to bootloader do_spm function
 *
 * address - address to write (in bytes) but must be even number
 * command - one of __BOOT_PAGE_WRITE, __BOOT_PAGE_ERASE or __BOOT_PAGE_FILL
 * data - data to write in __BOOT_PAGE_FILL. In __BOOT_PAGE_ERASE or 
 *          __BOOT_PAGE_WRITE it control if boot_rww_enable is run
 *         (0 = run, !0 = skip running boot_rww_enable)
 *
 */

// 'typedef' (in following line) and 'const' (few lines below) are a way to define external function at some arbitrary address
typedef void (*do_spm_t)(uint16_t address, uint8_t command, uint16_t data);


/*
 * Devices with more than 64KB of flash:
 * - have larger bootloader area (1KB) (they are BIGBOOT targets)
 * - have RAMPZ register :-) 
 * - need larger variable to hold address (pgmspace.h uses uint32_t)
 */
#ifdef RAMPZ
  typedef uint32_t optiboot_addr_t;
#else
  typedef uint16_t optiboot_addr_t;
#endif

#if FLASHEND > 65534
  const do_spm_t do_spm = (do_spm_t)((FLASHEND-1023+2)>>1);
#else
  const do_spm_t do_spm = (do_spm_t)((FLASHEND-511+2)>>1);
#endif

void do_spm_cli(optiboot_addr_t, uint8_t, uint16_t);
void optiboot_page_erase(optiboot_addr_t);
void optiboot_page_fill(optiboot_addr_t, uint16_t);
void optiboot_page_write(optiboot_addr_t);
void optiboot_readPage(const uint8_t[], uint8_t[], uint16_t, char);
void optiboot_readPage(const uint8_t[], uint8_t[], uint16_t);
void optiboot_writePage(const uint8_t[], uint8_t[], uint16_t);

#endif /* _OPTIBOOT_H_ */
