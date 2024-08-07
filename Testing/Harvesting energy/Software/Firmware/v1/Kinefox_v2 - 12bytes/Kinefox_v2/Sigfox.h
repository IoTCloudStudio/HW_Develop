#ifndef SIGFOX_H_
#define SIGFOX_H_

#include "UART.h"

#define SIGFOX_MAX_COMMAND_LEN				90
#define SIGFOX_START_BUFFER_SIZE			(2 * SIGFOX_MAX_COMMAND_LEN)

bool sigfoxPrintIdAndPac(const char* additionalInfo);
bool sigfoxAlive();
void sigfoxFirstStart();
bool sigfoxWaitForOkay(uint16_t timeoutMs);
bool sigfoxSleep();
bool sigfoxSend(uint8_t *data, uint8_t len);


#endif