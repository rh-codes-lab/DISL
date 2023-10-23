#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void arty_transfer_data(uint8_t *data, int size, uint8_t EP);
void arty_transfer_control(uint8_t addr, uint8_t ep, uint8_t bmReqType, uint8_t bRequest, uint8_t wValLo, uint8_t wValHi, uint16_t wInd, uint16_t total);
uint16_t arty_receive_data(uint8_t *data, uint16_t size, uint8_t EP);
void arty_gpio_uart_riscv_flash(char *filename);
void arty_flash(char *filename);
#ifdef __cplusplus
}
#endif

