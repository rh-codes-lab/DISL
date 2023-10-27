import usb.core
import usb.util
import math
import sys
import copy
import time
import array


class JTAG_CMD:
    def __init__(self, type_, desc):
        self.type = type_ # statemove, stableclocks, sleep, scan, pathmove, tms, runtest
        if type_ == 'JTAG_STATEMOVE':
            self.statemove = desc
            #struct statemove_command {  
	        #       tap_state_t end_state; /** state in which JTAG commands should finish */
        elif type_ == 'JTAG_STABLECLOCKS':
            self.stableclocks = desc
            #struct stableclocks_command {
            #       int num_cycles; 	/** number of clock cycles that should be sent */
        elif type_ == 'JTAG_SLEEP':
            self.sleep = desc
            #struct sleep_command {   
	        #      uint32_t us; /** number of microseconds to sleep */
        elif type_ == 'JTAG_SCAN':
            self.scan = desc
            #struct scan_command {      
	        #        bool ir_scan; /** instruction/not data scan */
	        #        int num_fields; /** number of fields in *fields array *
	        #        struct scan_field *fields; /** pointer to an array of data scan fields */
	        #        tap_state_t end_state; /** state in which JTAG commands should finish */
	        # -
            #struct scan_field { 
	        #        int num_bits;  /** The number of bits this field specifies */
	        #        const uint8_t *out_value; /** A pointer to value to be scanned into the device */
	        #        uint8_t *in_value; /** A pointer to a 32-bit memory location for data scanned out */
	        #        uint8_t *check_value; /** The value used to check the data scanned out. */
	        #        uint8_t *check_mask; /** The mask to go with check_value */
        elif type_ == 'JTAG_PATHMOVE':
            self.pathmove = desc
	        #struct pathmove_command {
	        #       int num_states; /** number of states in *path */
	        #       tap_state_t *path; /** states that have to be passed */
        elif type_ == 'JTAG_TMS':
            self.tms = desc
            #struct tms_command {
	        #       unsigned num_bits; 	/** How many bits should be clocked out. */
	        #       const uint8_t *bits; /** The bits to clock out; the LSB is bit 0 of bits[0]. */
        elif type_ == 'JTAG_RUNTEST':
            self.runtest = desc
	        #struct runtest_command {
	        #       int num_cycles; /** number of cycles to spend in Run-Test/Idle state */
	        #       tap_state_t end_state; /** state in which JTAG commands should finish */
        else:
            assert 0, "Invalid JTAG command"

class DEVICE:
    def __init__ (self, idVendor ,idProduct):
        self.dev = usb.core.find(idVendor=idVendor, idProduct=idProduct)
        if self.dev is None:
            raise ValueError('Our device is not connected')
        for i in range(2):
            if self.dev.is_kernel_driver_active(i):
                reattach = True
                self.dev.detach_kernel_driver(i)
        self.cfg = self.dev.get_active_configuration()
        self.mpsse_intfc = self.cfg[(0,0)]
        self.uart_intfc = self.cfg[(1,0)]
        self.mpsse_ep_wr = (usb.util.find_descriptor(self.mpsse_intfc, custom_match=lambda e: not (e.bEndpointAddress & 0x80))).bEndpointAddress
        self.mpsse_ep_rd = (usb.util.find_descriptor(self.mpsse_intfc, custom_match=lambda e: (e.bEndpointAddress & 0x80))).bEndpointAddress
        self.uart_ep_wr = (usb.util.find_descriptor(self.uart_intfc, custom_match=lambda e: not (e.bEndpointAddress & 0x80))).bEndpointAddress
        self.uart_ep_rd = (usb.util.find_descriptor(self.uart_intfc, custom_match=lambda e: (e.bEndpointAddress & 0x80))).bEndpointAddress
        self.uart_ep_rd_wMaxPacketSize = (usb.util.find_descriptor(self.uart_intfc, custom_match=lambda e: (e.bEndpointAddress & 0x80))).wMaxPacketSize
        # Commands from: ftdi_sio.h.
        self.FTDI_SIO_RESET_REQUEST_TYPE = 0x40
        self.FTDI_SIO_SET_BAUDRATE_REQUEST_TYPE = 0x40
        self.FTDI_SIO_SET_DATA_REQUEST_TYPE = 0x40
        self.FTDI_SIO_SET_FLOW_CTRL_REQUEST_TYPE = 0x40
        self.FTDI_SIO_SET_MODEM_CTRL_REQUEST_TYPE = 0x40
        self.FTDI_SIO_GET_LATENCY_TIMER_REQUEST_TYPE = 0xC0
        self.FTDI_SIO_SET_LATENCY_TIMER_REQUEST_TYPE = 0x40
        self.FTDI_SIO_SET_EVENT_CHAR_REQUEST_TYPE = 0x40
        self.FTDI_SIO_GET_MODEM_STATUS_REQUEST_TYPE = 0xc0
        self.FTDI_SIO_SET_BITMODE_REQUEST_TYPE = 0x40
        self.FTDI_SIO_READ_EEPROM_REQUEST_TYPE = 0xc0
        # Requests
        self.FTDI_SIO_RESET = 0
        self.FTDI_SIO_MODEM_CTRL = 1
        self.FTDI_SIO_SET_FLOW_CTRL = 2
        self.FTDI_SET_BAUD_RATE = 3
        self.FTDI_SIO_SET_DATA = 4
        # Channels
        self.CHANNEL_A = 1
        self.CHANNEL_B = 2
        # Values
        self.FTDI_SIO_RESET_PURGE_RX = 1
        self.FTDI_SIO_RESET_PURGE_TX = 2
        self.FTDI_SIO_SET_DATA_STOP_BITS_1 = 0x0 << 11
        self.FTDI_SIO_SET_DATA_PARITY_NONE = 0x0 << 8
        self.FTDI_BASECLOCK = 120000000
        self.FTDI_SIO_DISABLE_FLOW_CTRL = 0x0
        self.FTDI_SIO_RTS_CTS_HS  = (0x1 << 8)
        self.FTDI_SIO_DTR_DSR_HS  = (0x2 << 8)
        self.FTDI_SIO_XON_XOFF_HS = (0x4 << 8)
        self.XOFF = 0x13
        self.XON = 0x11
        self.FTDI_FLOW_CONTROL = {"NONE": 0, "RTS/CTS": 1, "DTR/DSR": 2, "XON/XOFF": 4}
        
    def control(self, bmRequestType, bmRequest, wValue, wIndex, packet):
        return self.dev.ctrl_transfer(bmRequestType, bmRequest, wValue, wIndex, packet)
        
    def free_uart(self):
        self.dev.reset()
        self.dev.attach_kernel_driver(1)
        
    def uart_configure(self, data_size=8, baudrate=1000000, flowcontrol="XON/XOFF"):
        divfrac = [ 0, 3, 2, 4, 1, 5, 6, 7 ]
        divisor3 = round((8 * self.FTDI_BASECLOCK)/(10 * baudrate))
        divisor = divisor3 >> 3
        divisor |= divfrac[divisor3 & 0x7] << 14
        if (divisor == 1):
            divisor = 0
        elif (divisor == 0x4001):
            divisor = 1
        FTDI_BAUD_DIVISOR = divisor
        self.control(self.FTDI_SIO_RESET_REQUEST_TYPE, self.FTDI_SIO_RESET, 
                                self.FTDI_SIO_RESET_PURGE_RX | self.FTDI_SIO_RESET_PURGE_TX, 
                                ((0x00) << 8) | self.CHANNEL_B, 0)
        self.control(self.FTDI_SIO_SET_DATA_REQUEST_TYPE, self.FTDI_SIO_SET_DATA, 
                                data_size | self.FTDI_SIO_SET_DATA_STOP_BITS_1 | self.FTDI_SIO_SET_DATA_PARITY_NONE, 
                                ((0x00) << 8) | self.CHANNEL_B, 0)
        self.control(self.FTDI_SIO_SET_BAUDRATE_REQUEST_TYPE, self.FTDI_SET_BAUD_RATE, FTDI_BAUD_DIVISOR, 
                                ((0x02) << 8) | self.CHANNEL_B, 0)
        self.control(self.FTDI_SIO_SET_FLOW_CTRL_REQUEST_TYPE, self.FTDI_SIO_SET_FLOW_CTRL,   
                                ((self.XOFF) << 8) | self.XON, ((self.FTDI_FLOW_CONTROL[flowcontrol]) << 8) | self.CHANNEL_B, 0)
        
    def uart_write(self, msg=[], timeout=100):
        self.dev.write(self.uart_ep_wr, msg, timeout)
        
    def uart_read(self, is_str=1, timeout=100):
        buf = array.array('b',[0]*self.uart_ep_rd_wMaxPacketSize)
        size = self.dev.read(self.uart_ep_rd, buf, timeout)
        if size == 2:
            return ''
        if is_str:
            return ''.join([chr(x) for x in buf[2:size]])
        else:
            return buf[2:size]
        
    def mpsse_write(self, msg, timeout):
        self.dev.write(self.mpsse_ep_wr, msg, timeout)
        
    def mpsse_read(self, buf, timeout):
        size = self.dev.read(self.mpsse_ep_rd, buf, timeout)
        return size
 
    def free_dev(self):
        self.free_uart()
        usb.util.dispose_resources(self.dev)
        
class MPSSE:
    def __init__(self, dev):
        self.POS_EDGE_OUT = 0x00
        self.NEG_EDGE_OUT = 0x01
        self.POS_EDGE_IN = 0x00
        self.NEG_EDGE_IN = 0x04
        self.MSB_FIRST = 0x00
        self.LSB_FIRST = 0x08     
        self.BITMODE_MPSSE  = 0x02
        self.SIO_RESET_REQUEST = 0x00
        self.SIO_SET_LATENCY_TIMER_REQUEST = 0x09
        self.SIO_GET_LATENCY_TIMER_REQUEST = 0x0A
        self.SIO_SET_BITMODE_REQUEST = 0x0B
        self.SIO_RESET_SIO = 0
        self.SIO_RESET_PURGE_RX = 1
        self.SIO_RESET_PURGE_TX = 2
        ctx = {}
        ctx["usb_dev"] = dev
        ctx["usb_write_timeout"] = 0
        ctx["usb_read_timeout"] = 0
        ctx["max_packet_size"] = 0
        ctx["packet_size"] = 0
        ctx["ftdi_chip_type"] = "TYPE_FT2232H"
        ctx["write_buffer"] = []
        ctx["write_size"] = 0
        ctx["write_count"] = 0
        ctx["read_buffer"] = []
        ctx["read_size"] = 0
        ctx["read_count"] = 0
        ctx["read_count_bits"] = 0
        ctx["read_chunk"] = []
        ctx["read_chunk_size"] = 0
        ctx["read_queue"] = {}
        ctx["retval"] = 0
        ctx["transferred"] = 0
        self.ctx = ctx
    
    def mpsse_open(self):
        self.ctx["read_chunk_size"] = 16384
        self.ctx["read_size"] = 16384
        self.ctx["write_size"] = 16384
        self.ctx["usb_write_timeout"] = 5000
        self.ctx["usb_read_timeout"] = 5000
        self.ctx["max_packet_size"] = 0x200 #512 bytes
        self.ctx["packet_size"] = 0x100 # 256 bytes
        resp =  self.ctx["usb_dev"].control(0x40, 0, 0, 1, 0)
        resp =  self.ctx["usb_dev"].control(0x40, 9, 255, 1, 0)
        resp =  self.ctx["usb_dev"].control(0x40, 11, 0x020b, 1, 0)
        resp =  self.ctx["usb_dev"].control(0x40, 0, 1, 1, 0)
        resp =  self.ctx["usb_dev"].control(0x40, 0, 2, 1, 0)
        resp =  self.ctx["usb_dev"].mpsse_write([0x80, 0x88, 0x8b, 0x82, 0x00, 0x00, 0x85, 0x97, 0x8A, 0x86, 0x02, 0x00],100)   
        resp =  self.ctx["usb_dev"].mpsse_write([0x97, 0x8A, 0x86, 0x02, 0x00],100)  
        resp =  self.ctx["usb_dev"].mpsse_write([0x4B, 0x06, 0x7F],100)  
    def mpsse_is_high_speed(self):
        return (0 if self.ctx["type"] == "TYPE_FT2232C" else 1)
        
    def mpsse_purge(self):
        resp =  self.ctx["usb_dev"].control(0x40, 0, 1, 1, 0)
        resp =  self.ctx["usb_dev"].control(0x40, 0, 2, 1, 0)
        
    def buffer_write_space(self):
        return (self.ctx["write_size"] - self.ctx["write_count"] - 1) #Reserve one byte for SEND_IMMEDIATE
        
    def buffer_read_space(self):
        return (self.ctx["read_size"] - self.ctx["read_count"])
        
    def buffer_write_byte(self, data):
        assert (self.ctx["write_count"] < self.ctx["write_size"])
        self.ctx["write_buffer"].append(data)
        self.ctx["write_count"] += 1
    
    def DIV_ROUND_UP(self, int_1, int_2):
        return (int(math.ceil(int_1/int_2)))  
        
    def bit_copy(self,dst, dst_start, src, src_start, bit_count):
        db = dst_start / 8
        sb = src_start / 8
        dq = dst_start % 8
        sq = src_start % 8
        lb = bit_count / 8
        lq = bit_count % 8
        for i in range(int(db)):
            dst.append(0)
        if (sq == 0) and (lq == 0) and (dq == 0):
            for i in range(int(lb)):
                dst.append(src[int(sb)+i])
            return dst
        idx = int(sb)
        odx = 0
        dst.append(0)
        for i in range(bit_count):
            if (((src[idx] >> (sq&7)) & 1) == 1):
                dst[odx] |= 1 << (dq&7)
            else:
                dst[odx] &= ~(1 << (dq&7))
            sq += 1
            if (sq == 7):
                sq = 0
                idx += 1
            dq += 1
            if (dq == 7):
                dq = 0
                odx += 1
                dst.append(0)
        if dq == 0:
            dst = dst[:-1]
        return dst
        
    def buffer_write(self, out, out_offset, bit_count):
        assert (self.ctx["write_count"] + self.DIV_ROUND_UP(bit_count,8) <= self.ctx["write_size"])
        self.ctx["write_buffer"].extend(self.bit_copy([], 0, out, out_offset, bit_count))
        self.ctx["write_count"] += self.DIV_ROUND_UP(bit_count, 8)
        return bit_count
        
    def buffer_add_read(self, bit_count):
        self.ctx["read_count_bits"] += bit_count
        assert (self.DIV_ROUND_UP(self.ctx["read_count_bits"],8) <= self.ctx["read_size"])
        self.ctx["read_count"] =  self.DIV_ROUND_UP(self.ctx["read_count_bits"],8)
        return bit_count
    
    def write_transfer(self):
        remaining_bytes = self.ctx["write_count"]
        self.ctx["transferred"] = 0
        while (remaining_bytes > 0):
            if remaining_bytes < self.ctx["packet_size"]:
                chunk = self.ctx["write_buffer"][self.ctx["transferred"]:]
                self.ctx["usb_dev"].mpsse_write(bytearray(chunk),self.ctx["usb_write_timeout"])
                remaining_bytes = 0
            else:
                chunk = self.ctx["write_buffer"][self.ctx["transferred"]:self.ctx["transferred"]+self.ctx["packet_size"]]
                self.ctx["usb_dev"].mpsse_write(bytearray(chunk),self.ctx["usb_write_timeout"])
                self.ctx["transferred"] += self.ctx["packet_size"]
                remaining_bytes -= self.ctx["packet_size"]
        self.ctx["write_buffer"] = []
        self.ctx["write_count"] = 0
        self.ctx["transferred"] = 0
        return

    def read_transfer(self):
        remaining_bytes = self.ctx["read_count"]
        while (remaining_bytes > 0):
            chunk = array.array('b',[0]*self.ctx["max_packet_size"])
            size = self.ctx["usb_dev"].mpsse_read(chunk,self.ctx["usb_read_timeout"])
            chunk = chunk[2:]
            for i in range(size-2):
                self.ctx["read_buffer"].append(chunk[i])
            remaining_bytes -= (size-2)
        self.ctx["read_count_bits"] = 0
        self.ctx["read_count"] = 0
        self.ctx["read_chunk"] = []
        self.ctx["read_chunk_size"] = 0
        self.ctx["transferred"] = 0
        return
        
    def mpsse_flush(self):
        retval = self.ctx["retval"]
        assert(self.ctx["write_count"] > 0 or self.ctx["read_count"] == 0) # No read data without write data
        if (self.ctx["write_count"] == 0):
            return retval
        if self.ctx["read_count"]:
            self.buffer_write_byte(0x87)
        retval = self.write_transfer()
        if self.ctx["read_count"]:
            retval = self.read_transfer()
        return retval
        
    def mpsse_clock_data_out(self, out, out_offset, length, mode):
        self.mpsse_clock_data(out, out_offset, [], 0, length, mode)  
        
    def mpsse_clock_data_in(self, in_, in_offset, length, mode):
        self.mpsse_clock_data([], 0, in_, in_offset, length, mode)     
    
    def mpsse_clock_data(self, out, out_offset, in_, in_offset, length, mode):
        if (out or ((not out) and (not in_))):
            mode |= 0x10
        if in_:
            mode |= 0x20
        while (length > 0):
            if ((self.buffer_write_space() + (1 if length < 8 else 0)) < (4 if (out or ((not out) and (not in_))) else 3)) or (in_ and (self.buffer_read_space() < 1)):
                self.ctx["retval"] = self.mpsse_flush()
            if (length < 8):
                self.buffer_write_byte(0x02 | mode)
                self.buffer_write_byte(length-1)
                if out:
                    out_offset += self.buffer_write(out, out_offset, length)
                if in_:
                    in_offset += self.buffer_add_read(length)
                if (not out) and (not in_):
                    self.buffer_write_byte(0x00)
                length = 0
            else:
                this_bytes = int(length/8)
                if (this_bytes > 65536):
                    this_bytes = 65536
                if (out or ((not out) and (not in_))) and ((this_bytes + 3) > self.buffer_write_space()):
                    this_bytes = self.buffer_write_space() - 3
                if in_ and (this_bytes > self.buffer_read_space()):
                    this_bytes = self.buffer_read_space()
                if (this_bytes > 0):
                    self.buffer_write_byte(mode)
                    self.buffer_write_byte((this_bytes - 1) & 0xff)
                    self.buffer_write_byte((this_bytes - 1) >> 8)
                    if out:
                        out_offset += self.buffer_write(out, out_offset, this_bytes * 8)
                    if in_:
                        in_offset += self.buffer_add_read(this_bytes * 8)
                    if (not out) and (not in_):
                        for n in range(this_bytes):
                            self.buffer_write_byte(0x00)
                    length -= this_bytes * 8

    def mpsse_clock_tms_cs_out(self, out, out_offset, length, tdi, mode):
        self.mpsse_clock_tms_cs(out, out_offset, [], 0, length, tdi, mode)

    def mpsse_clock_tms_cs(self, out, out_offset, in_, in_offset, length, tdi, mode):
        assert(out)
        mode |= 0x42
        if (in_):
            mode |= 0x20

        while (length > 0):
            if (self.buffer_write_space() < 3 or (in_ and (self.buffer_read_space() < 1))):
                self.ctx["retval"] = self.mpsse_flush()
            this_bits = length
            if (this_bits > 7):
                this_bits = 7

            if (this_bits > 0):
                self.buffer_write_byte(mode)
                self.buffer_write_byte(this_bits - 1)
                data = 0
                data = self.bit_copy([], 0, out, out_offset, this_bits)[0]
                out_offset += this_bits
                self.buffer_write_byte(data | ( 0x80 if tdi else 0x00))
                if (in_):
                    in_offset += self.buffer_add_read(this_bits)
                length -= this_bits    
    
    def mpsse_set_data_bits_low_byte(self, data, dir_):
        if (self.buffer_write_space() < 3):
            self.ctx["retval"] = self.mpsse_flush()
        self.buffer_write_byte(0x80)
        self.buffer_write_byte(data)
        self.buffer_write_byte(dir_)
    
    def mpsse_set_data_bits_high_byte(self, data, dir_):
        if (self.buffer_write_space() < 3):
            self.ctx["retval"] = self.mpsse_flush()
        self.buffer_write_byte(0x82)
        self.buffer_write_byte(data)
        self.buffer_write_byte(dir_)
    
    def mpsse_read_data_bits_low_byte(self, data):
        if (self.buffer_write_space() < 1 or self.buffer_read_space() < 1):
            self.ctx["retval"] = self.mpsse_flush()
        self.buffer_write_byte(0x81)
        self.buffer_add_read(data, 0, 8, 0)
    
    def mpsse_read_data_bits_high_byte(self, data):
        if (self.buffer_write_space() < 1 or self.buffer_read_space() < 1):
            self.ctx["retval"] = self.mpsse_flush()
        self.buffer_write_byte(0x83)
        self.buffer_add_read(data, 0, 8, 0)    
    
    def single_byte_boolean_helper(self, var, var_if_true, var_if_false):
        if (self.buffer_write_space() < 1):
            self.ctx["retval"] = self.mpsse_flush()
        self.buffer_write_byte(val_if_true if var else val_if_false)

    def mpsse_loopback_config(self, enable):
        self.single_byte_boolean_helper(enable, 0x84, 0x85)
        
        
class FTDI:
    def __init__(self, idVendor ,idProduct, current_state):
        self.dev = DEVICE(idVendor, idProduct)
        self.mpsse_layer = MPSSE(self.dev)
        self.mpsse_layer.mpsse_open()
        self.current_state = current_state
        self.target_state = current_state
        self.ftdi_jtag_mode = (self.mpsse_layer.LSB_FIRST | self.mpsse_layer.POS_EDGE_IN | self.mpsse_layer.NEG_EDGE_OUT)
        self.tms_seqs = [['1111111', '0000000', '1110100', '0101000', '1101100', '0110100'],
                         ['1111111', '0000000', '100', '1010', '1100', '11010'], 
                         ['1111111', '110', '11100', '10', '111100', '1111010'], 
                         ['1111111', '110', '10', '0', '111100', '1111010'], 
                         ['1111111', '110', '11100', '111010', '111100', '10'], 
                         ['1111111', '110', '11100', '111010', '10', '0']]
        if self.current_state == "":
            tms_count = 7
            tms_bits = "1111111"
            self.current_state = "TAP_RESET"
            self.target_state = "TAP_RESET"
            self.mpsse_layer.mpsse_clock_tms_cs_out([int(tms_bits[::-1],2)], 0, tms_count, 0, self.ftdi_jtag_mode)
        
    def tap_get_state(self):
        return self.current_state
        
    def tap_set_state(self, state):
        self.current_state = state
        
    def tap_get_end_state(self):
        return self.target_state
    
    def tap_set_end_state(self, state):
        self.target_state = state
     
    def tap_is_state_stable(self, state):
        if state in ["TAP_RESET", "TAP_IDLE", "TAP_DRSHIFT", "TAP_DRPAUSE", "TAP_IRSHIFT", "TAP_IRPAUSE"]:
            return 1
        else:
            return 0
            
    def tap_get_state_enum(self, state):
        if state == "RESET":
            return 0
        elif state == "RUN/IDLE":
            return 1
        elif state == "DRSHIFT":
            return 2
        elif state == "DRPAUSE":
            return 3
        elif state == "IRSHIFT":
            return 4
        else: #IRPAUSE
            return 5
            
    def tap_state_transition(self, cur_state, tms):
        new_state = ""
        if tms:
            if cur_state == "TAP_RESET":
                new_state = cur_state
            elif cur_state in ["TAP_IDLE", "TAP_DRUPDATE", "TAP_IRUPDATE"]:
                new_state = "TAP_DRSELECT"
            elif cur_state == "TAP_DRSELECT":
                new_state = "TAP_IRSELECT"
            elif cur_state in ["TAP_DRCAPTURE", "TAP_DRSHIFT"]:
                new_state = "TAP_DREXIT1"
            elif cur_state in ["TAP_DREXIT1", "TAP_DREXIT2"]:
                new_state = "TAP_DRUPDATE"
            elif cur_state == "TAP_DRPAUSE":
                new_state = "TAP_DREXIT2"
            elif cur_state == "TAP_IRSELECT":
                new_state = "TAP_RESET"
            elif cur_state in ["TAP_IRCAPTURE", "TAP_IRSHIFT"]:
                new_state = "TAP_IREXIT1"
            elif cur_state in ["TAP_IREXIT1", "TAP_IREXIT2"]:
                new_state = "TAP_IRUPDATE"
            elif cur_state == "TAP_IRPAUSE":
                new_state = "TAP_IREXIT2"
            else:
                assert 0, "fatal: invalid argument cur_state= " + cur_state
        else:
            if cur_state in ["TAP_RESET", "TAP_IDLE", "TAP_DRUPDATE", "TAP_IRUPDATE"]:
                new_state = "TAP_IDLE"
            elif cur_state == "TAP_DRSELECT":
                new_state = "TAP_DRCAPTURE"
            elif cur_state in ["TAP_DRCAPTURE", "TAP_DRSHIFT", "TAP_DREXIT2"]:
                new_state = "TAP_DRSHIFT"
            elif cur_state in ["TAP_DREXIT1", "TAP_DRPAUSE"]:
                new_state = "TAP_DRPAUSE"
            elif cur_state == "TAP_IRSELECT":
                new_state = "TAP_IRCAPTURE"
            elif cur_state in ["TAP_IRCAPTURE", "TAP_IRSHIFT", "TAP_IREXIT2"]:
                new_state = "TAP_IRSHIFT"
            elif cur_state in ["TAP_IREXIT1", "TAP_IRPAUSE"]:
                new_state = "TAP_IRPAUSE"
            else:
                assert 0, "fatal: invalid argument cur_state= " + cur_state
        return new_state
        
    def tap_name_mapping(self, state):
        if state == "TAP_RESET":
            return "RESET"
        elif state == "TAP_IDLE":
            return "RUN/IDLE"
        elif state == "TAP_DRSELECT":
            return "DRSELECT"
        elif state == "TAP_DRCAPTURE":
            return "DRCAPTURE"
        elif state == "TAP_DRSHIFT":
            return "DRSHIFT"
        elif state == "TAP_DREXIT1":
            return "DREXIT1"
        elif state == "TAP_DRPAUSE":
            return "DRPAUSE"
        elif state == "TAP_DREXIT2":
            return "DREXIT2"
        elif state == "TAP_DRUPDATE":
            return "DRUPDATE"
        elif state == "TAP_IRSELECT":
            return "IRSELECT"
        elif state == "TAP_IRCAPTURE":
            return "IRCAPTURE"
        elif state == "TAP_IRSHIFT":
            return "IRSHIFT"
        elif state == "TAP_IREXIT1":
            return "IREXIT1"
        elif state == "TAP_IRPAUSE":
            return "IRPAUSE"
        elif state == "TAP_IREXIT2":
            return "IREXIT2"
        elif state == "TAP_IRUPDATE":
            return "IRUPDATE"
        else:
            return "IDLE"

    def tap_get_tms_path(self, start, end):
        return self.tms_seqs[self.tap_get_state_enum(self.tap_name_mapping(start))][self.tap_get_state_enum(self.tap_name_mapping(end))]
        
    def tap_get_tms_path_len(self, start, end):
        return len(self.tap_get_tms_path(start, end)) 

    def move_to_state(self, goal_state):
        start_state = self.tap_get_state()
        tms_bits  = self.tap_get_tms_path(start_state, goal_state)
        tms_count = self.tap_get_tms_path_len(start_state, goal_state)
        assert(tms_count <= 8)
        for i in range(tms_count):
            self.tap_set_state(self.tap_state_transition(self.tap_get_state(), (int(tms_bits[i])) & 1))    
        self.mpsse_layer.mpsse_clock_tms_cs_out([int(tms_bits[::-1],2)], 0, tms_count, 0, self.ftdi_jtag_mode)
    
    def ftdi_end_state(self, state):
        assert self.tap_is_state_stable(state)
        self.tap_set_end_state(state)
 
    def ftdi_execute_runtest(self, cmd):
        zero = [0]
        if (self.tap_get_state() != "TAP_IDLE"):
            self.move_to_state("TAP_IDLE")
        i = cmd.runtest["num_cycles"]
        while (i > 0):
            this_len = 7  if i > 7 else i
            self.mpsse_clock_tms_cs_out(zero, 0, this_len, 0, self.ftdi_jtag_mode)
            i -= this_len
        self.ftdi_end_state(cmd.runtest["end_state"])
        if (self.tap_get_state() != self.tap_get_end_state()):
            self.move_to_state(self.tap_get_end_state())
  
    def ftdi_execute_statemove(self, cmd):
        self.ftdi_end_state(cmd.statemove["end_state"])
        if (self.tap_get_state() != self.tap_get_end_state() or self.tap_get_end_state() == "TAP_RESET"):
            self.move_to_state(self.tap_get_end_state())
 
    def ftdi_execute_tms(self, cmd):
        mpsse_clock_tms_cs_out(cmd.tms["bits"], 0, cmd.tms["num_bits"], 0, self.ftdi_jtag_mode)

    def ftdi_execute_pathmove(self, cmd):
        path = cmd.pathmove["path"]
        num_states  = cmd.pathmove["num_states"]
        state_count = 0
        bit_count = 0
        tms_byte = 0
        while (num_states):
            num_states -= 1
            if (self.tap_state_transition(self.tap_get_state(), 0) == path[state_count]):
                bit_count += 1
            elif (self.tap_state_transition(self.tap_get_state(), 1)  == path[state_count]):
                tms_byte |= 1 << bit_count
            else:
                assert 0, "Invalid path: " + str(self.tap_get_state()) + " -> " + str(path[state_count])
            self.tap_set_state(path[state_count])
            state_count+=1
            if (bit_count == 7 or num_states == 0):
                self.mpsse_layer.mpsse_clock_tms_cs_out([tms_byte], 0, bit_count, 0, self.ftdi_jtag_mode)
                bit_count = 0
        self.tap_set_end_state(self.tap_get_state())
 
    def ftdi_execute_scan(self, cmd):
        while (cmd.scan["num_fields"] > 0 and len(cmd.scan["fields"][cmd.scan["num_fields"] - 1]) == 0):
            cmd.scan["num_fields"] -= 1
        if (cmd.scan["num_fields"] == 0):
            return
        if (cmd.scan["ir_scan"]): 
            if (self.tap_get_state() != "TAP_IRSHIFT"):
                self.move_to_state("TAP_IRSHIFT")
        else:
            if (self.tap_get_state() != "TAP_DRSHIFT"):
                self.move_to_state("TAP_DRSHIFT")
        self.ftdi_end_state(cmd.scan["end_state"])
        scan_size = 0
        for i in range(cmd.scan["num_fields"]):
            field = cmd.scan["fields"][i]
            scan_size += field["num_bits"]
            if (i == cmd.scan["num_fields"] - 1 and self.tap_get_state() != self.tap_get_end_state()):
                self.mpsse_layer.mpsse_clock_data(field["out_value"], 0, field["in_value"], 0, field["num_bits"] - 1, self.ftdi_jtag_mode)
                last_bit = self.mpsse_layer.bit_copy([], 0, field["out_value"], field["num_bits"]-1, 1)[0] if field['out_value'] else 0
                tms_bits = [0x03]
                self.mpsse_layer.mpsse_clock_tms_cs(tms_bits, 0, field["in_value"], field["num_bits"] - 1, 1, last_bit, self.ftdi_jtag_mode)
                self.tap_set_state(self.tap_state_transition(self.tap_get_state(), 1))
                if (self.tap_get_end_state() == "TAP_IDLE"):
                    self.mpsse_layer.mpsse_clock_tms_cs_out(tms_bits, 1, 2, last_bit, self.ftdi_jtag_mode)
                    self.tap_set_state(self.tap_state_transition(self.tap_get_state(), 1))
                    self.tap_set_state(self.tap_state_transition(self.tap_get_state(), 0))
                else:
                    self.mpsse_layer.mpsse_clock_tms_cs_out(tms_bits, 2, 1, last_bit, self.ftdi_jtag_mode)
                    self.tap_set_state(self.tap_state_transition(self.tap_get_state(), 0))
            else:
                self.mpsse_layer.mpsse_clock_data(field["out_value"], 0, field["in_value"], 0, field["num_bits"], self.ftdi_jtag_mode)
        if (self.tap_get_state() != self.tap_get_end_state()):
            self.move_to_state(self.tap_get_end_state())


    def ftdi_execute_sleep(self, cmd):
        self.mpsse_layer.mpsse_flush()
        time.sleep(cmd.sleep["us"]/1000000.0)
        
    def ftdi_execute_stableclocks(self, cmd):
        num_cycles = cmd.stableclocks["num_cycles"]
        tms = 0x7f if self.tap_get_state() == TAP_RESET else 0x00
        while (num_cycles > 0):
            this_len = 7 if num_cycles > 7 else num_cycles
            self.mpsse_layer.mpsse_clock_tms_cs_out(tms, 0, this_len, 0, self.ftdi_jtag_mode)
            num_cycles -= this_len

    def ftdi_execute_command(self, cmd):
        if cmd.type == "JTAG_RUNTEST":
            self.ftdi_execute_runtest(cmd)
        elif cmd.type == "JTAG_TLR_RESET":
            self.ftdi_execute_statemove(cmd)
        elif cmd.type == "JTAG_PATHMOVE":
            self.ftdi_execute_pathmove(cmd)
        elif cmd.type == "JTAG_STATEMOVE":
            self.ftdi_execute_statemove(cmd)
        elif cmd.type == "JTAG_SCAN":
            self.mpsse_layer.ctx["read_buffer"] = []  
            self.ftdi_execute_scan(cmd)
        elif cmd.type == "JTAG_SLEEP":
            self.ftdi_execute_sleep(cmd)
        elif cmd.type == "JTAG_STABLECLOCKS":
            self.ftdi_execute_stableclocks(cmd)
        elif cmd.type == "JTAG_TMS":
            self.ftdi_execute_tms(cmd)
        else:
            assert 0, "BUG: unknown JTAG command type encountered: " + cmd.type
        retval = self.mpsse_layer.mpsse_flush()
        return copy.deepcopy(self.mpsse_layer.ctx["read_buffer"]) if cmd.type == "JTAG_SCAN" else []
        
    def ftdi_execute_queue(self,jtag_command_queue):
        read_buffer = []
        for cmd in jtag_command_queue:
            read_buffer.extend(self.ftdi_execute_command(cmd))
        return read_buffer
        
    def free_dev(self):
        self.dev.free_dev()
