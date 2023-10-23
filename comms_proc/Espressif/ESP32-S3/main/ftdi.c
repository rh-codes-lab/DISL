#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "arty_driver.h"
#include "ftdi.h"


static struct mpsse_ctx ctx;
static tap_state_t current_state;
static tap_state_t target_state;
static uint8_t ftdi_jtag_mode;
static uint8_t mpsse_ep_wr;
static uint8_t mpsse_ep_rd;
static uint8_t uart_ep_wr;
static uint8_t uart_ep_rd;
static uint32_t uart_ep_rd_wMaxPacketSize;
static uint32_t FTDI_BASECLOCK;
    
const uint8_t POS_EDGE_OUT = 0x00;
const uint8_t NEG_EDGE_OUT = 0x01;
const uint8_t POS_EDGE_IN = 0x00;
const uint8_t NEG_EDGE_IN = 0x04;
const uint8_t MSB_FIRST = 0x00;
const uint8_t LSB_FIRST = 0x08;
const uint8_t BITMODE_MPSSE  = 0x02;
const uint8_t SIO_RESET_REQUEST = 0x00;
const uint8_t SIO_SET_LATENCY_TIMER_REQUEST = 0x09;
const uint8_t SIO_GET_LATENCY_TIMER_REQUEST = 0x0A;
const uint8_t SIO_SET_BITMODE_REQUEST = 0x0B;
const uint8_t SIO_RESET_SIO = 0;
const uint8_t SIO_RESET_PURGE_RX = 1;
const uint8_t SIO_RESET_PURGE_TX = 2;
const uint8_t FTDI_SIO_RESET_REQUEST_TYPE = 0x40;
const uint8_t FTDI_SIO_SET_BAUDRATE_REQUEST_TYPE = 0x40;
const uint8_t FTDI_SIO_SET_DATA_REQUEST_TYPE = 0x40;
const uint8_t FTDI_SIO_SET_FLOW_CTRL_REQUEST_TYPE = 0x40;
const uint8_t FTDI_SIO_SET_MODEM_CTRL_REQUEST_TYPE = 0x40;
const uint8_t FTDI_SIO_GET_LATENCY_TIMER_REQUEST_TYPE = 0xC0;
const uint8_t FTDI_SIO_SET_LATENCY_TIMER_REQUEST_TYPE = 0x40;
const uint8_t FTDI_SIO_SET_EVENT_CHAR_REQUEST_TYPE = 0x40;
const uint8_t FTDI_SIO_GET_MODEM_STATUS_REQUEST_TYPE = 0xc0;
const uint8_t FTDI_SIO_SET_BITMODE_REQUEST_TYPE = 0x40;
const uint8_t FTDI_SIO_READ_EEPROM_REQUEST_TYPE = 0xc0;
const uint8_t FTDI_SIO_RESET = 0;
const uint8_t FTDI_SIO_MODEM_CTRL = 1;
const uint8_t FTDI_SIO_SET_FLOW_CTRL = 2;
const uint8_t FTDI_SET_BAUD_RATE = 3;
const uint8_t FTDI_SIO_SET_DATA = 4;
const uint8_t CHANNEL_A = 1;
const uint8_t CHANNEL_B = 2;
const uint8_t FTDI_SIO_RESET_PURGE_RX = 1;
const uint8_t FTDI_SIO_RESET_PURGE_TX = 2;
const uint32_t FTDI_SIO_SET_DATA_STOP_BITS_1 = 0x0 << 11;
const uint32_t FTDI_SIO_SET_DATA_PARITY_NONE = 0x0 << 8;
const uint8_t FTDI_SIO_DISABLE_FLOW_CTRL = 0x0;
const uint32_t FTDI_SIO_RTS_CTS_HS  = (0x1 << 8);
const uint32_t FTDI_SIO_DTR_DSR_HS  = (0x2 << 8);
const uint32_t FTDI_SIO_XON_XOFF_HS = (0x4 << 8);
const uint8_t XOFF = 0x13;
const uint8_t XON = 0x11;  
const uint8_t tms_seqs_bits[6][6] = {
{0xff, 0x00, 0x17, 0x0A, 0x1B, 0x16}, 
{0xff, 0x00, 0x01, 0x05, 0x03, 0x0B}, 
{0xff, 0x03, 0x07, 0x01, 0x0F, 0x2F}, 
{0xff, 0x03, 0x01, 0x00, 0x0F, 0x2F}, 
{0xff, 0x03, 0x07, 0x17, 0x01, 0x01}, 
{0xff, 0x03, 0x07, 0x17, 0x01, 0x00}
};

const uint8_t tms_seqs_bit_count[6][6] = {
{7, 7, 7, 7, 7, 7}, 
{7, 7, 3, 4, 4, 5}, 
{7, 3, 5, 2, 6, 7}, 
{7, 3, 2, 1, 6, 7}, 
{7, 3, 5, 6, 2, 2}, 
{7, 3, 5, 6, 2, 1}
};



void ftdi_init(void)
{
        current_state = TAP_RESET;
        target_state = TAP_RESET;
        ftdi_jtag_mode = (LSB_FIRST | POS_EDGE_IN | NEG_EDGE_OUT);
        ctx.type = TYPE_FT2232H;
        ctx.write_count = 0;
        ctx.write_count_bits = 0;
        ctx.read_count = 0;
        ctx.read_count_bits = 0;
        ctx.transferred = 0; 
        mpsse_ep_wr = FT2232H_MPSSE_WRITE_EP;
        mpsse_ep_rd = FT2232H_MPSSE_READ_EP;
        uart_ep_wr = FT2232H_UART_WRITE_EP;
        uart_ep_rd = FT2232H_UART_READ_EP;
}

uint8_t tap_move_ndx(tap_state_t astate)
{
	uint8_t ndx;
	switch (astate) {
		case TAP_RESET:
			ndx = 0;
			break;
		case TAP_IDLE:
			ndx = 1;
			break;
		case TAP_DRSHIFT:
			ndx = 2;
			break;
		case TAP_DRPAUSE:
			ndx = 3;
			break;
		case TAP_IRSHIFT:
			ndx = 4;
			break;
		case TAP_IRPAUSE:
			ndx = 5;
			break;
		default:
		        ndx = 0;
		        break;		
	}
	return ndx;
}

int DIV_ROUND_CLOSEST(uint32_t x, uint32_t divisor)
{ 
  typeof(x) __x = x; 
  typeof(divisor) __d = divisor; 
  return (((typeof(x))-1) > 0 || (__x) > 0) ? (((__x) + ((__d) / 2)) / (__d)) :  (((__x) - ((__d) / 2)) / (__d)); 
  }

uint8_t tap_get_state_enum(tap_state_t state)
{
  return tap_move_ndx(state);
}
  
tap_state_t tap_state_transition(tap_state_t cur_state, uint8_t tms)
{
  tap_state_t new_state;
  if (tms) 
  {
	  switch (cur_state) {
		  case TAP_RESET:
			  new_state = cur_state;
			  break;
		  case TAP_IDLE:
		  case TAP_DRUPDATE:
		  case TAP_IRUPDATE:
			  new_state = TAP_DRSELECT;
			  break;
		  case TAP_DRSELECT:
			  new_state = TAP_IRSELECT;
			  break;
		  case TAP_DRCAPTURE:
		  case TAP_DRSHIFT:
			  new_state = TAP_DREXIT1;
			  break;
		  case TAP_DREXIT1:
		  case TAP_DREXIT2:
			  new_state = TAP_DRUPDATE;
			  break;
		  case TAP_DRPAUSE:
			  new_state = TAP_DREXIT2;
			  break;
		  case TAP_IRSELECT:
			  new_state = TAP_RESET;
			  break;
		  case TAP_IRCAPTURE:
		  case TAP_IRSHIFT:
			  new_state = TAP_IREXIT1;
			  break;
		  case TAP_IREXIT1:
		  case TAP_IREXIT2:
			  new_state = TAP_IRUPDATE;
			  break;
		  case TAP_IRPAUSE:
			  new_state = TAP_IREXIT2;
			  break;
		  default:
			  new_state = cur_state; // Error
			  break;
	  }
  } 
  else 
  {
	  switch (cur_state) {
		  case TAP_RESET:
		  case TAP_IDLE:
		  case TAP_DRUPDATE:
		  case TAP_IRUPDATE:
			  new_state = TAP_IDLE;
			  break;
		  case TAP_DRSELECT:
			  new_state = TAP_DRCAPTURE;
			  break;
		  case TAP_DRCAPTURE:
		  case TAP_DRSHIFT:
		  case TAP_DREXIT2:
			  new_state = TAP_DRSHIFT;
			  break;
		  case TAP_DREXIT1:
		  case TAP_DRPAUSE:
			  new_state = TAP_DRPAUSE;
			  break;
		  case TAP_IRSELECT:
			  new_state = TAP_IRCAPTURE;
			  break;
		  case TAP_IRCAPTURE:
		  case TAP_IRSHIFT:
		  case TAP_IREXIT2:
			  new_state = TAP_IRSHIFT;
			  break;
		  case TAP_IREXIT1:
		  case TAP_IRPAUSE:
			  new_state = TAP_IRPAUSE;
			  break;
		  default:
			  new_state = cur_state; // Error
			  break;
	  }
  }
  return new_state;
}

uint8_t tap_get_tms_path(tap_state_t start, tap_state_t end)
{
  return tms_seqs_bits[tap_move_ndx(start)][tap_move_ndx(end)];
}

uint8_t tap_get_tms_path_len(tap_state_t start, tap_state_t end)
{
  return tms_seqs_bit_count[tap_move_ndx(start)][tap_move_ndx(end)];
}


tap_state_t ftdi_tap_get_state()
{
    return current_state;
}

void ftdi_tap_set_state(tap_state_t state)
{
    current_state = state;
    return;
}

tap_state_t ftdi_tap_get_end_state()
{
    return target_state;
}
   
void bit_copy(uint8_t* dst, uint16_t dst_start, uint8_t* src, uint16_t src_start, uint16_t bit_count)
{
        uint16_t db = dst_start / 8;
        uint16_t sb = src_start / 8;
        uint8_t dq = dst_start % 8;
        uint8_t sq = src_start % 8;
        uint16_t lb = bit_count / 8;
        uint8_t lq = bit_count % 8;
        if ((sq == 0) && (lq == 0) && (dq == 0))
	{
            for (int i = 0; i < lb; i++)
                dst[db + i] = src[sb + i];
            return;
        }
        uint16_t idx = sb;
        uint16_t odx = db;
        for (int i = 0; i < bit_count; i++)
	{
            if (((src[idx] >> (sq&7)) & 1) == 1)
                dst[odx] |= 1 << (dq&7);
            else
                dst[odx] &= ~(1 << (dq&7));
            sq += 1;
            if (sq == 7)
	    {
                sq = 0;
                idx += 1;
            }
            dq += 1;
            if (dq == 7)
	    {
                dq = 0;
                odx += 1;
            }
        }
        return;
}


void ftdi_tap_set_end_state(tap_state_t state)
{
    target_state = state;
    return;
}

void ftdi_move_to_state(tap_state_t goal_state)
{
    tap_state_t start_state = ftdi_tap_get_state();
    uint8_t tms_bits  = tap_get_tms_path(start_state, goal_state);
    uint8_t tms_count = tap_get_tms_path_len(start_state, goal_state);
    for (int i = 0; i < tms_count; i++)
    {
        ftdi_tap_set_state(tap_state_transition(ftdi_tap_get_state(),(tms_bits >> i) & 1)); ;
    }
    ftdi_mpsse_clock_tms_cs_out(&tms_bits, 0, tms_count, 0, ftdi_jtag_mode);
}

void ftdi_end_state(tap_state_t state)
{
    ftdi_tap_set_end_state(state);
}

void ftdi_execute_statemove(struct jtag_command cmd)
{
    ftdi_end_state(cmd.end_state);
    if ((ftdi_tap_get_state() != ftdi_tap_get_end_state()) || ftdi_tap_get_end_state() == TAP_RESET)
        ftdi_move_to_state(ftdi_tap_get_end_state());
}

void ftdi_execute_scan(struct jtag_command cmd)
{
    if (cmd.ir_scan)
    {
        if (ftdi_tap_get_state() != TAP_IRSHIFT)
            ftdi_move_to_state(TAP_IRSHIFT);
    }
    else
    {
        if (ftdi_tap_get_state() != TAP_DRSHIFT)
            ftdi_move_to_state(TAP_DRSHIFT);
    }
    ftdi_end_state(cmd.end_state);
    uint16_t scan_size = cmd.num_bits;
    while (scan_size > 0)
    {
        if ((scan_size < (PACKET_SIZE << 3)) || ((scan_size == (PACKET_SIZE << 3))  && (ftdi_tap_get_state() != ftdi_tap_get_end_state())))
	{
            ftdi_mpsse_clock_data(cmd.out_buffer, 0, cmd.in_buffer, 0, scan_size-1, ftdi_jtag_mode);
            uint8_t last_bit = 0;
            bit_copy(&last_bit, 0, cmd.out_buffer, scan_size-1, 1);
            uint8_t tms_bits = 0x03;
            ftdi_mpsse_clock_tms_cs(&tms_bits, 0, cmd.in_buffer, scan_size - 1, 1, last_bit, ftdi_jtag_mode);
            ftdi_tap_set_state(tap_state_transition(ftdi_tap_get_state(), 1));
            if (ftdi_tap_get_end_state() == TAP_IDLE)
	    {
                ftdi_mpsse_clock_tms_cs_out(&tms_bits, 1, 2, last_bit, ftdi_jtag_mode);
                ftdi_tap_set_state(tap_state_transition(ftdi_tap_get_state(), 1));
                ftdi_tap_set_state(tap_state_transition(ftdi_tap_get_state(), 0));
            }
	    else
	    {
                ftdi_mpsse_clock_tms_cs_out(&tms_bits, 2, 1, last_bit, ftdi_jtag_mode);
                ftdi_tap_set_state(tap_state_transition(ftdi_tap_get_state(), 0));
            }
            scan_size = 0;
        }
        else
	{
            ftdi_mpsse_clock_data(cmd.out_buffer, 0, cmd.in_buffer, 0, PACKET_SIZE << 3, ftdi_jtag_mode);
            scan_size -= PACKET_SIZE << 3;
        }
    }
    if (ftdi_tap_get_state() != ftdi_tap_get_end_state())
        ftdi_move_to_state(ftdi_tap_get_end_state());
}

void ftdi_execute_command(struct jtag_command cmd)
{
    if (cmd.type == JTAG_STATEMOVE)
        ftdi_execute_statemove(cmd);
    else if (cmd.type == JTAG_SCAN)
        ftdi_execute_scan(cmd);
    ftdi_mpsse_flush();
    if (cmd.in_buffer)
    {
      bit_copy(cmd.in_buffer, 0, ctx.read_buffer, 0, cmd.num_bits);
    }
    return;
}

void ftdi_mpsse_open()
{
        ftdi_control(0x40, 0, 0, 1, 0);
        ftdi_control(0x40, 9, 255, 1, 0);
        ftdi_control(0x40, 11, 0x020b, 1, 0);
        ftdi_control(0x40, 0, 1, 1, 0);
        ftdi_control(0x40, 0, 2, 1, 0);
        uint8_t buf_1[12] = {0x80, 0x88, 0x8b, 0x82, 0x00, 0x00, 0x85, 0x97, 0x8A, 0x86, 0x02, 0x00};
        uint8_t buf_2[5] = {0x97, 0x8A, 0x86, 0x02, 0x00};
        uint8_t buf_3[3] = {0x4B, 0x06, 0x7F};
        ftdi_mpsse_write(buf_1, 12);
        ftdi_mpsse_write(buf_2,5);  
        ftdi_mpsse_write(buf_3,3); 
        uint8_t tms_count = tms_seqs_bit_count[tap_move_ndx(current_state)][tap_move_ndx(target_state)];
        uint8_t tms_bits = tms_seqs_bits[tap_move_ndx(current_state)][tap_move_ndx(target_state)];
        ftdi_mpsse_clock_tms_cs_out(&tms_bits, 0, tms_count, 0, ftdi_jtag_mode);  
        return;
}

void ftdi_mpsse_purge()
{
        ftdi_control(0x40, 0, 1, 1, 0);
        ftdi_control(0x40, 0, 2, 1, 0);
}
        
uint16_t ftdi_buffer_write_space()
{
        return (MAX_PACKET_SIZE - ctx.write_count - 1);
}
        
uint16_t ftdi_buffer_read_space()
{
        return (MAX_PACKET_SIZE - ctx.read_count);
}

void ftdi_buffer_write_byte(uint8_t data)
{
        ctx.write_buffer[ctx.write_count] = data;
        ctx.write_count_bits += 8;
        ctx.write_count = DIV_ROUND_UP(ctx.write_count_bits, 8);
}

        
uint16_t ftdi_buffer_write(uint8_t* out, uint16_t out_offset, uint16_t bit_count)
{
        bit_copy(ctx.write_buffer, ctx.write_count_bits, out, out_offset, bit_count);
        ctx.write_count_bits += bit_count;
        ctx.write_count = DIV_ROUND_UP(ctx.write_count_bits, 8);
        return bit_count;
}

uint16_t ftdi_buffer_add_read(uint16_t bit_count)
{
        ctx.read_count_bits += bit_count;
        ctx.read_count =  DIV_ROUND_UP(ctx.read_count_bits,8);
        return bit_count;
}
    
void ftdi_write_transfer()
{
        uint8_t* chunk;
        uint16_t remaining_bytes = ctx.write_count;
        ctx.transferred = 0;
        while (remaining_bytes > 0)
	{
            if (remaining_bytes < PACKET_SIZE)
            {
                chunk = ctx.write_buffer + ctx.transferred;
                ftdi_mpsse_write(chunk,remaining_bytes);
                remaining_bytes = 0;
            }
            else
            {
                chunk = ctx.write_buffer + ctx.transferred;
                ftdi_mpsse_write(chunk,PACKET_SIZE);
                remaining_bytes -= PACKET_SIZE;
                ctx.transferred += PACKET_SIZE;
            }
        }
        ctx.write_count = 0;
        ctx.write_count_bits = 0;
        ctx.transferred = 0;
        return;
}

void ftdi_read_transfer()
{
        uint16_t remaining_bytes = ctx.read_count;
        ctx.transferred = 0;
        while (remaining_bytes > 0)
	{
            uint16_t size;
            uint8_t chunk[MAX_PACKET_SIZE]={0};
            size = ftdi_mpsse_read(chunk);
            for (int i = 2; i < size; i++)
                ctx.read_buffer[ctx.transferred + i - 2] = chunk[i];
            
            if (remaining_bytes < (size-2))
              remaining_bytes = 0;
            else
              remaining_bytes = remaining_bytes - size + 2;
            ctx.transferred = ctx.transferred - size + 2;
            vTaskDelay(1000/ portTICK_PERIOD_MS); 
        }
        ctx.read_count_bits = 0;
        ctx.read_count = 0;
        ctx.transferred = 0;
        return;
}       

void ftdi_mpsse_flush()
{
      if (ctx.write_count == 0)
          return;
      if (ctx.read_count)
          ftdi_buffer_write_byte(0x87);
      ftdi_write_transfer();
      if (ctx.read_count)
          ftdi_read_transfer();
      return;
}  
  
void ftdi_mpsse_clock_data_out(uint8_t* out, uint16_t out_offset, uint16_t length, uint8_t mode)
{
        ftdi_mpsse_clock_data(out, out_offset, NULL, 0, length, mode);  
        return;
}
        
void ftdi_mpsse_clock_data_in(uint8_t* in_, uint16_t in_offset, uint16_t length, uint8_t mode)
{
        ftdi_mpsse_clock_data(NULL, 0, in_, in_offset, length, mode); 
        return;
}

void ftdi_mpsse_clock_data(uint8_t* out, uint16_t out_offset, uint8_t* in_, uint16_t in_offset, uint16_t length, uint8_t mode)
{
        uint8_t _mode = mode;
        uint16_t _length = length;
        uint16_t _out_offset = out_offset;
        uint16_t _in_offset = in_offset;
        uint8_t cond1, cond2, cond3;
        cond1 = (out || ((out == NULL) && (in_ == NULL))); 
        cond2 = cond1 ? 4 : 3;
        if (cond1)
            _mode |= 0x10;
        if (in_)
            _mode |= 0x20;
        while (_length > 0)
	{
            cond3 = (_length < 8) ? 1 : 0;
            if (((ftdi_buffer_write_space() + cond3) < cond2) || (in_ && (ftdi_buffer_read_space() < 1)))
                ftdi_mpsse_flush();
            if (_length < 8)
	    {
                ftdi_buffer_write_byte(0x02 | _mode);
                ftdi_buffer_write_byte(_length-1);
                if (out)
                    _out_offset += ftdi_buffer_write(out, _out_offset, _length);
                if (in_)
                    _in_offset += ftdi_buffer_add_read(_length);
                if ((out == 0) && (in_ == 0))
                    ftdi_buffer_write_byte(0x00);
                _length = 0;
            }
            else
	    {
                uint16_t this_bytes = (uint16_t)(_length/8);
                if (this_bytes > MAX_PACKET_SIZE)
                    this_bytes = MAX_PACKET_SIZE;
                if ((cond1) && ((this_bytes + 3) > ftdi_buffer_write_space()))
                    this_bytes = ftdi_buffer_write_space() - 3;
                if (in_ && (this_bytes > ftdi_buffer_read_space()))
                    this_bytes = ftdi_buffer_read_space();
                if (this_bytes > 0){
                    ftdi_buffer_write_byte(_mode);
                    ftdi_buffer_write_byte((this_bytes - 1) & 0xff);
                    ftdi_buffer_write_byte((this_bytes - 1) >> 8);
                    if (out)
                        _out_offset += ftdi_buffer_write(out, _out_offset, this_bytes * 8);
                    if (in_)
                        _in_offset += ftdi_buffer_add_read(this_bytes * 8);
                    if ((out == 0) && (in_ == 0))
		    {
                        for (int n = 0; n < this_bytes; n++)
                            ftdi_buffer_write_byte(0x00);
                    }
                    _length -= this_bytes * 8;
                }
            }
        }
}

void ftdi_mpsse_clock_tms_cs_out(uint8_t* out, uint16_t out_offset, uint16_t length, uint8_t tdi, uint8_t mode)
{
        ftdi_mpsse_clock_tms_cs(out, out_offset, NULL, 0, length, tdi, mode);
        return;
}

void ftdi_mpsse_clock_tms_cs(uint8_t* out, uint16_t out_offset, uint8_t* in_, uint16_t in_offset, uint16_t length, uint8_t tdi, uint8_t mode)
{
        uint8_t _mode = mode;
        uint16_t _length = length;
        uint16_t _out_offset = out_offset;
        uint16_t _in_offset = in_offset;
        _mode |= 0x42;
        if (in_)
            _mode |= 0x20;
        while (_length > 0)
	{
            if ((ftdi_buffer_write_space() < 3) || (in_ && (ftdi_buffer_read_space() < 1)))
                ftdi_mpsse_flush();
            uint16_t this_bits = _length;
            if (this_bits > 7)
                this_bits = 7;
            if (this_bits > 0)
	    {
                ftdi_buffer_write_byte(_mode);
                ftdi_buffer_write_byte(this_bits - 1);
                uint8_t data = 0;
                bit_copy(&data, 0, out, _out_offset, this_bits);
                _out_offset += this_bits;
                ftdi_buffer_write_byte(data | ( tdi ? 0x80 :  0x00));
                if (in_)
                    _in_offset += ftdi_buffer_add_read(this_bits);
                _length -= this_bits ;
           }
        }
  }

void ftdi_control(uint8_t bmRequestType, uint8_t bmRequest, uint16_t wValue, uint16_t wIndex, uint8_t packet)
{
    arty_transfer_control(1, 0, bmRequestType, bmRequest, wValue & 0xFF, (wValue >> 8) & 0xFF, wIndex, 0);
    // ADDBACK Usb->ctrlReq(Arty->bAddress, 0, bmRequestType, bmRequest, wValue & 0xFF, (wValue >> 8) & 0xFF, wIndex & 0xFF, (wIndex >> 8) & 0xFF, packet, NULL, NULL);
    return;
}

void ftdi_uart_configure(uint8_t data_size, uint32_t baud_rate, flow_control_t flowcontrol, uint32_t ftdi_clock_freq)
{
      FTDI_BASECLOCK = ftdi_clock_freq;
      uint8_t divfrac[8] = {0, 3, 2, 4, 1, 5, 6, 7 };
      int divisor3 = DIV_ROUND_CLOSEST(8 * FTDI_BASECLOCK, 10 * baud_rate);
      uint32_t divisor = divisor3 >> 3;
      divisor |= divfrac[divisor3 & 0x7] << 14;
      if (divisor == 1)
          divisor = 0;
      else if (divisor == 0x4001)
          divisor = 1;
      uint32_t FTDI_BAUD_DIVISOR = divisor;
      ftdi_control(FTDI_SIO_RESET_REQUEST_TYPE, FTDI_SIO_RESET, FTDI_SIO_RESET_PURGE_RX | FTDI_SIO_RESET_PURGE_TX, ((0x00) << 8) | CHANNEL_B, 0);
      ftdi_control(FTDI_SIO_SET_DATA_REQUEST_TYPE, FTDI_SIO_SET_DATA, data_size | FTDI_SIO_SET_DATA_STOP_BITS_1 | FTDI_SIO_SET_DATA_PARITY_NONE, ((0x00) << 8) | CHANNEL_B, 0);
      ftdi_control(FTDI_SIO_SET_BAUDRATE_REQUEST_TYPE, FTDI_SET_BAUD_RATE, FTDI_BAUD_DIVISOR, ((0x02) << 8) | CHANNEL_B, 0);
      ftdi_control(FTDI_SIO_SET_FLOW_CTRL_REQUEST_TYPE, FTDI_SIO_SET_FLOW_CTRL, ((XOFF) << 8) | XON, (flowcontrol << 8) | CHANNEL_B, 0);  
      return;
}

void ftdi_uart_write(uint8_t* msg, uint16_t len)
{
	arty_transfer_data(msg, len, uart_ep_wr);
        // ADDBACK Arty->SndData(uart_ep_wr, len, msg);
        return;
}
        
uint16_t ftdi_uart_read(uint8_t* buf)
{
      uint16_t recv = 0;
      uint8_t ibuf[256];
      recv = arty_receive_data(ibuf, 1, uart_ep_rd);
      // ADDBACK Arty->RcvData(uart_ep_rd, &recv, ibuf);
       
      for (int i =0; i < recv; i++){buf[i] = ibuf[i];}
       return recv;
}

void ftdi_mpsse_write(uint8_t* msg, uint16_t len)
{
	arty_transfer_data(msg, len, mpsse_ep_wr);
        // ADDBACK Arty->SndData(mpsse_ep_wr, len, msg);
        return;
}
        
uint16_t ftdi_mpsse_read(uint8_t* buf)
{
      uint16_t recv = 0;
      uint8_t ibuf[256];
      recv = arty_receive_data(ibuf, 256, mpsse_ep_rd);
       // ADDBACK Arty->RcvData(mpsse_ep_rd, &recv, ibuf);
      for (int i =0; i < recv; i++){buf[i] = ibuf[i];}
      return recv;
}
