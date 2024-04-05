`timescale 1ps/1ps
//`define SKIP_PHY_SIM
//`define USE_BRAM

module ddr_fifo (
clk,
rst,
data_in_data,
data_in_valid,
data_in_ready,
data_out_data,
data_out_ready,
data_out_valid);

parameter DATA_WIDTH = 8;
parameter BUFFER_LOG_SIZE = 6;
parameter BUFFER_SIZE = 2**BUFFER_LOG_SIZE;

input 				clk;
input 				rst;
input [DATA_WIDTH-1:0] 		data_in_data;
input 				data_in_valid;
output				data_in_ready;
input 				data_out_ready;
output 	reg			data_out_valid;
output reg [DATA_WIDTH-1:0]	data_out_data;


reg [DATA_WIDTH-1:0] ring_buffer [0:BUFFER_SIZE-1];
reg [BUFFER_LOG_SIZE-1:0] w_pointer;
reg [BUFFER_LOG_SIZE-1:0]  r_pointer;

assign data_in_ready = 1;

always @(negedge clk) begin
	if (rst) begin
		w_pointer <= 0;
	end else if (data_in_valid) begin
		ring_buffer[w_pointer] <= data_in_data;
		w_pointer <= w_pointer + {{BUFFER_LOG_SIZE-1{1'b0}},1'b1};
	end
end


always @(posedge clk) begin
	if (rst) 
		data_out_data <= 0;
	else
		data_out_data <= ring_buffer[r_pointer];
end

always @(posedge clk) begin
	if (rst) begin
		r_pointer <= 0;
		data_out_valid <= 0;
	end else if (data_out_ready && data_out_valid) begin
		r_pointer <= r_pointer +  {{BUFFER_LOG_SIZE-1{1'b0}},1'b1};
		data_out_valid <= (r_pointer +  {{BUFFER_LOG_SIZE-1{1'b0}},1'b1} == w_pointer) ?  1'b0 : 1'b1;
	end else
		data_out_valid <= (w_pointer == r_pointer) ? 1'b0 : 1'b1;
	
end

endmodule






module ddr_address_map (
	clk,
	rst,
	config_valid,
	config_word,
	config_addr,
	address_in, 
	bank_out, 
	row_out, 
	col_out, 
	address_out, 
	bank_in, 
	row_in, 
	col_in 
);

parameter CONFIGURATION_DATA_BITS = 32;
parameter CONFIGURATION_ADDR_BITS = 32;
parameter ADDR_BITS = 32;
parameter BA_BITS = 3;
parameter ROW_BITS = 14;
parameter COL_BITS = 10;
parameter ADDRESS_FORMAT_RESET = 1;
parameter CONFIGADDR_START_ADDRESS_FORMAT = 0;
parameter CONFIGADDR_END_ADDRESS_FORMAT = 0;
parameter BURST_BITS = 3;

input clk;
input rst;
input config_valid;
input [CONFIGURATION_DATA_BITS-1:0] config_word;
input [CONFIGURATION_ADDR_BITS-1:0] config_addr;
input [ADDR_BITS-1:0] address_in;
output reg [BA_BITS-1:0] bank_out;
output reg [ROW_BITS-1:0] row_out;
output reg [COL_BITS-1:0] col_out;
output reg [ADDR_BITS-1:0] address_out;
input [BA_BITS-1:0] bank_in;
input [ROW_BITS-1:0] row_in;
input [COL_BITS-1:0] col_in;

reg [7:0] map_id;

   always @(posedge clk) begin
        if (rst) 
            map_id <= ADDRESS_FORMAT_RESET;
        else if (config_valid) begin
			if ((config_addr >= CONFIGADDR_START_ADDRESS_FORMAT) && (config_addr <= CONFIGADDR_END_ADDRESS_FORMAT))
				map_id <= config_word[7:0];
	    end
    end

// 0: Interleaved, 1: RBC, 2: BRC, 3: reserved
always @(*) begin
	if (map_id == 0) begin 
		address_out =  {{ADDR_BITS-ROW_BITS-BA_BITS-COL_BITS{1'b0}},row_in,col_in[COL_BITS-1:BURST_BITS],bank_in,col_in[BURST_BITS-1:0]};
		bank_out = address_in[BURST_BITS+BA_BITS-1:BURST_BITS];
		col_out = {address_in[BA_BITS+COL_BITS-1:BURST_BITS+BA_BITS],address_in[BURST_BITS-1:0]};
		row_out = address_in[BA_BITS+COL_BITS+ROW_BITS-1:BA_BITS+COL_BITS];
		
	end else if (map_id == 8'd1) begin
		address_out =  {{ADDR_BITS-ROW_BITS-BA_BITS-COL_BITS{1'b0}},row_in,bank_in,col_in};
		bank_out = address_in[COL_BITS+BA_BITS-1:COL_BITS];
		col_out = address_in[COL_BITS-1:0];
		row_out = address_in[BA_BITS+COL_BITS+ROW_BITS-1:BA_BITS+COL_BITS];
		
	end else if (map_id == 8'd2) begin 
		address_out =  {{ADDR_BITS-ROW_BITS-BA_BITS-COL_BITS{1'b0}},row_in,col_in[COL_BITS-1:BURST_BITS+1],bank_in,col_in[BURST_BITS:0]};
		bank_out = address_in[BURST_BITS+BA_BITS:BURST_BITS+1];
		col_out = {address_in[BA_BITS+COL_BITS-1:BURST_BITS+1+BA_BITS],address_in[BURST_BITS:0]};
		row_out = address_in[BA_BITS+COL_BITS+ROW_BITS-1:BA_BITS+COL_BITS];	
		
	end else if (map_id == 8'd3) begin 
		address_out =  {{ADDR_BITS-ROW_BITS-BA_BITS-COL_BITS{1'b0}},row_in,col_in[COL_BITS-1:BURST_BITS+2],bank_in,col_in[BURST_BITS+1:0]};
		bank_out = address_in[BURST_BITS+BA_BITS+1:BURST_BITS+2];
		col_out = {address_in[BA_BITS+COL_BITS-1:BURST_BITS+2+BA_BITS],address_in[BURST_BITS+1:0]};
		row_out = address_in[BA_BITS+COL_BITS+ROW_BITS-1:BA_BITS+COL_BITS];	
		
	end else if (map_id == 8'd4) begin 
		address_out =  {{ADDR_BITS-ROW_BITS-BA_BITS-COL_BITS{1'b0}},row_in,col_in[COL_BITS-1:BURST_BITS+3],bank_in,col_in[BURST_BITS+2:0]};
		bank_out = address_in[BURST_BITS+BA_BITS+2:BURST_BITS+3];
		col_out = {address_in[BA_BITS+COL_BITS-1:BURST_BITS+3+BA_BITS],address_in[BURST_BITS+2:0]};
		row_out = address_in[BA_BITS+COL_BITS+ROW_BITS-1:BA_BITS+COL_BITS];	
		
	end else if (map_id == 8'd5) begin 
		address_out =  {{ADDR_BITS-ROW_BITS-BA_BITS-COL_BITS{1'b0}},row_in,col_in[COL_BITS-1:BURST_BITS+4],bank_in,col_in[BURST_BITS+3:0]};
		bank_out = address_in[BURST_BITS+BA_BITS+3:BURST_BITS+4];
		col_out = {address_in[BA_BITS+COL_BITS-1:BURST_BITS+4+BA_BITS],address_in[BURST_BITS+3:0]};
		row_out = address_in[BA_BITS+COL_BITS+ROW_BITS-1:BA_BITS+COL_BITS];	
		
	end else begin
		address_out =  {{ADDR_BITS-ROW_BITS-BA_BITS-COL_BITS{1'b0}},bank_in,row_in,col_in};
		bank_out = address_in[BA_BITS+COL_BITS+ROW_BITS-1:COL_BITS+ROW_BITS];
		col_out = address_in[COL_BITS-1:0];
		row_out = address_in[COL_BITS+ROW_BITS-1:COL_BITS];
	end
end
endmodule





module ddr_arbiter (
	clk,
	rst,
	request, 
	grant,
	hold,
	// Configuration bus
	config_valid,
	config_word,
	config_addr
);

	parameter CONFIGURATION_DATA_BITS = 32;
	parameter CONFIGURATION_ADDR_BITS = 32;
	parameter BA_BITS = 3;
	parameter NUM_BANKS = 8;
	parameter SIMULATION = 0;
	parameter ARBITER_RULES_INIT_HEX = "init_arbiter_rules.hex";
	parameter CONFIGADDR_START_ARBITER = 0;
	parameter CONFIGADDR_END_ARBITER = (2**(BA_BITS + NUM_BANKS)) - 1;

	input clk;
	input rst;
	
	input config_valid;
	input [CONFIGURATION_DATA_BITS-1:0] config_word;
	input [CONFIGURATION_ADDR_BITS-1:0] config_addr;

	input [NUM_BANKS-1:0] request;
	output [BA_BITS-1:0] grant;

	input hold;

	reg [BA_BITS-1:0] last_granted;
	reg [BA_BITS-1:0] arbiter_rules [0:(2**(BA_BITS + NUM_BANKS)) -1];
	

    generate if(SIMULATION) begin
        initial $readmemh({"../../../../../" ,  ARBITER_RULES_INIT_HEX}, arbiter_rules);
    end else begin
        initial $readmemh(ARBITER_RULES_INIT_HEX, arbiter_rules);
    end
    endgenerate

	always @(posedge clk) begin
		if (config_valid) begin
			if ((config_addr >= CONFIGADDR_START_ARBITER) && (config_addr <= CONFIGADDR_END_ARBITER))
				arbiter_rules[config_addr - CONFIGADDR_START_ARBITER] <= config_word[BA_BITS-1:0];
		end
	end
    
	assign grant = arbiter_rules[{request, last_granted}];
    always @(posedge clk) begin
		if (rst) begin
			last_granted <= 0;
		end else if (request && (!hold)) begin
			last_granted <= grant;
		end
	end
endmodule





module ddr_timer_db(
	//Clock
	clk, 

	// Configuration bus
	config_valid,
	config_word,
	config_addr,

	// 
	tccd,
	trp,
	trp_ap,
	trtp,
	trcd,
	tras,
	trefi,
	trfc,
	twtr,
	trtw,
	twr,
	refresh_debt
);

	parameter CONFIGURATION_DATA_BITS = 32;
	parameter CONFIGURATION_ADDR_BITS = 32;
	parameter CONFIGADDR_START_TIMERS = 0;
	parameter CONFIGADDR_END_TIMERS = 0;
	parameter TIMER_TCCD_RESET = 0;
	parameter TIMER_TRP_RESET = 0;
	parameter TIMER_TRP_AP_RESET = 0;
	parameter TIMER_TRTP_RESET = 0;
	parameter TIMER_TRCD_RESET = 0;
	parameter TIMER_TRAS_RESET = 0;
	parameter TIMER_TREFI_RESET = 0;
	parameter TIMER_TRFC_RESET = 0;
	parameter TIMER_TWTR_RESET = 0;
	parameter TIMER_TRTW_RESET = 0;
	parameter TIMER_TWR_RESET = 0;
	parameter MAX_REFRESH_DEBT = 0;
	
	input clk;
	input config_valid;
	input [CONFIGURATION_DATA_BITS-1:0] config_word;
	input [CONFIGURATION_ADDR_BITS-1:0] config_addr;
	
	output reg [2:0] tccd;
	output reg [2:0] trp;
	output reg [2:0] trp_ap;
	output reg [2:0] trtp;
	output reg [2:0] trcd;
	output reg [2:0] tras;
	output reg [2:0] twtr;
	output reg [2:0] twr;
	output reg [2:0] trtw;
	output reg [9:0] trefi;
	output reg [9:0] trfc;
	output reg [31:0] refresh_debt;

	initial tccd = TIMER_TCCD_RESET;
	initial trp = TIMER_TRP_RESET;
	initial trp_ap = TIMER_TRP_AP_RESET;
	initial trtp = TIMER_TRTP_RESET;
	initial trcd = TIMER_TRCD_RESET;
	initial tras = TIMER_TRAS_RESET;
	initial trefi = TIMER_TREFI_RESET;
	initial trfc = TIMER_TRFC_RESET;
	initial twtr = TIMER_TWTR_RESET;
	initial trtw = TIMER_TRTW_RESET;
	initial twr = TIMER_TWR_RESET;
	initial refresh_debt = MAX_REFRESH_DEBT;

	wire [CONFIGURATION_ADDR_BITS-1:0] c_addr = config_addr - CONFIGADDR_START_TIMERS;
	
	always @(posedge clk) begin
		if (config_valid) begin
			if ((config_addr >= CONFIGADDR_START_TIMERS) && (config_addr <= CONFIGADDR_END_TIMERS))
				if (c_addr == 0)
					tccd <= config_word[2:0];
				else if (c_addr == 1)
					trp <= config_word[2:0];
				else if (c_addr == 2)
					trcd <= config_word[2:0];
				else if (c_addr == 3)
					tras <= config_word[2:0];
				else if (c_addr == 4)
					trefi <= config_word[9:0];
				else if (c_addr == 5)
					trfc <= config_word[9:0];
				else if (c_addr == 6)
					twtr <= config_word[2:0];
				else if (c_addr == 7)
					trtw <= config_word[2:0];
				else if (c_addr == 8)
					twr <= config_word[2:0];
				else if (c_addr == 9)
					trtp <= config_word[2:0];
				else if (c_addr == 10)
					trp_ap <= config_word[2:0];
				else if (c_addr == 11)
					refresh_debt <= config_word[31:0];
		end
	end
endmodule








module ddr_runtime_rules(
	//Clock
	clk, 

	// Configuration bus
	config_valid,
	config_word,
	config_addr,

	// Macro bus
	row_open,
	r_wr,
	bank_precharged,

	macro
);

	parameter RUNTIME_RULES_INIT_HEX = "init_runtime_rules.hex";
	parameter MACRO_WORD = 128;
	parameter CONFIGADDR_START_RUNTIME_RULES = 0;
	parameter CONFIGADDR_END_RUNTIME_RULES = 0;
	parameter CONFIGURATION_DATA_BITS = 32;
	parameter CONFIGURATION_ADDR_BITS = 32;
	parameter SIMULATION = 0;
	parameter MACRO_CONFIG_WORDS_NEEDED = 4; 
	parameter MACRO_CONFIG_BITS_NEEDED = 2; 

	localparam NUMBER_OF_RUNTIME_RULES = 8;
	localparam RUNTIME_RULES_BITS = 3;
	localparam CONFIG_BITS_NEEDED = MACRO_CONFIG_BITS_NEEDED;

	input clk;
	input config_valid;
	input [CONFIGURATION_DATA_BITS-1:0] config_word;
	input [CONFIGURATION_ADDR_BITS-1:0] config_addr;
	input  row_open;
	input r_wr;
	input bank_precharged;
	output [MACRO_WORD-1:0]  macro;


	reg [MACRO_WORD-1:0] runtime_rules [0:NUMBER_OF_RUNTIME_RULES-1];

	generate if (SIMULATION) begin
        initial $readmemh({"../../../../../" , RUNTIME_RULES_INIT_HEX}, runtime_rules);
    end else begin
        initial $readmemh(RUNTIME_RULES_INIT_HEX, runtime_rules);
    end
    endgenerate
    


	wire [MACRO_WORD-1:0]  updated_macro;

	wire [CONFIGURATION_ADDR_BITS-1:0] c_addr = config_addr - CONFIGADDR_START_RUNTIME_RULES;
	wire c_valid = config_valid && (config_addr >= CONFIGADDR_START_RUNTIME_RULES) && (config_addr >= CONFIGADDR_END_RUNTIME_RULES);
    assign macro = c_valid ? runtime_rules[c_addr[RUNTIME_RULES_BITS+CONFIG_BITS_NEEDED-1:CONFIG_BITS_NEEDED]] : runtime_rules[{bank_precharged,row_open, r_wr}];
 	
	always @(posedge clk) begin
		if (c_valid)
			runtime_rules[c_addr[RUNTIME_RULES_BITS+CONFIG_BITS_NEEDED-1:CONFIG_BITS_NEEDED]] <= updated_macro;
	end
	
	wire [MACRO_WORD-1:0] config_word_repeated = {MACRO_CONFIG_WORDS_NEEDED{config_word}};
	reg [(MACRO_CONFIG_WORDS_NEEDED*CONFIGURATION_DATA_BITS)-1:0] config_word_mask;
	assign updated_macro =  c_valid ?  (macro & (~config_word_mask)) | (config_word_repeated & config_word_mask) : macro;
	integer i;
	integer j;
	always @(*) begin
		config_word_mask = 0;
			for (i=0; i < MACRO_CONFIG_WORDS_NEEDED; i=i+1) begin
			    for (j=0; j < CONFIGURATION_DATA_BITS; j=j+1) begin
					if (i == (c_addr[CONFIG_BITS_NEEDED-1:0])) begin
					    config_word_mask[i*MACRO_CONFIG_WORDS_NEEDED + j] = 1'b1;
				   end           
			    end
		    end
	end
endmodule





module ddr_refresh(
	//Clock
	clk, 
	rst,
	// Configuration bus
	config_valid,
	config_word,
	config_addr,

	busy,
	start_refresh,
	refresh_debt,

	command,
	command_valid
);

	parameter CONFIGURATION_DATA_BITS = 32;
	parameter CONFIGURATION_ADDR_BITS = 32;
	parameter CMD_REFRESH =  4;
	parameter COMMAND_WORD = 11;
	parameter REFRESH_RULES_INIT_HEX = "init_refresh_rules.hex";
	parameter SIMULATION = 0;
	parameter CONFIGADDR_START_REFRESH_RULES = 0;
	parameter CONFIGADDR_END_REFRESH_RULES = 0;

	localparam NUMBER_OF_REFRESH_RULES = 2;
	localparam REFRESH_RULES_BITS = $clog2(NUMBER_OF_REFRESH_RULES);
	
	
	parameter CONFIGADDR_START_TIMERS = 0;
	parameter CONFIGADDR_END_TIMERS = 0;
	
	parameter TIMER_TCCD_RESET = 0;
	parameter TIMER_TRP_RESET = 0;
	parameter TIMER_TRP_AP_RESET = 0;
	parameter TIMER_TRTP_RESET = 0;
	parameter TIMER_TRCD_RESET = 0;
	parameter TIMER_TRAS_RESET = 0;
	parameter TIMER_TREFI_RESET = 0;
	parameter TIMER_TRFC_RESET = 0;
	parameter TIMER_TWTR_RESET = 0;
	parameter TIMER_TRTW_RESET = 0;
	parameter TIMER_TWR_RESET = 0;
	parameter MAX_REFRESH_DEBT = 0;
	 
    


	input clk;
	input rst;
	input config_valid;
	input [CONFIGURATION_DATA_BITS-1:0] config_word;
	input [CONFIGURATION_ADDR_BITS-1:0] config_addr;

	input start_refresh;
	output busy;
	output [COMMAND_WORD-1:0] command;
	output command_valid;
	output reg [31:0] refresh_debt;
			
	wire [9:0] trefi_reset;
	wire [9:0] trfc_reset;
	wire [2:0] trp_reset;

	reg [9:0] trfc;
	reg [9:0] trefi;
	reg [2:0] trp;


	reg [3:0] refresh_command_counter;

	reg [COMMAND_WORD-1:0] refresh_rules [0: NUMBER_OF_REFRESH_RULES-1];

	generate if (SIMULATION) begin
       initial $readmemh({"../../../../../" , REFRESH_RULES_INIT_HEX}, refresh_rules);
    end else begin
	   initial $readmemh(REFRESH_RULES_INIT_HEX, refresh_rules);
    end
    endgenerate 
    
    ddr_timer_db 
	#(
	.CONFIGURATION_DATA_BITS(CONFIGURATION_DATA_BITS),
	.CONFIGURATION_ADDR_BITS(CONFIGURATION_ADDR_BITS),
	.CONFIGADDR_START_TIMERS(CONFIGADDR_START_TIMERS),
	.CONFIGADDR_END_TIMERS(CONFIGADDR_END_TIMERS),
	.TIMER_TRP_RESET(TIMER_TRP_RESET),
	.TIMER_TREFI_RESET(TIMER_TREFI_RESET),
	.TIMER_TRFC_RESET(TIMER_TRFC_RESET)
	)
	timers(
		.clk(clk), 
		.config_valid(config_valid),
		.config_word(config_word),
		.config_addr(config_addr),
		.trp(trp_reset),
		.trefi(trefi_reset),
		.trfc(trfc_reset)
	);

    wire [CONFIGURATION_ADDR_BITS-1:0] c_addr = config_addr - CONFIGADDR_START_REFRESH_RULES;
    wire c_valid = config_valid && (config_addr >= CONFIGADDR_START_REFRESH_RULES) && (config_addr <= CONFIGADDR_END_REFRESH_RULES);
  
	always @(posedge clk) begin
		if (c_valid) begin
			if (refresh_command_counter == 0) begin
				refresh_rules[c_addr[REFRESH_RULES_BITS-1:0]] <= config_word[COMMAND_WORD-1:0];
			end
		end
	end

	assign busy = (refresh_command_counter > 4'd0) ? 1'b1 : 1'b0;
	assign command_valid = ((refresh_command_counter == 4'd4) || (refresh_command_counter == 4'd2)) ? 1'b1 : 1'b0;
	assign command =  refresh_rules[refresh_command_counter == 4'd4 ? 0 : 1];


	always @(posedge clk) begin
		if (rst) 
			trefi <= trefi_reset;
		else if (trefi == 0)
			trefi <= trefi_reset;
		else
			trefi <= trefi - 10'd1;


		if (rst) 
			trfc <= 0;
		else
			trfc <= (command_valid && (command[3:0] == CMD_REFRESH)) ? trfc_reset : ((trfc > 0) ? trfc - 10'd1 : 0);


		if (rst) 
			trp <= 0;
		else if (refresh_command_counter  == 4'd4)
			trp <= trp;
		else if (trp > 0)
			trp <= trp - 3'd1;


		if (rst)
			refresh_debt <= 0;
		else if ((refresh_command_counter ==  4'd2) && refresh_debt && trefi) 
			refresh_debt <= refresh_debt - 32'd1; 
		else if ((refresh_command_counter !=  4'd2 ) && (trefi == 0))
			refresh_debt <= refresh_debt + 32'd1; 


		if (rst)
			refresh_command_counter <= 0;
		else if (start_refresh)
			refresh_command_counter <= 4'd7;
		else if ((refresh_command_counter > 4'd4))
			refresh_command_counter <= refresh_command_counter - 1;
		else if ((refresh_command_counter == 4'd4))
			refresh_command_counter <= 4'd3;
		else if ((refresh_command_counter == 4'd3) && (trp == 0))
			refresh_command_counter <= 4'd2;
		else if ((refresh_command_counter == 4'd2))
			refresh_command_counter <= 4'd1;
		else if ((refresh_command_counter == 4'd1) && (trfc == 0))
			refresh_command_counter <= 4'd0;
		else
			refresh_command_counter <= refresh_command_counter;
	end

endmodule



module ddr_perftuner(
	//Clock
	clk, 
	rst,

	// Configuration bus
	config_valid,
	config_word,
	config_addr,

	bank_busy,

	r_valid,
	r_wr, 
	r_addr,
	r_wrmask,
	r_wrdata,

	r_valid_delayed,
	r_wr_delayed,
	r_addr_delayed,
	r_wrmask_delayed,
	r_wrdata_delayed,
	
	empty,

	auto_precharge_request,
	auto_precharge_ack,
		
	auto_activate_possible,
	auto_activate_ack,
		
	selected_auto_precharge_bank,
	auto_precharge,
	ap_bit,
	auto_activate,
	auto_activate_bank,
	auto_activate_row,

	active_cmd,
	active_cmd_valid,
	active_cmd_bank
    
);
	parameter ENABLE_PERFTUNER = 1;	
	parameter CONFIGURATION_DATA_BITS = 32;
	parameter CONFIGURATION_ADDR_BITS = 32;
	parameter ADDR_BITS = 32;
	parameter MASK_BITS = 16;
	parameter DATA_BITS = 128;
	parameter AP_BIT_RESET = 0;
	parameter AUTO_PRECHARGE_IDLE_ROW_EN_RESET = 0;
	parameter TRANSACTION_DELAY = 0;
	parameter AUTO_PRECHARGE_IDLE_ROW_LIMIT_RESET = 0;
	parameter AUTO_ACTIVATE_IDLE_BANK_EN_RESET = 0;
	parameter CMD_ACTIVATE = 0;
    parameter CMD_PRECHARGE = 0;
    parameter CONFIGADDR_START_PERFTUNER = 0;
	parameter CONFIGADDR_END_PERFTUNER = 0;
	parameter BA_BITS = 3;
	parameter ROW_BITS = 14;
	parameter COL_BITS = 10;
	parameter ADDRESS_FORMAT_RESET = 1;
	parameter CONFIGADDR_START_ADDRESS_FORMAT = 10;
	parameter CONFIGADDR_END_ADDRESS_FORMAT = 10;
	parameter BURST_BITS = 3;
	parameter COMMAND_WORD = 11;

	input clk;
	input rst;
	input config_valid;
	input [CONFIGURATION_DATA_BITS-1:0] config_word;
	input [CONFIGURATION_ADDR_BITS-1:0] config_addr;

	input bank_busy;

	input r_valid;
	input r_wr; 
	input [ADDR_BITS-1:0] r_addr;
	input [MASK_BITS-1:0] r_wrmask;
	input [DATA_BITS-1:0] r_wrdata;


	output r_valid_delayed;
	output r_wr_delayed; 
	output [ADDR_BITS-1:0] r_addr_delayed;
	output [MASK_BITS-1:0] r_wrmask_delayed;
	output [DATA_BITS-1:0] r_wrdata_delayed;

	input [(2**BA_BITS)-1:0] auto_precharge_request;
	output reg [(2**BA_BITS)-1:0] auto_precharge_ack;

	output reg ap_bit;
	output reg [BA_BITS-1:0] selected_auto_precharge_bank;
    output auto_precharge;

	input [(2**BA_BITS)-1:0] auto_activate_possible;
	output reg [(2**BA_BITS)-1:0] auto_activate_ack;

    output auto_activate;
	output [ROW_BITS-1:0] auto_activate_row;
	output [BA_BITS-1:0] auto_activate_bank;

    input      [COMMAND_WORD-1:0]           active_cmd;
    input active_cmd_valid;
    input [BA_BITS-1:0] active_cmd_bank;
    
    output empty;
    


    generate if (ENABLE_PERFTUNER) begin

        localparam TRANSACTION_WORD_SIZE = ADDR_BITS+MASK_BITS+DATA_BITS+1+1; 
    
        reg [(TRANSACTION_WORD_SIZE*TRANSACTION_DELAY)-1:0] transaction_shiftreg;
        
        reg empty_int;
        
        integer iiii;
        always @(*) begin
        empty_int = 0;
            for (iiii=1; iiii<= TRANSACTION_DELAY; iiii=iiii+1) begin 
                empty_int = empty_int | transaction_shiftreg[(iiii*TRANSACTION_WORD_SIZE)-1];
            end
        end
    
        assign empty = !empty_int;
    
        always @(posedge clk) begin
            if (rst)
                transaction_shiftreg <= 0;
            else if (!bank_busy)
                transaction_shiftreg <= {transaction_shiftreg[0+:(TRANSACTION_WORD_SIZE*(TRANSACTION_DELAY-1))],r_valid,r_wr,r_addr,r_wrmask,r_wrdata};
        end
    
        assign {r_valid_delayed,r_wr_delayed,r_addr_delayed,r_wrmask_delayed,r_wrdata_delayed} = (TRANSACTION_DELAY == 0) ? {r_valid,r_wr,r_addr,r_wrmask,r_wrdata} : transaction_shiftreg[(TRANSACTION_WORD_SIZE*(TRANSACTION_DELAY-1))+:TRANSACTION_WORD_SIZE];
    
        ddr_address_map 
        #(
        .CONFIGURATION_DATA_BITS(CONFIGURATION_DATA_BITS),
	.CONFIGURATION_ADDR_BITS(CONFIGURATION_ADDR_BITS),
	.ADDR_BITS(ADDR_BITS),
	.BA_BITS(BA_BITS),
	.ROW_BITS(ROW_BITS),
	.COL_BITS(COL_BITS),
	.ADDRESS_FORMAT_RESET(ADDRESS_FORMAT_RESET),
	.CONFIGADDR_START_ADDRESS_FORMAT(CONFIGADDR_START_ADDRESS_FORMAT),
	.CONFIGADDR_END_ADDRESS_FORMAT(CONFIGADDR_END_ADDRESS_FORMAT),
	.BURST_BITS(BURST_BITS)
        )
        activate_map(
           .clk(clk), 
           .rst(rst),
           .config_valid(config_valid),
           .config_word(config_word),
           .config_addr(config_addr),
           .address_in(r_addr), 
           .bank_out(auto_activate_bank), 
           .row_out(auto_activate_row)
        );
    
        reg auto_precharge_idle_row_en;
        reg [9:0] auto_precharge_idle_row_limit;
        reg auto_activate_idle_bank_en;
        wire [CONFIGURATION_ADDR_BITS-1:0] c_addr = config_addr - CONFIGADDR_START_PERFTUNER;
       	
        always @(posedge clk) begin
            if (rst) begin
                auto_precharge_idle_row_en <= AP_BIT_RESET ? 0 : AUTO_PRECHARGE_IDLE_ROW_EN_RESET;
                auto_precharge_idle_row_limit <= AUTO_PRECHARGE_IDLE_ROW_LIMIT_RESET > TRANSACTION_DELAY ? AUTO_PRECHARGE_IDLE_ROW_LIMIT_RESET : TRANSACTION_DELAY+1;
                auto_activate_idle_bank_en <= AUTO_ACTIVATE_IDLE_BANK_EN_RESET;
                ap_bit <= AP_BIT_RESET;
    
            end else if (config_valid) begin
		if ((config_addr >= CONFIGADDR_START_PERFTUNER) && (config_addr <= CONFIGADDR_END_PERFTUNER)) begin
                    if (c_addr == 0)
                        auto_precharge_idle_row_en <= ap_bit ? 0 : config_word[0:0];
                    else if (c_addr == 1)
                        auto_precharge_idle_row_limit <= config_word[9:0] > TRANSACTION_DELAY[9:0] ? config_word[9:0] : TRANSACTION_DELAY[9:0]+10'd1;
                    else if (c_addr == 2)
                        auto_activate_idle_bank_en <= config_word[0:0];
                    else if (c_addr == 3) begin
                        ap_bit <= config_word[0:0];
                        if (config_word[0:0])
                            auto_precharge_idle_row_en <= 0;
                    end
                end
            end
        end
    
    
        wire activate_cmd = (active_cmd[3:0] == CMD_ACTIVATE) && active_cmd_valid ? 1'b1 : 1'b0;
        wire precharge_cmd = (active_cmd[3:0] == CMD_PRECHARGE) && active_cmd_valid ? 1'b1 : 1'b0;
    
        reg [(2**BA_BITS)-1:0] auto_precharge_counter_limit_reached;
        wire [(2**BA_BITS)-1:0] auto_precharge_request_masked =  {2**BA_BITS{auto_precharge_idle_row_en & (!bank_busy)}} & auto_precharge_counter_limit_reached &  auto_precharge_request;
    
        
        wire [(2**BA_BITS)-1:0] auto_activate_possible_masked = (r_valid  && !bank_busy && auto_activate_idle_bank_en) ? auto_activate_possible : 0;
    
    
        assign auto_precharge = auto_precharge_ack[selected_auto_precharge_bank];
        assign auto_activate = auto_activate_possible_masked[auto_activate_bank];
    
    
        integer i;
        always @(*) begin
            selected_auto_precharge_bank = 0;
            auto_precharge_ack = 0;
            for (i=0;i<2**BA_BITS;i=i+1) begin
                selected_auto_precharge_bank = auto_precharge_request_masked[i] ? i : selected_auto_precharge_bank;
                auto_activate_ack[i] = (i == auto_activate_bank) ? auto_activate_possible_masked[i] & (!activate_cmd) : 0;
            end
            auto_precharge_ack[selected_auto_precharge_bank] = auto_precharge_request_masked[selected_auto_precharge_bank] & (!precharge_cmd);
        end
        
    
       
    
        reg [31:0] auto_precharge_counters [0:(2**BA_BITS)-1];
    
    
        integer ii;
        always @(posedge clk) begin
            for (ii=0;ii<2**BA_BITS;ii=ii+1) begin
                if (rst) 
                    auto_precharge_counters[ii] <= 0;
                else if ((auto_activate_bank == ii) && active_cmd_valid)
                    auto_precharge_counters[ii] <= 0;
                else if (auto_precharge_request[ii] == 0)
                    auto_precharge_counters[ii] <= 0;
                else if ((auto_precharge_counters[ii] < auto_precharge_idle_row_limit) && !bank_busy && auto_precharge_request[ii])
                    auto_precharge_counters[ii] <= auto_precharge_counters[ii] + 32'd1;
            end
        end
    
    
        integer iii;
        always @(*) begin
            for (iii=0;iii<2**BA_BITS;iii=iii+1) begin
                auto_precharge_counter_limit_reached[iii] = (auto_precharge_counters[iii] == auto_precharge_idle_row_limit) ? 1'b1 : 1'b0;
            end
        end
    
    end else begin
        assign r_valid_delayed = r_valid;
        assign r_wr_delayed = r_wr; 
        assign r_addr_delayed = r_addr;
        assign r_wrmask_delayed = r_wrmask;
        assign r_wrdata_delayed = r_wrdata;
        assign auto_precharge = 0;
        assign auto_activate = 0;
        assign auto_activate_row = 0;
        assign auto_activate_bank = 0;
        assign empty = 1;
        initial begin
            auto_precharge_ack = 0;
            ap_bit = 0;
            selected_auto_precharge_bank = 0;
            auto_activate_ack = 0;
        end
        always @(*) begin
            auto_precharge_ack = 0;
            ap_bit = 0;
            selected_auto_precharge_bank = 0;
            auto_activate_ack = 0;
        end
    end
    endgenerate

endmodule


module ddr_bank_machine
(
		clk,
		rst,

		config_valid,
		config_word,
		config_addr,

		busy, 
		refresh,

		r_valid,
		r_bank,
		r_row,
		r_col,
		r_wr,
		r_wrmask,
		r_wrdata,

		command_valid, 
		selected_bank, 

		command_out, 
		slot_out,
		row_out, 
		col_out,
		wrmask_out,
		wrdata_out,
		can_precharge,
		
		auto_precharge_request, 
		auto_precharge_ack,
		ap_bit,
		
		auto_activate_possible,
		auto_activate_ack,
		auto_activate_row
	);
	
		
	parameter BANK_MACHINE_ID = 0;
	parameter CONFIGURATION_DATA_BITS = 32;
	parameter CONFIGURATION_ADDR_BITS = 32;
	parameter MASK_BITS = 16;
	parameter DATA_BITS = 128;
	parameter BA_BITS = 3;
	parameter ROW_BITS = 14;
	parameter COL_BITS = 10;
	parameter SIMULATION = 0;
	parameter COMMAND_WORD = 0; 
	parameter SLOT_BITS = 2;
	parameter MACRO_WORD = 128;
	parameter MACRO_COUNT_BITS = 8;
	parameter TIMER_BITS = 0;
	parameter CMD_READ = 0;
	parameter CMD_WRITE = 0;
	parameter CMD_ACTIVATE = 0;
	parameter CMD_PRECHARGE = 0;
	parameter TIMERID_0 = 0;
	parameter TIMERID_TRP = 0;
	parameter TIMERID_TRCD = 0;
	parameter TIMERID_TRTP_TWR_TRAS = 0;
	parameter TIMERID_TCCD = 0;
	parameter CONFIGADDR_START_TIMERS = 0;
	parameter CONFIGADDR_END_TIMERS = 0;
	parameter CONFIGADDR_START_RUNTIME_RULES = 0;
	parameter CONFIGADDR_END_RUNTIME_RULES = 0;
	parameter TIMER_TCCD_RESET = 0;
	parameter TIMER_TRP_RESET = 0;
	parameter TIMER_TRP_AP_RESET = 0;
	parameter TIMER_TRTP_RESET = 0;
	parameter TIMER_TRCD_RESET = 0;
	parameter TIMER_TRAS_RESET = 0;
	parameter TIMER_TWR_RESET = 0;
	parameter RUNTIME_RULES_INIT_HEX = "init_runtime_rules.hex";
	parameter MACRO_CONFIG_WORDS_NEEDED = 4;
	parameter MACRO_CONFIG_BITS_NEEDED = 2;
	
	input clk;
	input rst;

	input config_valid;
	input [CONFIGURATION_DATA_BITS-1:0] config_word;
	input [CONFIGURATION_ADDR_BITS-1:0] config_addr;

	output busy; 
	input refresh; 	
	input   [BA_BITS-1:0] selected_bank; 

	input r_valid;
	input r_wr; 
	input [MASK_BITS-1:0] r_wrmask;
	input [DATA_BITS-1:0] r_wrdata;
	input [BA_BITS-1:0] r_bank;
	input [COL_BITS-1:0] r_col;
	input  [ROW_BITS-1:0] r_row;

	

	output command_valid; 
	output [COMMAND_WORD-1:0]	command_out; 
	output [SLOT_BITS-1:0]	slot_out;
	output reg [MASK_BITS-1:0] wrmask_out;
	output reg [DATA_BITS-1:0] wrdata_out;
	output reg [COL_BITS-1:0] col_out;
	output reg  [ROW_BITS-1:0] row_out;

	output can_precharge;
	input ap_bit;
	output auto_precharge_request; 
	input  auto_precharge_ack;
	
	
	input auto_activate_ack;
	input [ROW_BITS-1:0] auto_activate_row;
	output  auto_activate_possible;


	reg	command_valid_int; 
	wire [COMMAND_WORD-1:0]	command_out_int; 
	wire [MACRO_WORD-1:0] macro ;
	reg [MACRO_WORD-MACRO_COUNT_BITS-1:0] active_macro ;
	reg [MACRO_COUNT_BITS-1:0] active_macro_counter;


	reg [2:0] trp;
	reg [2:0] trtp;
	reg [2:0] tccd;
	reg [2:0] trcd;
	reg [2:0] tras;
	reg [2:0] twr;

	wire [2:0] tccd_reset;
	wire [2:0] trp_reset;
	wire [2:0] trp_ap_reset;
	wire [2:0] trtp_reset;
	wire [2:0] trcd_reset;
	wire [2:0] tras_reset;
	wire [2:0] twr_reset;

    
  


	wire [3:0] issued_command;
	reg [ROW_BITS:0] activated_row;


	reg busy_macro;
	
	reg auto_activate_ap_hold;
	
	
	reg full_command_valid;
	reg [COMMAND_WORD + TIMER_BITS + SLOT_BITS-1 :0] full_command;


	reg [MACRO_WORD-MACRO_COUNT_BITS-1:0] active_macro_next ;
	reg [MACRO_COUNT_BITS-1:0] active_macro_counter_next;



    

    ddr_timer_db 
    #(
    .CONFIGURATION_DATA_BITS(CONFIGURATION_DATA_BITS),
	.CONFIGURATION_ADDR_BITS(CONFIGURATION_ADDR_BITS),
	.CONFIGADDR_START_TIMERS(CONFIGADDR_START_TIMERS),
	.CONFIGADDR_END_TIMERS(CONFIGADDR_END_TIMERS),
	.TIMER_TCCD_RESET(TIMER_TCCD_RESET),
	.TIMER_TRP_RESET(TIMER_TRP_RESET),
	.TIMER_TRP_AP_RESET(TIMER_TRP_AP_RESET),
	.TIMER_TRTP_RESET(TIMER_TRTP_RESET),
	.TIMER_TRCD_RESET(TIMER_TRCD_RESET),
	.TIMER_TRAS_RESET(TIMER_TRAS_RESET),
	.TIMER_TWR_RESET(TIMER_TWR_RESET)
    )
    timers(
		.clk(clk), 
		.config_valid(config_valid),
		.config_word(config_word),
		.config_addr(config_addr),
		.tccd(tccd_reset),
		.trp(trp_reset),
		.trp_ap(trp_ap_reset),
		.trtp(trtp_reset),
		.trcd(trcd_reset),
		.tras(tras_reset),
		.twr(twr_reset)
	);

	ddr_runtime_rules 
	#(
	.CONFIGURATION_DATA_BITS(CONFIGURATION_DATA_BITS),
	.CONFIGURATION_ADDR_BITS(CONFIGURATION_ADDR_BITS),
	.RUNTIME_RULES_INIT_HEX(RUNTIME_RULES_INIT_HEX),
	.MACRO_WORD(MACRO_WORD),
	.SIMULATION(SIMULATION),	
	.CONFIGADDR_START_RUNTIME_RULES(CONFIGADDR_START_RUNTIME_RULES),
	.CONFIGADDR_END_RUNTIME_RULES(CONFIGADDR_END_RUNTIME_RULES),
	.MACRO_CONFIG_WORDS_NEEDED(MACRO_CONFIG_WORDS_NEEDED),
	.MACRO_CONFIG_BITS_NEEDED(MACRO_CONFIG_BITS_NEEDED)
	)
	rules(
		.clk(clk), 
		.config_valid(config_valid),
		.config_word(config_word),
		.config_addr(config_addr),
		.row_open(((({1'b0,r_row} == activated_row) || (auto_activate_ack && (auto_activate_row == r_row))) && !(auto_precharge_ack || (BANK_MACHINE_ID == selected_bank) && ap_bit && (command_valid_int) && ((issued_command == CMD_READ) || (issued_command == CMD_WRITE)))) ? 1'b1 : 1'b0),
		.bank_precharged(activated_row[ROW_BITS] ),
		.r_wr(r_wr),
		.macro(macro)
	);
	
	assign command_out = command_out_int ;
    assign command_valid = command_valid_int ;

	assign can_precharge = ((trtp > 1) || (twr > 1) || (tras > 1) || busy_macro || command_valid_int || full_command_valid) ? 1'b0 : 1'b1;
	assign issued_command = command_out_int[3:0]; 
	
	assign busy  = busy_macro;
	assign command_out_int = full_command[COMMAND_WORD+SLOT_BITS+TIMER_BITS-1:SLOT_BITS+TIMER_BITS];
	assign slot_out = full_command[SLOT_BITS-1:0];

    assign auto_activate_possible = (activated_row[ROW_BITS] && (trp <= 1) && (trtp <= 1)  && (tras  <= 1) &&  !(active_macro_counter) && !(auto_activate_ap_hold)) ? 1'b1 : 1'b0;
    assign auto_precharge_request = (can_precharge && !activated_row[ROW_BITS]) ? 1'b1 : 1'b0;


    always @(posedge clk) begin
        if (rst)
            auto_activate_ap_hold <= 0;
        else if ((BANK_MACHINE_ID == selected_bank) && ap_bit && (command_valid_int) && ((issued_command == CMD_READ) || (issued_command == CMD_WRITE)))
            auto_activate_ap_hold <= 1'b1;
        else
            auto_activate_ap_hold <= 0;
    end
    

	always @(posedge clk) begin
		if (rst) begin
			row_out <= 0;
			col_out <= 0;
			wrmask_out <= 0;
			wrdata_out <= 0;
			full_command <= 0;
			full_command_valid <= 0;
		end else if (r_valid && (r_bank == BANK_MACHINE_ID)) begin
			row_out <= r_row;
			col_out <= r_col;
			wrmask_out <= r_wrmask;
			wrdata_out <= r_wrdata;
			full_command <= macro[COMMAND_WORD + TIMER_BITS + SLOT_BITS-1 :0];
			full_command_valid <= 1'b1;
		end else if (full_command_valid && command_valid_int && (BANK_MACHINE_ID==selected_bank)) begin
			full_command_valid <= active_macro_counter ? 1'b1 : 1'b0;
			full_command <= active_macro[COMMAND_WORD + TIMER_BITS + SLOT_BITS-1 :0];
		end
	end


	always @(posedge clk) begin
		active_macro <= active_macro_next;
		active_macro_counter <= active_macro_counter_next;
	end
	
	always @(*) begin
			if (rst) begin
				active_macro_next = 0;
				active_macro_counter_next = 0;
			end else begin
				if (active_macro_counter && command_valid_int && (BANK_MACHINE_ID==selected_bank)) begin
					active_macro_counter_next = active_macro_counter - 1;
					active_macro_next = active_macro >> (COMMAND_WORD+SLOT_BITS+TIMER_BITS);
				end else if (r_valid) begin
					active_macro_counter_next = macro[MACRO_WORD-1: MACRO_WORD-MACRO_COUNT_BITS]-1;
					active_macro_next = macro[MACRO_WORD-MACRO_COUNT_BITS-1:0] >> (COMMAND_WORD+SLOT_BITS+TIMER_BITS);
				end else begin
				 	active_macro_counter_next = active_macro_counter;
				 	active_macro_next = active_macro;	 
				end 	
			end
	end	


	always @(*) begin
		if (rst)
			busy_macro = 0;
		else if (full_command_valid && command_valid_int && (BANK_MACHINE_ID==selected_bank) && (active_macro==0) && !(ap_bit && ((issued_command == CMD_READ) || (issued_command == CMD_WRITE))))
			busy_macro = 0;
		else if (full_command_valid)
			busy_macro = 1;
		else
			busy_macro = 0;
	end



	always @(posedge clk) begin
			if (rst) 
				activated_row <= {(ROW_BITS)+1{1'b1}};
			else if (command_valid_int && (selected_bank == BANK_MACHINE_ID) && (issued_command == CMD_ACTIVATE))
				activated_row <= {1'b0,row_out};
			else if ((BANK_MACHINE_ID == selected_bank) && (command_valid_int) && (issued_command == CMD_PRECHARGE))
				activated_row <= {(ROW_BITS)+1{1'b1}};
			else if ((BANK_MACHINE_ID == selected_bank) && ap_bit && (command_valid_int) && ((issued_command == CMD_READ) || (issued_command == CMD_WRITE)))
				activated_row <= {(ROW_BITS)+1{1'b1}};
			else if (refresh) 
				activated_row <= {(ROW_BITS)+1{1'b1}};
			else if (auto_precharge_request & auto_precharge_ack)
				activated_row <= {(ROW_BITS)+1{1'b1}};
			else if (auto_activate_ack)
				activated_row <= {1'b0,auto_activate_row};
			else 
				activated_row <= activated_row;
	end


	always @(posedge clk) begin
		if (rst) begin
			trp <= trp_reset;
			trtp <= trp_reset;
			tccd <= tccd_reset;
			trcd <= trcd_reset;
			tras <= tras_reset;	
			twr <= twr_reset;	
		end else begin 
			trp  <= (auto_precharge_ack || (command_valid_int && (selected_bank == BANK_MACHINE_ID) && (issued_command == CMD_PRECHARGE))) ? trp_reset : (((BANK_MACHINE_ID == selected_bank) && ap_bit && (command_valid_int) && ((issued_command == CMD_READ) || (issued_command == CMD_WRITE))) ? trp_ap_reset :((trp > 0) ? trp - 3'd1 : 0));
			tccd  <= (command_valid_int && (selected_bank == BANK_MACHINE_ID) && ((issued_command[1:0] == 2'b01)))  ? tccd_reset : ((tccd> 0) ? tccd - 3'd1 : 0);
			trcd  <= (auto_activate_ack ||(command_valid_int && (selected_bank == BANK_MACHINE_ID) && (issued_command == CMD_ACTIVATE))) ? trcd_reset : ((trcd > 0) ? trcd - 3'd1 : 0);
			tras  <= (auto_activate_ack || (command_valid_int && (selected_bank == BANK_MACHINE_ID) && (issued_command == CMD_ACTIVATE))) ? tras_reset : ((tras > 0) ? tras - 3'd1 : 0);
			twr  <= (command_valid_int && (selected_bank == BANK_MACHINE_ID) && (issued_command == CMD_WRITE)) ? twr_reset : ((twr > 0) ? twr - 3'd1 : 0);
		    trtp  <= (command_valid_int && (selected_bank == BANK_MACHINE_ID) && (issued_command == CMD_READ)) ? trtp_reset : ((trtp > 0) ? trtp - 3'd1 : 0);
		
		end
	end



	always @(*) begin
			if (full_command_valid) begin
				if (full_command[TIMER_BITS+SLOT_BITS-1:SLOT_BITS] == TIMERID_0)
					command_valid_int = 1'b1;
				else if (full_command[TIMER_BITS+SLOT_BITS-1:SLOT_BITS] == TIMERID_TRP)
					command_valid_int = (trp <= 1) ? 1'b1 : 1'b0;
				else if (full_command[TIMER_BITS+SLOT_BITS-1:SLOT_BITS] == TIMERID_TRCD)
					command_valid_int = (trcd <= 1) ? 1'b1 : 1'b0;
				else if (full_command[TIMER_BITS+SLOT_BITS-1:SLOT_BITS] == TIMERID_TRTP_TWR_TRAS)
					command_valid_int = ((trtp <= 1) && (twr < 1) && (tras  < 1)) ? 1'b1 : 1'b0;
				else if (full_command[TIMER_BITS+SLOT_BITS-1:SLOT_BITS] == TIMERID_TCCD)
					command_valid_int = (tccd <= 1) ? 1'b1 : 1'b0;
				else 
					command_valid_int = 1'b0;
			end else begin
				command_valid_int= 0;
		    end
	end
endmodule






module ddr_algorithm
(
		clk,
		rst,


		config_valid,
		config_word,
		config_addr,

		
		refresh_busy,
		perftuner_shiftreg_empty,

		r_valid,
		r_wr,
		r_addr,
		r_wrmask,
		r_wrdata,
		r_busy,

		command,
		command_slot,
		command_valid,

		selected_row,
		selected_col,
		selected_bank,
		selected_address,
		selected_wrdata,
		selected_wrmask,
		
		cmd_odt,
		
		
		auto_precharge_request,
		auto_precharge_ack,
		ap_bit,
		
		auto_activate_possible,
		auto_activate_ack,
		auto_activate_row
	);
	
	parameter CONFIGURATION_DATA_BITS = 32;
	parameter CONFIGURATION_ADDR_BITS = 32;
	
	parameter ADDR_BITS = 32;
	parameter MASK_BITS = 16;
	parameter DATA_BITS = 128;
	parameter BA_BITS = 3;
	parameter ROW_BITS = 14;
	parameter COL_BITS = 10;
	parameter BURST_BITS = 3;
	
	parameter SIMULATION = 0;

	parameter CMD_READ = 0;
	parameter CMD_WRITE = 0;
	parameter CMD_ACTIVATE = 0;
	parameter CMD_PRECHARGE = 0;
	parameter CMD_REFRESH =  4;	
	parameter COMMAND_WORD = 11;
	parameter SLOT_BITS = 2;
	parameter MACRO_WORD = 128;
	parameter MACRO_COUNT_BITS = 8;
	parameter TIMER_BITS = 0;
	parameter MACRO_CONFIG_WORDS_NEEDED = 4;
	parameter MACRO_CONFIG_BITS_NEEDED = 2;
	
	parameter CONFIGADDR_START_TIMERS = 0;
	parameter CONFIGADDR_END_TIMERS = 0;
	parameter CONFIGADDR_START_RUNTIME_RULES = 0;
	parameter CONFIGADDR_END_RUNTIME_RULES = 0;
	parameter CONFIGADDR_START_REFRESH_RULES = 0;
	parameter CONFIGADDR_END_REFRESH_RULES = 0;
	parameter CONFIGADDR_START_ARBITER = 0;
	parameter CONFIGADDR_END_ARBITER = 0;
	parameter CONFIGADDR_START_ADDRESS_FORMAT = 10;
	parameter CONFIGADDR_END_ADDRESS_FORMAT = 10;
	
	parameter TIMERID_0 = 0;
	parameter TIMERID_TRP = 0;
	parameter TIMERID_TRCD = 0;
	parameter TIMERID_TRTP_TWR_TRAS = 0;
	parameter TIMERID_TCCD = 0;

	parameter TIMER_TCCD_RESET = 0;
	parameter TIMER_TRP_RESET = 0;
	parameter TIMER_TRP_AP_RESET = 0;
	parameter TIMER_TRTP_RESET = 0;
	parameter TIMER_TRCD_RESET = 0;
	parameter TIMER_TRAS_RESET = 0;
	parameter TIMER_TREFI_RESET = 0;
	parameter TIMER_TRFC_RESET = 0;
	parameter TIMER_TWTR_RESET = 0;
	parameter TIMER_TRTW_RESET = 0;
	parameter TIMER_TWR_RESET = 0;
	parameter MAX_REFRESH_DEBT = 0;
	parameter ADDRESS_FORMAT_RESET = 1;

	parameter REFRESH_RULES_INIT_HEX = "init_refresh_rules.hex";
	parameter RUNTIME_RULES_INIT_HEX = "init_runtime_rules.hex";
	parameter ARBITER_RULES_INIT_HEX = "init_arbiter_rules.hex";



	
	input clk; 
	input rst;

	input config_valid;
	input [CONFIGURATION_DATA_BITS-1:0] config_word;
	input [CONFIGURATION_ADDR_BITS-1:0] config_addr;
	

	output r_busy;	
    	output refresh_busy; 
   	input perftuner_shiftreg_empty;

	input r_valid;
	input r_wr; 
	input [ADDR_BITS-1:0] r_addr;
	input [MASK_BITS-1:0] r_wrmask;
	input [DATA_BITS-1:0] r_wrdata;


	output [SLOT_BITS-1:0] command_slot;
	output [COMMAND_WORD-1:0] command;
	output command_valid;

	output [BA_BITS-1:0] selected_bank;
	output [ROW_BITS-1:0] selected_row;
	output [COL_BITS-1:0] selected_col;
	output [ADDR_BITS-1:0] selected_address;
	output [MASK_BITS-1:0] selected_wrmask;
    	output [DATA_BITS-1:0] selected_wrdata;
	


	output [1:0] cmd_odt;
	
	input [(2**BA_BITS)-1:0] auto_precharge_ack;
	output [(2**BA_BITS)-1:0] auto_precharge_request;
	input ap_bit;
	
	
	output [(2**BA_BITS)-1:0] auto_activate_possible;
	input [(2**BA_BITS)-1:0] auto_activate_ack;
	input [ROW_BITS-1:0] auto_activate_row;



	wire [2:0] twtr_reset;
	wire [2:0] trtw_reset;
    	wire [31:0] refresh_debt_max;

	reg [2:0] twtr;
	reg [2:0] trtw;

	wire start_refresh;
	wire refresh_busy_int;
	wire [31:0] refresh_debt;
	wire [COMMAND_WORD-1:0] refresh_command;
	wire refresh_command_valid;

	reg odt_counter;

	wire [BA_BITS-1:0] r_bank;
	wire [COL_BITS-1:0] r_col;
	wire [ROW_BITS-1:0] r_row;
    	wire [BA_BITS-1:0] r_rd_bank; 

	wire [(2**BA_BITS)-1:0] busy_flags ;
	wire [DATA_BITS-1:0] write_words [0:(2**BA_BITS)-1];
	wire [MASK_BITS-1:0] write_masks [0:(2**BA_BITS)-1];
	wire [ROW_BITS-1:0] rows [0:(2**BA_BITS)-1];
	wire [COL_BITS-1:0] cols [0:(2**BA_BITS)-1];
	wire [COMMAND_WORD-1:0] commands [0:(2**BA_BITS)-1];
	wire [SLOT_BITS-1:0] slots [0:(2**BA_BITS)-1];
	wire [(2**BA_BITS)-1:0] bank_ready ;


	wire [(2**BA_BITS)-1:0] can_precharge;
	

 	assign start_refresh = (busy_flags == 0) && (bank_ready == 0) && (refresh_debt) && (!r_valid) && (can_precharge) && perftuner_shiftreg_empty && (!refresh_busy_int);
	assign r_busy = busy_flags[r_bank] | refresh_busy_int | start_refresh | (twtr && !r_wr) | (trtw && r_wr);
	assign refresh_busy = (refresh_debt >= refresh_debt_max); 

	assign cmd_odt = (((command[3:0] == CMD_WRITE) && command_valid) || odt_counter) ? 2'b11 : 2'b00;

	assign command_slot = slots[selected_bank];
	assign command = refresh_busy_int ? refresh_command : commands[selected_bank];
	assign command_valid = refresh_busy_int ? refresh_command_valid : bank_ready[selected_bank];
	assign selected_row = rows[selected_bank];
	assign selected_col = cols[selected_bank];
	assign selected_wrdata = write_words[selected_bank];
	assign selected_wrmask = write_masks[selected_bank];



	always @(posedge clk) begin
		if (rst) begin
			twtr <= twtr_reset;
			trtw <= trtw_reset;				
		end else begin 
			twtr <= (r_valid && r_wr) ? twtr_reset : ((twtr > 0) ? twtr- 3'd1 : 0);
			trtw <= (r_valid && !r_wr) ? trtw_reset : ((trtw > 0) ? trtw- 3'd1 : 0);
		end
	end

	always @(posedge clk) begin
        	if (rst) 
            		odt_counter <= 0;
        	else if (command[3:0] == CMD_WRITE)
            		odt_counter <= 1;
       	 	else
            		odt_counter <= 0;
	end



	ddr_timer_db 
	#(
	.CONFIGURATION_DATA_BITS(CONFIGURATION_DATA_BITS),
	.CONFIGURATION_ADDR_BITS(CONFIGURATION_ADDR_BITS),
	.CONFIGADDR_START_TIMERS(CONFIGADDR_START_TIMERS),
	.CONFIGADDR_END_TIMERS(CONFIGADDR_END_TIMERS),
	.TIMER_TCCD_RESET(TIMER_TCCD_RESET),
	.TIMER_TRP_RESET(TIMER_TRP_RESET),
	.TIMER_TRP_AP_RESET(TIMER_TRP_AP_RESET),
	.TIMER_TRTP_RESET(TIMER_TRTP_RESET),
	.TIMER_TRCD_RESET(TIMER_TRCD_RESET),
	.TIMER_TRAS_RESET(TIMER_TRAS_RESET),
	.TIMER_TREFI_RESET(TIMER_TREFI_RESET),
	.TIMER_TRFC_RESET(TIMER_TRFC_RESET),
	.TIMER_TWTR_RESET(TIMER_TWTR_RESET),
	.TIMER_TRTW_RESET(TIMER_TRTW_RESET),
	.TIMER_TWR_RESET(TIMER_TWR_RESET),
	.MAX_REFRESH_DEBT(MAX_REFRESH_DEBT)
	)
	timers(
		.clk(clk), 
		.config_valid(config_valid),
		.config_word(config_word),
		.config_addr(config_addr),
		.twtr(twtr_reset),
		.trtw(trtw_reset),
		.refresh_debt(refresh_debt_max)
	);
	
	

	ddr_refresh 
	#(
	.CONFIGURATION_DATA_BITS(CONFIGURATION_DATA_BITS),
	.CONFIGURATION_ADDR_BITS(CONFIGURATION_ADDR_BITS),
	.CMD_REFRESH(CMD_REFRESH),
	.COMMAND_WORD(COMMAND_WORD),
	.REFRESH_RULES_INIT_HEX(REFRESH_RULES_INIT_HEX),
	.SIMULATION(SIMULATION),
	.CONFIGADDR_START_REFRESH_RULES(CONFIGADDR_START_REFRESH_RULES),
	.CONFIGADDR_END_REFRESH_RULES(CONFIGADDR_END_REFRESH_RULES),
	.CONFIGADDR_START_TIMERS(CONFIGADDR_START_TIMERS),
	.CONFIGADDR_END_TIMERS(CONFIGADDR_END_TIMERS),
	.TIMER_TCCD_RESET(TIMER_TCCD_RESET),
	.TIMER_TRP_RESET(TIMER_TRP_RESET),
	.TIMER_TRP_AP_RESET(TIMER_TRP_AP_RESET),
	.TIMER_TRTP_RESET(TIMER_TRTP_RESET),
	.TIMER_TRCD_RESET(TIMER_TRCD_RESET),
	.TIMER_TRAS_RESET(TIMER_TRAS_RESET),
	.TIMER_TREFI_RESET(TIMER_TREFI_RESET),
	.TIMER_TRFC_RESET(TIMER_TRFC_RESET),
	.TIMER_TWTR_RESET(TIMER_TWTR_RESET),
	.TIMER_TRTW_RESET(TIMER_TRTW_RESET),
	.TIMER_TWR_RESET(TIMER_TWR_RESET),
	.MAX_REFRESH_DEBT(MAX_REFRESH_DEBT)
	)
	refresh(
		.clk(clk), 
		.rst(rst),
		.config_valid(config_valid),
		.config_word(config_word),
		.config_addr(config_addr),
		.busy(refresh_busy_int),
		.start_refresh(start_refresh),
		.refresh_debt(refresh_debt),
		.command(refresh_command),
		.command_valid(refresh_command_valid)
	);
	
	
	ddr_arbiter
	#(
	.CONFIGURATION_DATA_BITS(CONFIGURATION_DATA_BITS),
	.CONFIGURATION_ADDR_BITS(CONFIGURATION_ADDR_BITS),
	.BA_BITS(BA_BITS),
	.NUM_BANKS(2**BA_BITS),
	.SIMULATION(SIMULATION),
	.ARBITER_RULES_INIT_HEX(ARBITER_RULES_INIT_HEX),
	.CONFIGADDR_START_ARBITER(CONFIGADDR_START_ARBITER),
	.CONFIGADDR_END_ARBITER(CONFIGADDR_END_ARBITER)
	)
	arbiter (
		.clk(clk),
		.rst(rst), 
		.hold(refresh_busy_int || start_refresh),
		.request(bank_ready), 
		.grant(selected_bank), 
		.config_valid(config_valid),
		.config_word(config_word),
		.config_addr(config_addr)
	);
		


	ddr_address_map 
	#(
	.CONFIGURATION_DATA_BITS(CONFIGURATION_DATA_BITS),
	.CONFIGURATION_ADDR_BITS(CONFIGURATION_ADDR_BITS),
	.ADDR_BITS(ADDR_BITS),
	.BA_BITS(BA_BITS),
	.ROW_BITS(ROW_BITS),
	.COL_BITS(COL_BITS),
	.ADDRESS_FORMAT_RESET(ADDRESS_FORMAT_RESET),
	.CONFIGADDR_START_ADDRESS_FORMAT(CONFIGADDR_START_ADDRESS_FORMAT),
	.CONFIGADDR_END_ADDRESS_FORMAT(CONFIGADDR_END_ADDRESS_FORMAT),
	.BURST_BITS(BURST_BITS)
	) 
	ui_map(
		.clk(clk), 
		.rst(rst),
		.config_valid(config_valid),
		.config_word(config_word),
		.config_addr(config_addr),
		.address_in(r_addr), 
		.bank_out(r_bank), 
		.row_out(r_row), 
		.col_out(r_col), 
		.address_out(selected_address), 
		.bank_in(selected_bank), 
		.row_in(selected_row), 
		.col_in(selected_col) 
    );


    localparam NUM_BANKS = 2**BA_BITS;
    genvar i;
    generate
    	for (i=0; i<NUM_BANKS; i=i+1) begin
		   ddr_bank_machine 
		   #(
		   .BANK_MACHINE_ID(i),
		   .CONFIGURATION_DATA_BITS(CONFIGURATION_DATA_BITS),
		   .CONFIGURATION_ADDR_BITS(CONFIGURATION_ADDR_BITS),
		   .MASK_BITS(MASK_BITS),
		   .DATA_BITS(DATA_BITS),
		   .BA_BITS(BA_BITS),
		   .ROW_BITS(ROW_BITS),
		   .COL_BITS(COL_BITS),
		   .SIMULATION(SIMULATION),
		   .COMMAND_WORD(COMMAND_WORD),
		   .SLOT_BITS(SLOT_BITS),
		   .MACRO_WORD(MACRO_WORD),
		   .MACRO_COUNT_BITS(MACRO_COUNT_BITS),
		   .TIMER_BITS(TIMER_BITS),
		   .CMD_READ(CMD_READ),
		   .CMD_WRITE(CMD_WRITE),
		   .CMD_ACTIVATE(CMD_ACTIVATE),
		   .CMD_PRECHARGE(CMD_PRECHARGE),
		   .TIMERID_0(TIMERID_0),
		   .TIMERID_TRP(TIMERID_TRP),
		   .TIMERID_TRCD(TIMERID_TRCD),
		   .TIMERID_TRTP_TWR_TRAS(TIMERID_TRTP_TWR_TRAS),
		   .TIMERID_TCCD(TIMERID_TCCD),
		   .CONFIGADDR_START_TIMERS(CONFIGADDR_START_TIMERS),
		   .CONFIGADDR_END_TIMERS(CONFIGADDR_END_TIMERS),
		   .CONFIGADDR_START_RUNTIME_RULES(CONFIGADDR_START_RUNTIME_RULES),
		   .CONFIGADDR_END_RUNTIME_RULES(CONFIGADDR_END_RUNTIME_RULES),
		   .TIMER_TCCD_RESET(TIMER_TCCD_RESET),
		   .TIMER_TRP_RESET(TIMER_TRP_RESET),
		   .TIMER_TRP_AP_RESET(TIMER_TRP_AP_RESET),
		   .TIMER_TRTP_RESET(TIMER_TRTP_RESET),
		   .TIMER_TRCD_RESET(TIMER_TRCD_RESET),
		   .TIMER_TRAS_RESET(TIMER_TRAS_RESET),
		   .TIMER_TWR_RESET(TIMER_TWR_RESET),
		   .RUNTIME_RULES_INIT_HEX(RUNTIME_RULES_INIT_HEX),
		   .MACRO_CONFIG_WORDS_NEEDED(MACRO_CONFIG_WORDS_NEEDED),
		   .MACRO_CONFIG_BITS_NEEDED(MACRO_CONFIG_BITS_NEEDED)
		   )
		   bankmachine 
			(
				.clk(clk),
				.rst(rst),

				.config_valid(config_valid),
				.config_word(config_word),
				.config_addr(config_addr),

				.refresh(start_refresh),
				.busy(busy_flags[i]), 

				.r_valid(r_valid && !r_busy && (i == r_bank)),
				.r_bank(r_bank),
				.r_row(r_row),
				.r_col(r_col),
				.r_wr(r_wr),
				.r_wrmask(r_wrmask),
				.r_wrdata(r_wrdata),

				.command_valid(bank_ready[i]), 
				.selected_bank(selected_bank), 

				.command_out(commands[i]), 
				.slot_out(slots[i]),
				.row_out(rows[i]), 
				.col_out(cols[i]),
				.wrmask_out(write_masks[i]),
				.wrdata_out(write_words[i]),

				.can_precharge(can_precharge[i]),
				.ap_bit(ap_bit),
				.auto_precharge_request(auto_precharge_request[i]),
				.auto_precharge_ack(auto_precharge_ack[i]),
				
				.auto_activate_possible(auto_activate_possible[i]),
				.auto_activate_ack(auto_activate_ack[i]),
				.auto_activate_row(auto_activate_row)
			);
    	end
    endgenerate

endmodule






module ddr_controller  
(
	//Clocks
	clk_i, 
	ui_clk,

	// Resets
	rst_i, 
	ui_rst,
    
	// Configuration Bus
	config_valid,
	config_word,
	config_addr,

	// Run-time Simple Bus
	r_read,
	r_write,
	r_address,
	r_wrdata,
	r_wrstrb,
	r_rdvalid,
	r_rddata,
	r_rdaddress,
	r_ready,

	// IO
 	ddr_reset_n,ddr_ck_p,ddr_ck_n,ddr_cke,ddr_cs_n,ddr_ras_n,ddr_cas_n,ddr_we_n,
	ddr_dm,ddr_ba,ddr_addr,ddr_dq,ddr_dqs_p,ddr_dqs_n,ddr_odt
); 

	parameter CONFIGURATION_DATA_BITS = 32;
	parameter CONFIGURATION_ADDR_BITS = 32;
	
	parameter USER_ADDR_WIDTH = 32;
	parameter USER_MASK_WIDTH = 16;
	parameter USER_DATA_WIDTH = 128;

	localparam USER_ADDR_BITS = USER_ADDR_WIDTH;
	localparam USER_MASK_BITS = USER_MASK_WIDTH;
	localparam USER_DATA_BITS = USER_DATA_WIDTH;


	parameter DDR_ADDR_BITS = 3;
	parameter DDR_BA_WIDTH  = 3;
	parameter DDR_ROW_WIDTH  = 14;
	parameter DDR_COL_WIDTH  = 10;
	parameter BURST_SIZE = 8;
	parameter AP = 10;
	parameter NUM_SLOTS = 4;    
	parameter CKE_BITS = 1;
    parameter CK_BITS = 1;
    parameter CS_BITS = 1;
	parameter ODT_BITS = 1;
	parameter DM_BITS = 2;
	parameter DQS_BITS = 2;
	parameter DQ_BITS = 16;
	
	parameter SIMULATION = 0;

	parameter CMD_READ = 0;
	parameter CMD_WRITE = 0;
	parameter CMD_ACTIVATE = 0;
	parameter CMD_PRECHARGE = 0;
	parameter CMD_REFRESH =  4;	
	parameter COMMAND_WORD = 11;
	parameter MACRO_WORD = 128;
	parameter MACRO_COUNT_BITS = 8;
	parameter TIMER_BITS = 0;

	parameter ENABLE_PERFTUNER = 1;
	parameter TRANSACTION_DELAY = 0;
	parameter MACRO_CONFIG_WORDS_NEEDED = 4;
	parameter MACRO_CONFIG_BITS_NEEDED = 2;
	parameter CONFIGADDR_START_TIMERS = 0;
	parameter CONFIGADDR_END_TIMERS = 0;
	parameter CONFIGADDR_START_RUNTIME_RULES = 0;
	parameter CONFIGADDR_END_RUNTIME_RULES = 0;
	parameter CONFIGADDR_START_REFRESH_RULES = 0;
	parameter CONFIGADDR_END_REFRESH_RULES = 0;
	parameter CONFIGADDR_START_ARBITER = 0;
	parameter CONFIGADDR_END_ARBITER = 0;
	parameter CONFIGADDR_START_ADDRESS_FORMAT = 10;
	parameter CONFIGADDR_END_ADDRESS_FORMAT = 10;
    parameter CONFIGADDR_START_PERFTUNER = 0;
	parameter CONFIGADDR_END_PERFTUNER = 0;
	
	parameter TIMERID_0 = 0;
	parameter TIMERID_TRP = 0;
	parameter TIMERID_TRCD = 0;
	parameter TIMERID_TRTP_TWR_TRAS = 0;
	parameter TIMERID_TCCD = 0;

	parameter TIMER_TCCD_RESET = 0;
	parameter TIMER_TRP_RESET = 0;
	parameter TIMER_TRP_AP_RESET = 0;
	parameter TIMER_TRTP_RESET = 0;
	parameter TIMER_TRCD_RESET = 0;
	parameter TIMER_TRAS_RESET = 0;
	parameter TIMER_TREFI_RESET = 0;
	parameter TIMER_TRFC_RESET = 0;
	parameter TIMER_TWTR_RESET = 0;
	parameter TIMER_TRTW_RESET = 0;
	parameter TIMER_TWR_RESET = 0;
	parameter MAX_REFRESH_DEBT = 0;
	parameter ADDRESS_FORMAT_RESET = 1;

	parameter REFRESH_RULES_INIT_HEX = "init_refresh_rules.hex";
	parameter RUNTIME_RULES_INIT_HEX = "init_runtime_rules.hex";
	parameter ARBITER_RULES_INIT_HEX = "init_arbiter_rules.hex";

	parameter AP_BIT_RESET = 0;
	parameter AUTO_PRECHARGE_IDLE_ROW_EN_RESET = 0;
	parameter AUTO_PRECHARGE_IDLE_ROW_LIMIT_RESET = 0;
	parameter AUTO_ACTIVATE_IDLE_BANK_EN_RESET = 0;

	localparam BURST_BITS = $clog2(BURST_SIZE);
	localparam SLOT_BITS = $clog2(NUM_SLOTS);
    




	input clk_i; 
	output ui_clk;

	// Resets
	input rst_i;
	output ui_rst;

	// Configuration Bus
	input config_valid;
	input [CONFIGURATION_DATA_BITS-1:0] config_word;
	input [CONFIGURATION_ADDR_BITS-1:0] config_addr;
	

	// Run-time Bus
	input r_read;
	input r_write;
	input [USER_ADDR_BITS-1:0] r_address;
	input [USER_DATA_BITS-1:0] r_wrdata;
	input [USER_MASK_BITS-1:0] r_wrstrb;
	output reg r_rdvalid;
	output reg [USER_DATA_BITS-1:0] r_rddata;
	output  [USER_ADDR_BITS-1:0] r_rdaddress;
	output r_ready;
	wire r_busy;

	// IO
	output ddr_reset_n; output [CKE_BITS-1:0] ddr_cke; output [CK_BITS-1:0] ddr_ck_p; output [CK_BITS-1:0]  ddr_ck_n;
	output [CS_BITS-1:0] ddr_cs_n; output ddr_ras_n; output ddr_cas_n; output ddr_we_n;
	output [DDR_BA_WIDTH-1:0] ddr_ba; output [DDR_ADDR_BITS-1:0] ddr_addr; output [ODT_BITS-1:0] ddr_odt; output [DM_BITS-1:0] ddr_dm;
	inout [DQS_BITS-1:0] ddr_dqs_p; inout [DQS_BITS-1:0] ddr_dqs_n; inout [DQ_BITS-1:0] ddr_dq;

	wire bank_busy;	
    wire refresh_busy;
    
	wire r_valid;
	wire r_wr; 
	wire [USER_ADDR_BITS-1:0] r_addr;
	wire [USER_MASK_BITS-1:0] r_wrmask;
	wire  [USER_ADDR_BITS-1:0] r_rdaddr;

	wire [USER_DATA_BITS-1:0] r_rddata_int;
	wire  r_rdvalid_int;
	
	wire [COMMAND_WORD-1:0] command;

	wire [USER_DATA_BITS-1:0] wrdata;
	wire [USER_MASK_BITS-1:0] wrmask;


	wire [DDR_BA_WIDTH-1:0] selected_bank;
	wire [DDR_ROW_WIDTH-1:0] selected_row;
	wire [DDR_COL_WIDTH-1:0] selected_col;
	wire [USER_ADDR_BITS-1:0] selected_address;
	
	wire [SLOT_BITS-1:0] cmd_slot;
	wire cmd_valid;
	

	
	// PHY 
	wire [NUM_SLOTS-1:0]                    cmd_ras_n;
	wire [NUM_SLOTS-1:0]                    cmd_cas_n;
	wire [NUM_SLOTS-1:0]                    cmd_we_n;
	wire [NUM_SLOTS-1:0]                    cmd_cs_n;
	wire [1:0]                     		cmd_odt;
	wire [2:0]                     		cmd_cmd;
	wire [3:0]                     		cmd_aux_out0;
	wire [3:0]                     		cmd_aux_out1;
	wire [1:0]                     		cmd_rank_cnt;
	wire                           		cmd_idle;
	wire [5:0]                     		cmd_data_offset;
	wire [5:0]                     		cmd_data_offset_1;
	wire [5:0]                     		cmd_data_offset_2;
	wire                           		cmd_wrdata_en;
	wire [(NUM_SLOTS*DDR_ADDR_BITS)-1:0]        cmd_address;
	wire [(NUM_SLOTS*DDR_BA_WIDTH)-1:0]          cmd_bank;
	wire [31:0] 				ddr_status_reg;
	
	
	wire [(2**DDR_BA_WIDTH)-1:0] auto_precharge_ack;
	wire [(2**DDR_BA_WIDTH)-1:0] auto_precharge_request;
	wire ap_bit;
	wire [DDR_BA_WIDTH-1:0] selected_auto_precharge_bank;
    	wire auto_precharge;
	
	wire [(2**DDR_BA_WIDTH)-1:0] auto_activate_possible;
	wire [(2**DDR_BA_WIDTH)-1:0] auto_activate_ack;
	wire [DDR_ROW_WIDTH-1:0] auto_activate_row;
	wire [DDR_BA_WIDTH-1:0] auto_activate_bank;
    	wire auto_activate;


	wire r_valid_delayed;
	wire r_wr_delayed; 
	wire [USER_ADDR_BITS-1:0] r_addr_delayed;
	wire [USER_MASK_BITS-1:0] r_wrmask_delayed;
	wire [USER_DATA_BITS-1:0] r_wrdata_delayed;
	
	wire perftuner_shiftreg_empty;


    always @(posedge ui_clk) begin
        if (ui_rst) begin
            r_rddata <= 0;
            r_rdvalid <= 0;
        end else begin
            r_rddata <= r_rddata_int;
            r_rdvalid <= r_rdvalid_int;
	    end
    end
    
    
	assign bank_busy = bank_busy_int | refresh_busy;
	assign r_busy = bank_busy;
	assign r_ready = !r_busy;
	assign r_valid = r_read | r_write;
	assign r_wr = r_write; 
	assign r_addr = r_address;
	assign r_wrmask = r_wrstrb;
	assign r_rdaddress = r_rdaddr;

	ddr_algorithm 
	#(
	.CONFIGURATION_DATA_BITS(CONFIGURATION_DATA_BITS),
	.CONFIGURATION_ADDR_BITS(CONFIGURATION_ADDR_BITS),
	.ADDR_BITS(USER_ADDR_BITS),
	.MASK_BITS(USER_MASK_BITS),
	.DATA_BITS(USER_DATA_BITS),
	.BA_BITS(DDR_BA_WIDTH),
	.ROW_BITS(DDR_ROW_WIDTH),
	.COL_BITS(DDR_COL_WIDTH),
	.BURST_BITS(BURST_BITS),
	.SIMULATION(SIMULATION),
	.CMD_READ(CMD_READ),
	.CMD_WRITE(CMD_WRITE),
	.CMD_ACTIVATE(CMD_ACTIVATE),
	.CMD_PRECHARGE(CMD_PRECHARGE),
	.CMD_REFRESH(CMD_REFRESH),	
	.COMMAND_WORD(COMMAND_WORD),
	.SLOT_BITS(SLOT_BITS),
	.MACRO_WORD(MACRO_WORD),
	.MACRO_COUNT_BITS(MACRO_COUNT_BITS),
	.TIMER_BITS(TIMER_BITS),
	.MACRO_CONFIG_WORDS_NEEDED(MACRO_CONFIG_WORDS_NEEDED),
	.MACRO_CONFIG_BITS_NEEDED(MACRO_CONFIG_BITS_NEEDED),
	.CONFIGADDR_START_TIMERS(CONFIGADDR_START_TIMERS),
	.CONFIGADDR_END_TIMERS(CONFIGADDR_END_TIMERS),
	.CONFIGADDR_START_RUNTIME_RULES(CONFIGADDR_START_RUNTIME_RULES),
	.CONFIGADDR_END_RUNTIME_RULES(CONFIGADDR_END_RUNTIME_RULES),
	.CONFIGADDR_START_REFRESH_RULES(CONFIGADDR_START_REFRESH_RULES),
	.CONFIGADDR_END_REFRESH_RULES(CONFIGADDR_END_REFRESH_RULES),
	.CONFIGADDR_START_ARBITER(CONFIGADDR_START_ARBITER),
	.CONFIGADDR_END_ARBITER(CONFIGADDR_END_ARBITER),
	.CONFIGADDR_START_ADDRESS_FORMAT(CONFIGADDR_START_ADDRESS_FORMAT),
	.CONFIGADDR_END_ADDRESS_FORMAT(CONFIGADDR_END_ADDRESS_FORMAT),
	.TIMERID_0(TIMERID_0),
	.TIMERID_TRP(TIMERID_TRP),
	.TIMERID_TRCD(TIMERID_TRCD),
	.TIMERID_TRTP_TWR_TRAS(TIMERID_TRTP_TWR_TRAS),
	.TIMERID_TCCD(TIMERID_TCCD),
	.TIMER_TCCD_RESET(TIMER_TCCD_RESET),
	.TIMER_TRP_RESET(TIMER_TRP_RESET),
	.TIMER_TRP_AP_RESET(TIMER_TRP_AP_RESET),
	.TIMER_TRTP_RESET(TIMER_TRTP_RESET),
	.TIMER_TRCD_RESET(TIMER_TRCD_RESET),
	.TIMER_TRAS_RESET(TIMER_TRAS_RESET),
	.TIMER_TREFI_RESET(TIMER_TREFI_RESET),
	.TIMER_TRFC_RESET(TIMER_TRFC_RESET),
	.TIMER_TWTR_RESET(TIMER_TWTR_RESET),
	.TIMER_TRTW_RESET(TIMER_TRTW_RESET),
	.TIMER_TWR_RESET(TIMER_TWR_RESET),
	.MAX_REFRESH_DEBT(MAX_REFRESH_DEBT),
	.ADDRESS_FORMAT_RESET(ADDRESS_FORMAT_RESET),
	.REFRESH_RULES_INIT_HEX(REFRESH_RULES_INIT_HEX),
	.RUNTIME_RULES_INIT_HEX(RUNTIME_RULES_INIT_HEX),
	.ARBITER_RULES_INIT_HEX(ARBITER_RULES_INIT_HEX)
	)
	algo
	(
		.clk(ui_clk),
		.rst(ui_rst),

		.config_valid(config_valid),
		.config_word(config_word),
		.config_addr(config_addr),

		.refresh_busy(refresh_busy),
		.perftuner_shiftreg_empty(perftuner_shiftreg_empty),

		.r_valid(r_valid_delayed),
		.r_wr(r_wr_delayed),
		.r_addr(r_addr_delayed),
		.r_wrmask(r_wrmask_delayed),
		.r_wrdata(r_wrdata_delayed),
		.r_busy(bank_busy_int),


		.command(command),
		.command_slot(cmd_slot),
		.command_valid(cmd_valid),

		.selected_row(selected_row),
		.selected_col(selected_col),
		.selected_bank(selected_bank),
		.selected_address(selected_address),
		.selected_wrdata(wrdata),
		.selected_wrmask(wrmask),
		
		.cmd_odt(cmd_odt),
		
		
		.auto_precharge_request(auto_precharge_request),
		.auto_precharge_ack(auto_precharge_ack),
		.ap_bit(ap_bit),
		
		.auto_activate_possible(auto_activate_possible),
		.auto_activate_ack(auto_activate_ack),
		.auto_activate_row(auto_activate_row)
	);


	ddr_perftuner 
	#(
	.ENABLE_PERFTUNER(ENABLE_PERFTUNER),	
	.CONFIGURATION_DATA_BITS(CONFIGURATION_DATA_BITS),
	.CONFIGURATION_ADDR_BITS(CONFIGURATION_ADDR_BITS),
	.ADDR_BITS(USER_ADDR_BITS),
	.MASK_BITS(USER_MASK_BITS),
	.DATA_BITS(USER_DATA_BITS),
	.AP_BIT_RESET(AP_BIT_RESET),
	.AUTO_PRECHARGE_IDLE_ROW_EN_RESET(AUTO_PRECHARGE_IDLE_ROW_EN_RESET),
	.TRANSACTION_DELAY(TRANSACTION_DELAY),
	.AUTO_PRECHARGE_IDLE_ROW_LIMIT_RESET(AUTO_PRECHARGE_IDLE_ROW_LIMIT_RESET),
	.AUTO_ACTIVATE_IDLE_BANK_EN_RESET(AUTO_ACTIVATE_IDLE_BANK_EN_RESET),
	.CMD_ACTIVATE(CMD_ACTIVATE),
    .CMD_PRECHARGE(CMD_PRECHARGE),
    .CONFIGADDR_START_PERFTUNER(CONFIGADDR_START_PERFTUNER),
	.CONFIGADDR_END_PERFTUNER(CONFIGADDR_END_PERFTUNER),
	.BA_BITS(DDR_BA_WIDTH),
	.ROW_BITS(DDR_ROW_WIDTH),
	.COL_BITS(DDR_COL_WIDTH),
	.ADDRESS_FORMAT_RESET(ADDRESS_FORMAT_RESET),
	.CONFIGADDR_START_ADDRESS_FORMAT(CONFIGADDR_START_ADDRESS_FORMAT),
	.CONFIGADDR_END_ADDRESS_FORMAT(CONFIGADDR_END_ADDRESS_FORMAT),
	.BURST_BITS(BURST_BITS),
	.COMMAND_WORD(COMMAND_WORD)
	)
	perftuner(
		.clk(ui_clk),
		.rst(ui_rst),

		.config_valid(config_valid),
		.config_word(config_word),
		.config_addr(config_addr),
		
		.bank_busy(bank_busy_int),
		.empty(perftuner_shiftreg_empty),

		.r_valid(r_valid & (!refresh_busy)),
		.r_wr(r_wr),
		.r_addr(r_addr),
		.r_wrmask(r_wrmask),
		.r_wrdata(r_wrdata),

		.r_valid_delayed(r_valid_delayed),
		.r_wr_delayed(r_wr_delayed),
		.r_addr_delayed(r_addr_delayed),
		.r_wrmask_delayed(r_wrmask_delayed),
		.r_wrdata_delayed(r_wrdata_delayed),

		.auto_precharge_request(auto_precharge_request),
		.auto_precharge_ack(auto_precharge_ack),
		
		.auto_activate_possible(auto_activate_possible),
		.auto_activate_ack(auto_activate_ack),
		
		.selected_auto_precharge_bank(selected_auto_precharge_bank),
	    .auto_precharge(auto_precharge),
	    .ap_bit(ap_bit),
	    .auto_activate(auto_activate),
	    .auto_activate_bank(auto_activate_bank),
	    .auto_activate_row(auto_activate_row),
	    	
	    .active_cmd(command),
		.active_cmd_valid(cmd_valid),
		.active_cmd_bank(selected_bank)
		);


	ddr_command_loader 
	#(
	.SLOT_BITS(SLOT_BITS),
	.COMMAND_WORD(COMMAND_WORD),
	.BA_BITS(DDR_BA_WIDTH),
	.ROW_BITS(DDR_ROW_WIDTH),
	.COL_BITS(DDR_COL_WIDTH),
	.NUM_SLOTS(NUM_SLOTS),
	.ADDR_BITS(DDR_ADDR_BITS),
	.CMD_WRITE(CMD_WRITE),
	.CMD_READ(CMD_READ),
	.AP(AP)
	)
	command_loader(
		.cmd_slot(cmd_slot),
		.cmd_valid(cmd_valid),
		.active_cmd(command),


		.row(selected_row),
		.col(selected_col),
		.bank(selected_bank),
			
		.selected_auto_precharge_bank(selected_auto_precharge_bank),
	    .auto_precharge(auto_precharge),
	    .ap_bit(ap_bit),
	    .auto_activate(auto_activate),
	    .auto_activate_bank(auto_activate_bank),
	    .auto_activate_row(auto_activate_row),

		    // PHY Interface
		.ddr_status_reg(ddr_status_reg),
		.cmd_ras_n(cmd_ras_n),
	    .cmd_cas_n(cmd_cas_n),
	    .cmd_we_n(cmd_we_n),
	    .cmd_cs_n(cmd_cs_n),
	    .cmd_cmd(cmd_cmd),
	    .cmd_aux_out0(cmd_aux_out0),
	    .cmd_aux_out1(cmd_aux_out1),
	    .cmd_rank_cnt(cmd_rank_cnt),
	    .cmd_idle(cmd_idle),
	    .cmd_data_offset(cmd_data_offset),
	    .cmd_data_offset_1(cmd_data_offset_1),
	    .cmd_data_offset_2(cmd_data_offset_2),
	    .cmd_wrdata_en(cmd_wrdata_en),
	    .cmd_address(cmd_address),
	    .cmd_bank(cmd_bank)
	);



ddr_fifo #(.DATA_WIDTH(USER_ADDR_BITS)) fifo(
		.clk(ui_clk),
		.rst(ui_rst),
		.data_in_data(selected_address),
		.data_in_ready(),
		.data_in_valid((cmd_valid && (command[3:0] == CMD_READ)) ? 1'b1: 1'b0),
		.data_out_data(r_rdaddr),
		.data_out_ready(r_rdvalid_int),
		.data_out_valid()
	);
	

	ddr_phy phy(
		.clk_in(clk_i),
		.rst_in(rst_i),
		.clk_out(ui_clk),
		.rst_out(ui_rst),

		.cmd_ras_n(cmd_ras_n),
    	.cmd_cas_n(cmd_cas_n),
    	.cmd_we_n(cmd_we_n),
    	.cmd_cs_n(cmd_cs_n),
    	.cmd_odt(cmd_odt),
    	.cmd_cmd(cmd_cmd),
    	.cmd_aux_out0(cmd_aux_out0),
    	.cmd_aux_out1(cmd_aux_out1),
    	.cmd_rank_cnt(cmd_rank_cnt),
    	.cmd_idle(cmd_idle),
    	.cmd_data_offset(cmd_data_offset),
    	.cmd_data_offset_1(cmd_data_offset_1),
    	.cmd_data_offset_2(cmd_data_offset_2),
    	.cmd_wrdata_en(cmd_wrdata_en),
    	.cmd_address(cmd_address),
    	.cmd_bank(cmd_bank),

    	.cmd_wr_mask(wrmask),
    	.cmd_wr_data(wrdata),
		.ddr_rd_data(r_rddata_int),
		.ddr_rd_valid(r_rdvalid_int),

		.status_reg(ddr_status_reg),

		.ddr3_dq(ddr_dq),
		.ddr3_dqs_n(ddr_dqs_n),
		.ddr3_dqs_p(ddr_dqs_p),
		.ddr3_addr(ddr_addr),
		.ddr3_ba(ddr_ba),
		.ddr3_ras_n(ddr_ras_n),
		.ddr3_cas_n(ddr_cas_n),
		.ddr3_we_n(ddr_we_n),
		.ddr3_reset_n(ddr_reset_n),
		.ddr3_ck_p(ddr_ck_p),
		.ddr3_ck_n(ddr_ck_n),
		.ddr3_cke(ddr_cke),
		.ddr3_cs_n(ddr_cs_n),
		.ddr3_dm(ddr_dm),
		.ddr3_odt(ddr_odt)
	);	
endmodule
