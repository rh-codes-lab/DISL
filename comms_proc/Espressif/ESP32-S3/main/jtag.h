#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "ftdi.h"
void jtag_uart_tx_test();
void jtag_uart_loopback_test();
uint32_t jtag_axi_read(uint32_t address);
uint32_t jtag_axi_status();
void jtag_axi_write_noirscan(uint32_t address, uint32_t data);
void jtag_control_write(uint32_t control_0_31, uint32_t control_32_63, uint32_t control_64_95);
void jtag_axi_write(uint32_t address, uint32_t data);
void jtag_program(char* filename);
void jtag_program_softcore(char* filename);
void jtag_verify_softcore(char* filename);
void jtag_drscan_bytes(uint8_t* wbuf, uint16_t len);
void jtag_drscan_bytes_hold(uint8_t* wbuf, uint16_t len);
void jtag_drscan_bytes_read(uint8_t* wbuf, uint8_t* rbuf, uint16_t len);
void jtag_drscan_bits(uint16_t num_bits, uint8_t val);
void jtag_irscan_bits_hold(uint16_t num_bits, uint8_t val);
void jtag_irscan_bits_irpause(uint16_t num_bits, uint8_t val);
void jtag_irscan_bits_reset(uint16_t num_bits, uint8_t val);
void jtag_irscan_bits(uint16_t num_bits, uint8_t val);
void jtag_statemove(tap_state_t state);
void jtag_reset();
void jtag_idle();

#ifdef __cplusplus
}
#endif

