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
output reg busy
);


parameter OPCODE = 127;
parameter IMAGE_WIDTH = 320;
parameter IMAGE_HEIGHT = 240;
parameter UART_TX_CLKS_PER_BIT  = 16'd83;
parameter SPI_CLOCK_DIVISOR = 0;


wire [2:0] cmd = pcpi_insn[14:12];
wire cmd_reset = (cmd == 3'd0) ? 1'b1 : 1'b0;
wire cmd_start = (cmd == 3'd3) ? 1'b1 : 1'b0;
wire cmd_poll = (cmd == 3'd4) ? 1'b1 : 1'b0;
wire cmd_set_threshold = (cmd == 3'd1) ? 1'b1 : 1'b0;
wire cmd_get_capture_ticks = (cmd == 3'd5) ? 1'b1 : 1'b0;
wire cmd_get_process_ticks = (cmd == 3'd6) ? 1'b1 : 1'b0;
wire cmd_get_transmit_ticks = (cmd == 3'd7) ? 1'b1 : 1'b0;

wire valid_insn;
wire [15:0] Gray_tl;
wire [15:0] Gray_tc;
wire [15:0] Gray_tr;
wire [15:0] Gray_cl;
wire [15:0] Gray_cc;
wire [15:0] Gray_cr;
wire [15:0] Gray_bl;
wire [15:0] Gray_bc;
wire [15:0] Gray_br;
wire [15:0] pixel;
wire [15:0] pixel_overlayed;
wire valid_data;
wire clk_spi;
wire rst_spi;
wire [7:0] spi_rx_byte;
wire spi_busy;
wire spi_finish;
wire uart_busy;
wire uart_finish;
wire isEdge;

reg [31:0] capture_ticks;
reg [31:0] process_ticks;
reg [31:0] transmit_ticks;

reg [(3*IMAGE_WIDTH*16)+1:0] array;
reg [15:0] col_counter;
reg [15:0] row_counter;
reg [15:0] threshold;
reg [31:0] div_clk;
reg [7:0] state;
reg uart_start_trigger;
reg [7:0] uart_tx_byte;
reg [7:0] spi_tx_data;
reg spi_start_trigger;
reg [3:0] bit_counter;

assign pcpi_wait = 0;
assign pcpi_wr = valid_insn;
assign pcpi_ready = valid_insn;
assign valid_insn = (pcpi_insn[6:0] == OPCODE[6:0]) ? pcpi_valid : 0;
assign pixel = ((row_counter == 1) || (row_counter == IMAGE_HEIGHT) || (col_counter ==0) || (col_counter == 1)) ?  array[(16*IMAGE_WIDTH)+:16] : pixel_overlayed;
assign isEdge = (pixel == 16'b1111100000000000) ? 1'b1 :  1'b0;
assign pcpi_rd = (valid_insn && cmd_get_capture_ticks) ? capture_ticks : ((valid_insn && cmd_get_process_ticks) ? process_ticks : ((valid_insn && cmd_get_transmit_ticks) ? transmit_ticks : {31'd0,busy}));
assign valid_data = ((row_counter > 0) && (row_counter <= IMAGE_HEIGHT)) ? 1'b1 : 1'b0;
assign rst_spi = (busy) ? 0 : 1;

rgb2gray gtl(array[16*(IMAGE_WIDTH+IMAGE_WIDTH+1)+:16],Gray_tl);
rgb2gray gtc(array[16*(IMAGE_WIDTH+IMAGE_WIDTH)+:16],Gray_tc);
rgb2gray gtb(array[16*(IMAGE_WIDTH+IMAGE_WIDTH-1)+:16],Gray_tr);
rgb2gray gcl(array[16*(IMAGE_WIDTH+1)+:16],Gray_cl);
rgb2gray gcc(array[16*(IMAGE_WIDTH)+:16],Gray_cc);
rgb2gray gcr(array[16*(IMAGE_WIDTH-1)+:16],Gray_cr);
rgb2gray gbl(array[16*(2)+:16],Gray_bl);
rgb2gray gbc(array[16*(1)+:16],Gray_bc);
rgb2gray gbr(array[16*(0)+:16],Gray_br);
laplacian_filter lf(pixel_overlayed, array[16*(IMAGE_WIDTH)+:16], threshold, Gray_tl,Gray_tc,Gray_tr,Gray_cl,Gray_cc,Gray_cr,Gray_bl,Gray_bc,Gray_br);



uart_tx  #(.CLKS_PER_BIT(UART_TX_CLKS_PER_BIT)) tx(
	.i_Clock(clk),
	.i_Tx_DV(uart_start_trigger),
	.i_Tx_Byte(uart_tx_byte), 
	.o_Tx_Active(uart_busy),
	.o_Tx_Serial(uart_tx),
	.o_Tx_Done(uart_finish)
);


generate 
	if (SPI_CLOCK_DIVISOR > 0) begin
		initial div_clk <= 0;
		assign clk_spi = div_clk[SPI_CLOCK_DIVISOR-1];  
		always @(posedge clk) begin
		    div_clk <= div_clk + 32'd1;
		end
		spi_burst_read SPI(
			.clk(clk_spi),
			.rst(rst),
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
			.rst(rst),
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


always @(posedge clk) begin	
	if (valid_insn && cmd_set_threshold) begin
		threshold <= pcpi_rs1[15:0];
	end

	if (valid_insn && cmd_reset) begin
		col_counter <= 0;
		row_counter <= 0;
		busy <= 0;
		state <= 0;
		uart_start_trigger <= 0;
		uart_tx_byte <= 0;
		spi_tx_data <= 0;
		spi_start_trigger <= 0;
		capture_ticks <= 0;
		process_ticks <= 0;
		transmit_ticks <= 0;
	end

	if (valid_insn && cmd_start) begin
		busy <= 1'b1;
		state <= 0;
	end

	if (busy && ((state == 0) || (state == 1) || (state == 3) || (state == 6) || (state == 7) || (state == 8) || (state == 9))) 
		capture_ticks <= capture_ticks + 1;

	if (busy && (((state == 4) && (bit_counter[3])) || (state == 5))) 
		transmit_ticks <= transmit_ticks + 1;

	if (busy && ((state == 10))) 
		process_ticks <= process_ticks + 1;

	if (busy) begin
		if (row_counter > IMAGE_HEIGHT) begin
			col_counter <= 0;
			row_counter <= 0;
			busy <= 0;
			state <= 0;
			uart_start_trigger <= 0;
			uart_tx_byte <= 0;
			spi_tx_data <= 0;
			spi_start_trigger <= 0;
			bit_counter <= 0;
		end else if (state == 0) begin // sending burst fifo command
			spi_start_trigger <= 1;
			spi_tx_data <= 8'h3C;
			if (spi_busy) begin
				state <= 3; 
				spi_start_trigger <= 0;
				spi_tx_data <= 0;	
			end
		end else if (state == 3) begin // checking if done
			if (!spi_busy) begin
				state <= 4;
				uart_start_trigger <= 0;
				uart_tx_byte <= 0;
				spi_tx_data <= 0;
				spi_start_trigger <= 0;
				bit_counter <= 0;
			end
		end else if (state == 4) begin // base state of fsm - will return here after every pixel processed
			if (bit_counter[3]) begin // check if a binary image vector is ready to transmit
				if (!uart_busy) begin // check if uart is ready
					bit_counter <= 0;
					uart_start_trigger <= 1;
					state <= 5;
				end
			end else begin	// if not uart tx needed, get and process the next pixels
				spi_start_trigger <= 1;
				state <= 6;
			end
		end else if (state == 5) begin // if uart tx started, get and process the next pixels
			if (uart_busy) begin 
					uart_start_trigger <= 0;
					spi_start_trigger <= 1;
					state <= 6;
			end
		end else if (state == 6) begin // check if spi started
			if (spi_busy) begin 
					spi_start_trigger <= 0;
					state <= 7;
			end
		end else if (state == 7) begin // check if spi rx available - shift into pixel array
			if (!spi_busy) begin 
					array<= {array[(3*IMAGE_WIDTH*16)-1-8:0],spi_rx_byte};
					spi_start_trigger <= 1;
					state <= 8;
			end
		end else if (state == 8) begin // check if spi started
			if (spi_busy) begin 
					spi_start_trigger <= 0;
					state <= 9;
			end
		end else if (state == 9) begin // check if spi rx available - shift into pixel array
			if (!spi_busy) begin 
					array<= {array[(3*IMAGE_WIDTH*16)-1-8:0],spi_rx_byte};
					state <= 10;
			end
		end else if (state == 10) begin // update uart_tx_byte and counters
			if (valid_data) begin
				uart_tx_byte <= {uart_tx_byte[6:0],isEdge};
				bit_counter <= bit_counter + 4'd1;
			end
			col_counter <= col_counter + 1;
			if (col_counter == (IMAGE_WIDTH-1)) begin
				row_counter <= row_counter + 1'd1;
				col_counter <= 0;
			end
			state <= 4;
			spi_start_trigger <= 0;
			uart_start_trigger <= 0;
		end
	end
end
endmodule






module laplacian_rgb565_rv32_pcpi(
input clk,
input        	pcpi_valid,
input [31:0] 	pcpi_insn,
input [31:0] 	pcpi_rs1,
input [31:0] 	pcpi_rs2,
output       	pcpi_wr,
output  [31:0] 	pcpi_rd,
output       	pcpi_wait,
output 	    	pcpi_ready
);

parameter OPCODE = 127;
parameter IMAGE_WIDTH = 320;
parameter IMAGE_HEIGHT = 240;

wire [2:0] cmd = pcpi_insn[14:12];
wire cmd_reset = (cmd == 3'd0) ? 1'b1 : 1'b0;
wire cmd_push = (cmd == 3'd2) ? 1'b1 : 1'b0;
wire cmd_set_threshold = (cmd == 3'd1) ? 1'b1 : 1'b0;

wire valid_insn;
wire [15:0] Gray_tl;
wire [15:0] Gray_tc;
wire [15:0] Gray_tr;
wire [15:0] Gray_cl;
wire [15:0] Gray_cc;
wire [15:0] Gray_cr;
wire [15:0] Gray_bl;
wire [15:0] Gray_bc;
wire [15:0] Gray_br;
wire [15:0] pixel;
wire [15:0] pixel_overlayed;

reg [(3*IMAGE_WIDTH*16)+1:0] array;
reg [15:0] col_counter;
reg [15:0] row_counter;
reg [15:0] threshold;
reg valid_data;

assign pcpi_wait = 0;
assign pcpi_wr = valid_insn;
assign pcpi_ready = valid_insn;
assign valid_insn = (pcpi_insn[6:0] == OPCODE[6:0]) ? pcpi_valid : 0;
assign pixel = ((row_counter == 1) || (row_counter == IMAGE_HEIGHT) || (col_counter ==0) || (col_counter == 1)) ?  array[(16*IMAGE_WIDTH)+:16] : pixel_overlayed;
//(((col_counter > 0) && (col_counter < IMAGE_WIDTH-1)) && ((row_counter > 1) && (row_counter < IMAGE_HEIGHT))) ? pixel_overlayed : array[IMAGE_WIDTH];
assign pcpi_rd = {15'd0,(row_counter >= 1) ? 1'b1: 1'b0,pixel};

rgb2gray gtl(array[16*(IMAGE_WIDTH+IMAGE_WIDTH+1)+:16],Gray_tl);
rgb2gray gtc(array[16*(IMAGE_WIDTH+IMAGE_WIDTH)+:16],Gray_tc);
rgb2gray gtb(array[16*(IMAGE_WIDTH+IMAGE_WIDTH-1)+:16],Gray_tr);
rgb2gray gcl(array[16*(IMAGE_WIDTH+1)+:16],Gray_cl);
rgb2gray gcc(array[16*(IMAGE_WIDTH)+:16],Gray_cc);
rgb2gray gcr(array[16*(IMAGE_WIDTH-1)+:16],Gray_cr);
rgb2gray gbl(array[16*(2)+:16],Gray_bl);
rgb2gray gbc(array[16*(1)+:16],Gray_bc);
rgb2gray gbr(array[16*(0)+:16],Gray_br);
laplacian_filter lf(pixel_overlayed, array[16*(IMAGE_WIDTH)+:16], threshold, Gray_tl,Gray_tc,Gray_tr,Gray_cl,Gray_cc,Gray_cr,Gray_bl,Gray_bc,Gray_br);



integer i;
always @(posedge clk) begin
	
	if (valid_insn && cmd_set_threshold) begin
		threshold <= pcpi_rs1[15:0];
	end
	if (valid_insn && cmd_reset) begin
		col_counter <= 0;
		row_counter <= 0;
		valid_data <= 0;
	end
	if (valid_insn && cmd_push) begin
		valid_data <= ((row_counter > 0) && (row_counter <= IMAGE_HEIGHT)) ? 1'b1 : 1'b0;
		array<= {array[(3*IMAGE_WIDTH*16)-1-16:0],pcpi_rs1[15:0]};
		if (col_counter == (IMAGE_WIDTH-1)) begin
			row_counter <= row_counter + 1'd1;
			col_counter <= 0;
		end else begin
			col_counter <= col_counter + 1;
		end
	end
end
endmodule


module rgb2gray(input [15:0] pixel, output [15:0] gray);
	wire [5:0] R_s = {pixel[15:11],1'b0};
	wire [5:0] G_s = pixel[10:5];
	wire [5:0] B_s = {pixel[4:0],1'b0};
	wire [7:0] Gr = R_s+G_s+B_s;
	assign gray = {8'd0,Gr};
endmodule

module laplacian_filter(pixel_overlayed,pixel_rgb,threshold,tl,tc,tr,cl,cc,cr,bl,bc,br);
	output [15:0] pixel_overlayed;
	input [15:0] pixel_rgb;
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
	assign pixel_overlayed = mod_lp > threshold ? 16'b1111100000000000 : pixel_rgb;
endmodule