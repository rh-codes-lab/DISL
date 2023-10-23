#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PACKET_SIZE 64 //512
#define PACKET_SIZE 32 //256
#define DIV_ROUND_UP(m, n)  ((uint32_t)(((m) + (n) - 1) / (n)))
#define FT2232H_MPSSE_READ_EP 1
#define FT2232H_MPSSE_WRITE_EP 2
#define FT2232H_UART_READ_EP 3
#define FT2232H_UART_WRITE_EP 4

typedef enum flow_control {
	NONE = 0x0,
	RTS_CTS = 0x1,
	DTR_DSR = 0x2,
	XON_XOFF = 0x4
} flow_control_t;
  
typedef enum tap_state {
	TAP_INVALID = -1,
	TAP_DREXIT2 = 0x0,
	TAP_DREXIT1 = 0x1,
	TAP_DRSHIFT = 0x2,
	TAP_DRPAUSE = 0x3,
	TAP_IRSELECT = 0x4,
	TAP_DRUPDATE = 0x5,
	TAP_DRCAPTURE = 0x6,
	TAP_DRSELECT = 0x7,
	TAP_IREXIT2 = 0x8,
	TAP_IREXIT1 = 0x9,
	TAP_IRSHIFT = 0xa,
	TAP_IRPAUSE = 0xb,
	TAP_IDLE = 0xc,
	TAP_IRUPDATE = 0xd,
	TAP_IRCAPTURE = 0xe,
	TAP_RESET = 0x0f,
} tap_state_t;

typedef enum jtag_command_type {
	JTAG_SCAN         = 1,
	JTAG_STATEMOVE    = 2
} jtag_command_type_t;

struct jtag_command {
  jtag_command_type_t type;
	uint8_t ir_scan;
	uint16_t num_bits;
	uint8_t *out_buffer;
	uint8_t *in_buffer;
	tap_state_t end_state;
};

enum ftdi_chip_type {
	TYPE_FT2232C,
	TYPE_FT2232H,
	TYPE_FT4232H,
	TYPE_FT232H,
};

struct mpsse_ctx {
	enum ftdi_chip_type type;
	uint8_t write_buffer[MAX_PACKET_SIZE];
	uint16_t write_count;
	uint16_t write_count_bits;
	uint8_t read_buffer[MAX_PACKET_SIZE];
	uint16_t read_count;
	uint16_t read_count_bits;
	uint16_t transferred;
};

void ftdi_init(void);
uint8_t tap_move_ndx(tap_state_t astate);
uint8_t tap_get_state_enum(tap_state_t state);
int DIV_ROUND_CLOSEST(uint32_t x, uint32_t divisor);
uint8_t tap_get_tms_path(tap_state_t start, tap_state_t end);
uint8_t tap_get_tms_path_len(tap_state_t start, tap_state_t end);
tap_state_t tap_state_transition(tap_state_t cur_state, uint8_t tms);
void bit_copy(uint8_t* dst, uint16_t dst_start, uint8_t* src, uint16_t src_start, uint16_t bit_count);

void ftdi_control(uint8_t bmRequestType, uint8_t bmRequest, uint16_t wValue, uint16_t wIndex, uint8_t packet);
void ftdi_uart_configure(uint8_t data_size, uint32_t baud_rate, flow_control_t flowcontrol, uint32_t ftdi_clock_freq);
void ftdi_uart_write(uint8_t* msg, uint16_t len);
uint16_t ftdi_uart_read(uint8_t* buf);
void ftdi_mpsse_write(uint8_t* msg, uint16_t len);
uint16_t ftdi_mpsse_read(uint8_t* buf);
// MPSSE
void ftdi_mpsse_open();
void ftdi_mpsse_purge();
uint16_t ftdi_buffer_write_space();
uint16_t ftdi_buffer_read_space();
void ftdi_buffer_write_byte(uint8_t data);
uint16_t ftdi_buffer_write(uint8_t* out, uint16_t out_offset, uint16_t bit_count);
uint16_t ftdi_buffer_add_read(uint16_t bit_count);
void ftdi_write_transfer();
void ftdi_read_transfer();
void ftdi_mpsse_flush();
void ftdi_mpsse_clock_data_out(uint8_t* out, uint16_t out_offset, uint16_t length, uint8_t mode);
void ftdi_mpsse_clock_data_in(uint8_t* in_, uint16_t in_offset, uint16_t length, uint8_t mode);
void ftdi_mpsse_clock_data(uint8_t* out, uint16_t out_offset, uint8_t* in_, uint16_t in_offset, uint16_t length, uint8_t mode);
void ftdi_mpsse_clock_tms_cs(uint8_t* out, uint16_t out_offset, uint8_t* in_, uint16_t in_offset, uint16_t length, uint8_t tdi, uint8_t mode);
void ftdi_mpsse_clock_tms_cs_out(uint8_t* out, uint16_t out_offset, uint16_t length, uint8_t tdi, uint8_t mode);   
// FTDI
tap_state_t ftdi_tap_get_state();
void ftdi_tap_set_state(tap_state_t state);
tap_state_t ftdi_tap_get_end_state();
void ftdi_tap_set_end_state(tap_state_t state);
void ftdi_move_to_state(tap_state_t goal_state);
void ftdi_end_state(tap_state_t state);
void ftdi_execute_statemove(struct jtag_command cmd);
void ftdi_execute_scan(struct jtag_command cmd);
void ftdi_execute_command(struct jtag_command cmd);

#ifdef __cplusplus
}
#endif


