#pragma once
void init_uart(void);
int sendUARTData(const char*);
int sendUARTBytes(const uint8_t* bytes, int len);
void flushUART(void);
void resetUARTRXData(void);

