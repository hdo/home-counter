#ifndef __CONSOLE_H
#define __CONSOLE_H

void console_printString(char* data);
void console_printStringln(char* data);
void console_printNumber(uint32_t value);
void console_printNumberln(uint32_t value);
void console_printCRLF();
void console_printByte(uint8_t data);
void console_setEnabled(uint8_t enabled);

#endif /* end __CONSOLE_H */
/*****************************************************************************
**                            End Of File
******************************************************************************/
