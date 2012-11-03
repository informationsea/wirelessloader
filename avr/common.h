/*
 * common.h
 *
 *  Created on: 2012/04/22
 *      Author: yasu
 */

#ifndef COMMON_H_
#define COMMON_H_

// AVR -> PC
#define CODE_READY            'Y'
#define CODE_OK               'O'
#define CODE_CONTINE          'N'
#define CODE_ERROR            'E'
#define CODE_UNKNOWN          'U'

#define CODE_ERROR_CHECKSUM       '1'
#define CODE_ERROR_OUT_OF_RANGE   '2'
#define CODE_ERROR_WRONG_PAGESIZE '3'

// PC -> AVR
#define CODE_ENTER_BOOTLOADER 0x03
#define CODE_EXIT             'X'

// PC <-> AVR
#define CODE_READ_INFORMATION 'I' // Fuse bits and signature, lockbits
#define CODE_ERASE_PROGRAM    'A'
#define CODE_READ_PROGRAM     'R'
#define CODE_WRITE_PROGRAM    'W'
#define CODE_READ_VERSION     'V'
#define CODE_CRC              'C'
#define CODE_END              'D'

#endif /* COMMON_H_ */
