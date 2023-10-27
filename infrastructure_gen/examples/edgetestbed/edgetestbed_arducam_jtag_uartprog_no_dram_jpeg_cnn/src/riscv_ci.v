module laplacian_rgb565_rv32_pcpi_full(
input clk,
input        	pcpi_valid,
input [31:0] 	pcpi_insn,
input [31:0] 	pcpi_rs1,
input [31:0] 	pcpi_rs2,
output       	pcpi_wr,
output  [31:0] 	pcpi_rd,
output       	pcpi_wait,
output 	    	pcpi_ready,
output uart_tx,
output spi_sck,
output spi_mosi,
output spi_cs,
input spi_miso,
output reg busy,
output [7:0] debug
);


parameter OPCODE = 127;
parameter IMAGE_WIDTH = 320;
parameter IMAGE_HEIGHT = 240;
parameter UART_TX_CLKS_PER_BIT  = 16'd83;
parameter SPI_CLOCK_DIVISOR = 0;


wire [2:0] cmd = pcpi_insn[14:12];
wire cmd_reset = (cmd == 3'd0) ? 1'b1 : 1'b0;
wire cmd_set_threshold = (cmd == 3'd1) ? 1'b1 : 1'b0;
wire cmd_read_frame_buffer = (cmd == 3'd2) ? 1'b1 : 1'b0;
wire cmd_start = (cmd == 3'd3) ? 1'b1 : 1'b0;
wire cmd_poll = (cmd == 3'd4) ? 1'b1 : 1'b0;

wire valid_insn;
wire valid_data;
wire isEdge;

reg [31:0] div_clk;
reg [15:0] threshold;
reg [15:0] col_counter;
reg [15:0] row_counter;
reg [20:0] timeout;


reg [7:0] frame_buffer [0: ((IMAGE_WIDTH*IMAGE_HEIGHT)>>3) - 1];
reg [7:0] frame_buffer_write_word;
reg [7:0] frame_buffer_read_word;
reg [15:0] frame_buffer_write_pointer;
reg [15:0] frame_buffer_read_pointer;


reg [(3*IMAGE_WIDTH*8)+1:0] array;
reg [2:0] bit_counter;
wire [15:0] Gray_tl;
wire [15:0] Gray_tc;
wire [15:0] Gray_tr;
wire [15:0] Gray_cl;
wire [15:0] Gray_cc;
wire [15:0] Gray_cr;
wire [15:0] Gray_bl;
wire [15:0] Gray_bc;
wire [15:0] Gray_br;

wire clk_spi;
wire rst_spi;
wire spi_busy;
wire spi_finish;
wire [7:0] spi_rx_byte;
reg spi_start_trigger;
reg [7:0] spi_state;
reg [7:0] spi_tx_data;

wire jpeg_core_out_valid;
wire [7:0] jpeg_core_out_pixel;
wire jpeg_core_accept;
wire jpeg_core_idle;
reg jpeg_core_input_valid;
reg [31:0] jpeg_core_input_word;
wire [31:0] buffer_pixel_id;
wire ovalid;
wire [7:0] rbpixel;


assign pcpi_wait = 0 ;
assign pcpi_wr = valid_insn;
assign pcpi_ready = valid_insn;
assign valid_insn = (pcpi_insn[6:0] == OPCODE[6:0]) ? pcpi_valid : 0;
assign pcpi_rd = (valid_insn && cmd_read_frame_buffer) ? {24'd0,frame_buffer_read_word} : {31'd0,busy};
assign rst_spi = (busy) ? 0 : 1;
assign valid_data = (row_counter > 0) ? 1'b1 : 1'b0;


generate 
	if (SPI_CLOCK_DIVISOR > 0) begin
		initial div_clk <= 0;
		assign clk_spi = div_clk[SPI_CLOCK_DIVISOR-1];  
		always @(posedge clk) begin
		    div_clk <= div_clk + 32'd1;
		end
		spi_burst_read SPI(
			.clk(clk_spi),
			.rst(rst_spi),
			.spi_clk(spi_sck),
			.spi_miso(spi_miso),
			.spi_mosi(spi_mosi),
			.spi_cs(spi_cs),

			.rx_data(spi_rx_byte),
			.busy(spi_busy),
			.tx_data(spi_tx_data),
			.trigger(spi_start_trigger),
			.finish(spi_finish)
		);
	end else begin
		spi_burst_read SPI(
			.clk(clk),
			.rst(rst_spi),
			.spi_clk(spi_sck),
			.spi_miso(spi_miso),
			.spi_mosi(spi_mosi),
			.spi_cs(spi_cs),

			.rx_data(spi_rx_byte),
			.busy(spi_busy),
			.tx_data(spi_tx_data),
			.trigger(spi_start_trigger),
			.finish(spi_finish)
		);
	end
endgenerate


jpeg_core
#(
      .SUPPORT_WRITABLE_DHT(1), .GRAYSCALE(1)
)
 uut (
     .clk_i(clk)
    ,.rst_i(!busy)
    ,.inport_valid_i(jpeg_core_input_valid)
    ,.inport_data_i(jpeg_core_input_word)
    ,.inport_strb_i(4'hF)
    ,.inport_last_i(0)
    ,.outport_accept_i(1)
    ,.inport_accept_o(jpeg_core_accept)
    ,.outport_valid_o(jpeg_core_out_valid)
    ,.outport_width_o()
    ,.outport_height_o()
    ,.outport_pixel_x_o()
    ,.outport_pixel_y_o()
    ,.outport_pixel_r_o(jpeg_core_out_pixel)
    ,.outport_pixel_g_o()
    ,.outport_pixel_b_o()
    ,.idle_o(jpeg_core_idle)
);


jpeg_reorder_buffer #(.IMAGE_W(IMAGE_WIDTH), .IMAGE_H(IMAGE_HEIGHT)) rb(
.clk(clk),
.rst(rst_spi),
.ivalid(jpeg_core_out_valid),
.ipixel(jpeg_core_out_pixel),
.opixel(rbpixel),
.pixel_id(buffer_pixel_id),
.ovalid(ovalid)
);

assign Gray_tl = {8'd0,array[8*(IMAGE_WIDTH+IMAGE_WIDTH+1)+:8]};
assign Gray_tc = {8'd0,array[8*(IMAGE_WIDTH+IMAGE_WIDTH)+:8]};
assign Gray_tr = {8'd0,array[8*(IMAGE_WIDTH+IMAGE_WIDTH-1)+:8]};
assign Gray_cl = {8'd0,array[8*(IMAGE_WIDTH+1)+:8]};
assign Gray_cc = {8'd0,array[8*(IMAGE_WIDTH)+:8]};
assign Gray_cr = {8'd0,array[8*(IMAGE_WIDTH-1)+:8]};
assign Gray_bl = {8'd0,array[8*(1)+:8]};
assign Gray_bc = {8'd0,array[8*(0)+:8]};
assign Gray_br = {8'd0,rbpixel};

laplacian_filter lf(isEdge, threshold, 
							Gray_tl,Gray_tc,Gray_tr,
							Gray_cl,Gray_cc,Gray_cr,
							Gray_bl,Gray_bc,Gray_br);


initial begin
	busy = 0;
	spi_state = 0;
	spi_tx_data = 0;
	spi_start_trigger = 0;
	col_counter = 0;
	row_counter = 0;
	bit_counter = 0;
	jpeg_core_input_valid = 0;
	frame_buffer_write_word = 0;
	frame_buffer_read_word = 0;
	frame_buffer_write_pointer = 0;
	frame_buffer_read_pointer = 0;
	timeout = 0;
end

always @(posedge clk) begin	
		

	if (valid_insn && cmd_set_threshold) begin
		threshold <= pcpi_rs1[15:0];
	end

	if (valid_insn && cmd_read_frame_buffer) begin
		frame_buffer_read_pointer <= pcpi_rs1[15:0];
	end

	if (valid_insn && cmd_reset) begin
		busy <= 0;
		spi_state <= 0;
		spi_tx_data <= 0;
		spi_start_trigger <= 0;
		col_counter <= 0;
		row_counter <= 0;
		bit_counter <= 0;
		jpeg_core_input_valid <= 0;
		frame_buffer_write_word <= 0;
		frame_buffer_write_pointer <= 0;
		frame_buffer_read_pointer <= 0;
		timeout <= 0;
	end

	if (valid_insn && cmd_start) begin
		busy <= 1;
	end


	frame_buffer_read_word <= frame_buffer[frame_buffer_read_pointer];

	if (busy) begin
		
		if (((row_counter > IMAGE_HEIGHT) && (bit_counter != 3'b111)) || (timeout[15])) begin
			busy <= 0;
		end

		if (jpeg_core_out_valid) begin
			timeout <= 1;
		end else if (timeout && (row_counter < IMAGE_HEIGHT)) begin
			timeout <= timeout + 1;
		end

		if (spi_state == 0) begin
			spi_tx_data <= 8'h3C;
			spi_start_trigger <= 1;
			spi_state <= 1;
		end else if (spi_state == 1) begin
			if (spi_busy) begin
				spi_tx_data <= 0;
				spi_start_trigger <= 0;
				spi_state <= 2;
			end
		end else if (spi_state == 2) begin
			if (!spi_busy) begin
				spi_tx_data <= 0;
				spi_start_trigger <= 1;
				spi_state <= 3;
			end
		end else if (spi_state == 3) begin
			if (spi_busy) begin
				spi_start_trigger <= 0;
				spi_state <= 4;
			end
		end else if (spi_state == 4) begin
			if (!spi_busy) begin
				spi_start_trigger <= 1;
				jpeg_core_input_word <= {spi_rx_byte, jpeg_core_input_word[31:8]};
				spi_state <= 5;
			end
		end else if (spi_state == 5) begin
			if (spi_busy) begin
				spi_start_trigger <= 0;
				spi_state <= 6;
			end
		end else if (spi_state == 6) begin
			if (!spi_busy) begin
				spi_start_trigger <= 1;
				jpeg_core_input_word <= {spi_rx_byte, jpeg_core_input_word[31:8]};
				spi_state <= 7;
			end
		end else if (spi_state == 7) begin
			if (spi_busy) begin
				spi_start_trigger <= 0;
				spi_state <= 8;
			end
		end else if (spi_state == 8) begin
			if (!spi_busy) begin
				spi_start_trigger <= 1;
				jpeg_core_input_word <= {spi_rx_byte, jpeg_core_input_word[31:8]};
				spi_state <= 9;
			end
		end else if (spi_state == 9) begin
			if (spi_busy) begin
				spi_start_trigger <= 0;
				spi_state <= 10;
			end
		end else if (spi_state == 10) begin
			if (!spi_busy) begin
				jpeg_core_input_word <= {spi_rx_byte, jpeg_core_input_word[31:8]};
				jpeg_core_input_valid <= 1;
				spi_state <= 11;
			end
		end else if (spi_state == 11) begin
			if (jpeg_core_accept) begin
				jpeg_core_input_valid <= 0;
				spi_state <= 2;
			end
		end


		frame_buffer[frame_buffer_write_pointer] <= frame_buffer_write_word;

		if (row_counter <= IMAGE_HEIGHT) begin
			if (row_counter < IMAGE_HEIGHT) begin
				if (ovalid) begin
					array <= {array[(3*IMAGE_WIDTH*8)-1-8:0],rbpixel};
					if (col_counter == (IMAGE_WIDTH-1)) begin
						row_counter <= row_counter + 1'd1;
						col_counter <= 0;
					end else begin
						col_counter <= col_counter + 1;
					end
					if (valid_data) begin
						frame_buffer_write_word <= {frame_buffer_write_word[6:0], isEdge};
						if (bit_counter == 3'b111) begin
							frame_buffer_write_pointer <= frame_buffer_write_pointer + 1;
							bit_counter <= 0;
						end else begin
							bit_counter <= bit_counter + 1;
						end
					end
				end
			end else if (row_counter == IMAGE_HEIGHT) begin
				array <= {array[(3*IMAGE_WIDTH*8)-1-8:0],8'd0};
				frame_buffer_write_word <= {frame_buffer_write_word[6:0], isEdge};
				if (col_counter == (IMAGE_WIDTH-1)) begin
					row_counter <= row_counter + 1'd1;
					col_counter <= 0;
				end else begin
					col_counter <= col_counter + 1;
				end
				if (bit_counter == 3'b111) begin
					frame_buffer_write_pointer <= frame_buffer_write_pointer + 1;
					bit_counter <= 0;
				end else begin
					bit_counter <= bit_counter + 1;
				end
			end
		end else begin
			if (bit_counter == 3'b111) begin
				frame_buffer_write_pointer <= frame_buffer_write_pointer + 1;
				bit_counter <= 0;
			end
		end
	end
end


endmodule



module laplacian_filter(isEdge,threshold,tl,tc,tr,cl,cc,cr,bl,bc,br);
	output isEdge;
	input [15:0] threshold;
	input [15:0] tl;
	input [15:0] tc;
	input [15:0] tr;
	input [15:0] cl;
	input [15:0] cc;
	input [15:0] cr;
	input [15:0] bl;
	input [15:0] bc;
	input [15:0] br;
	wire [15:0] tl_neg = (~tl) + 16'd1;
	wire [15:0] tc_neg = (~tc) + 16'd1;
	wire [15:0] tr_neg = (~tr) + 16'd1;
	wire [15:0] cl_neg = (~cl) + 16'd1;
	wire [15:0] cc_times_8 = {cc[12:0],3'd0};
	wire [15:0] cr_neg = (~cr) + 16'd1;
	wire [15:0] bl_neg = (~bl) + 16'd1;
	wire [15:0] bc_neg = (~bc) + 16'd1;
	wire [15:0] br_neg = (~br) + 16'd1;
	wire [15:0] lp = tl_neg + tc_neg + tr_neg + cl_neg + cc_times_8 + cr_neg + bl_neg + bc_neg + br_neg; 
	wire [15:0] mod_lp = lp[15] ? (~lp) + 16'd1 : lp;
	assign isEdge = mod_lp > threshold ? 1 : 0;
endmodule