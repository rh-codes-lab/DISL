`timescale 1ps/1ps
module ddr_command_loader(
   // Command bus
	cmd_valid,
	cmd_slot,
	active_cmd,
	//pending_reads,
	//pending_writes,
	row,
	col,
	bank,

	selected_auto_precharge_bank,
	auto_precharge,
	ap_bit,

	auto_activate,
	auto_activate_bank,
	auto_activate_row,
		
	// PHY bus
	ddr_status_reg,
	cmd_ras_n,
	cmd_cas_n,
	cmd_we_n,
	cmd_cs_n,
	cmd_cmd,
	cmd_aux_out0,
	cmd_aux_out1,
	cmd_rank_cnt,
	cmd_idle,
	cmd_data_offset,
	cmd_data_offset_1,
	cmd_data_offset_2,
	cmd_wrdata_en,
	cmd_address,
	cmd_bank
  );


	parameter SLOT_BITS = 2;
	parameter COMMAND_WORD = 11;
	parameter BA_BITS = 3;
	parameter ROW_BITS = 14;
	parameter COL_BITS = 10;
	parameter NUM_SLOTS = 4;
	parameter ADDR_BITS = 14;
	parameter CMD_WRITE = 0;
	parameter CMD_READ = 0;
	parameter AP = 10;

	input                                   cmd_valid;
	input      [SLOT_BITS-1:0]              cmd_slot;
	input      [COMMAND_WORD-1:0]           active_cmd;

	input      [BA_BITS-1:0]                bank;
	input      [ROW_BITS-1:0]               row;
	input      [COL_BITS-1:0]               col;

	input      [31:0]                       ddr_status_reg;
	//  input                                   pending_reads;
	//  input   [0:(2**BA_BITS)-1]              pending_writes;
	output reg [NUM_SLOTS-1:0]              cmd_ras_n;
	output reg [NUM_SLOTS-1:0]              cmd_cas_n;
	output reg [NUM_SLOTS-1:0]              cmd_we_n;
	output reg [NUM_SLOTS-1:0]              cmd_cs_n;
	output reg [2:0]                        cmd_cmd;
	output  [3:0]                        cmd_aux_out0;
	output  [3:0]                        cmd_aux_out1;
	output  [1:0]                        cmd_rank_cnt;
	output                               cmd_idle;
	output  [5:0]                        cmd_data_offset;
	output  [5:0]                        cmd_data_offset_1;
	output  [5:0]                        cmd_data_offset_2;
	output reg                              cmd_wrdata_en;
	output reg [(NUM_SLOTS*ADDR_BITS)-1:0]  cmd_address;
	output reg [(NUM_SLOTS*BA_BITS)-1:0]    cmd_bank;
	input [BA_BITS-1:0] selected_auto_precharge_bank;
	input auto_precharge;
	input ap_bit;
	input auto_activate;
	input [BA_BITS-1:0]         auto_activate_bank;
	input [ROW_BITS-1:0]        auto_activate_row;

	assign cmd_data_offset = (active_cmd[3:0] == CMD_WRITE) ? 6'd7 + cmd_slot : ((active_cmd[3:0] == CMD_READ) ? ddr_status_reg[5:0] + cmd_slot : 0); 
	assign cmd_data_offset_1 = (active_cmd[3:0] == CMD_WRITE) ? 6'd7 + cmd_slot : ((active_cmd[3:0] == CMD_READ) ? ddr_status_reg[13:8] + cmd_slot : 0); //ddr_status_reg[13:8] + cmd_slot;
	assign cmd_data_offset_2 = (active_cmd[3:0] == CMD_WRITE) ? 6'd7 + cmd_slot : ((active_cmd[3:0] == CMD_READ) ? ddr_status_reg[21:16] + cmd_slot : 0);//ddr_status_reg[31:16] + cmd_slot;
	assign cmd_aux_out0 = 0;
	assign cmd_aux_out1 = 0;
	assign cmd_rank_cnt = 0;
	assign cmd_idle =  0;// ((pending_reads == 0) && (pending_writes == 0)) ? 1'b1 : 1'b0;

  // ----------------- NEED TO PARAMETERIZE THIS ALWAYS BLOCK ---------------
  always @(*) begin
    if (cmd_valid) begin
      cmd_cmd = active_cmd[6:4];
      cmd_wrdata_en = active_cmd[7];
      if (cmd_slot == 2'b00) begin
            cmd_ras_n = {1'b1, !auto_precharge, 1'b1, active_cmd[0]};
            cmd_cas_n = {1'b1, 1'b1, 1'b1, active_cmd[1]};
            cmd_we_n = {1'b1, !auto_precharge, 1'b1, active_cmd[2]};
            cmd_cs_n = {1'b1, !auto_precharge, 1'b1, active_cmd[3]};
            cmd_address =  {14'h3FFF, {14{!auto_precharge}}, 14'h3FFF, (active_cmd[10:9] == 2'b01) ? row : ((active_cmd[10:9] == 2'b10) ? {3'd0,ap_bit,col} : ((active_cmd[10:9] == 2'b11) ? (14'd1 << AP) : 14'd0 ))};
            cmd_bank =   {3'h7, selected_auto_precharge_bank, 3'h7, active_cmd[8] ? bank : 3'h7};
      
      end else if (cmd_slot == 2'b01) begin
            cmd_ras_n = {1'b1, !auto_precharge, active_cmd[0], !auto_activate};
            cmd_cas_n = {1'b1, 1'b1, active_cmd[1], 1'b1};
            cmd_we_n = {1'b1, !auto_precharge, active_cmd[2], 1'b1};
            cmd_cs_n = {1'b1, !auto_precharge, active_cmd[3], !auto_activate};
            cmd_address =  {14'h3FFF, {14{!auto_precharge}}, (active_cmd[10:9] == 2'b01) ? row : ((active_cmd[10:9] == 2'b10) ? {3'd0,ap_bit,col} : ((active_cmd[10:9] == 2'b11) ? (14'd1 << AP) : 14'd0 )), auto_activate_row};
            cmd_bank = {3'h7, selected_auto_precharge_bank, active_cmd[8] ? bank : 3'h7, auto_activate_bank};
        
            
      end else if (cmd_slot == 2'b10) begin
            cmd_ras_n = {1'b1, active_cmd[0], 1'b1, !auto_activate};
            cmd_cas_n = {1'b1, active_cmd[1], 1'b1, 1'b1};
            cmd_we_n = {1'b1, active_cmd[2], 1'b1, 1'b1};
            cmd_cs_n = {1'b1, active_cmd[3], 1'b1, !auto_activate};
            cmd_address = {14'h3FFF, (active_cmd[10:9] == 2'b01) ? row : ((active_cmd[10:9] == 2'b10) ? {3'd0,ap_bit,col} : ((active_cmd[10:9] == 2'b11) ? (14'd1 << AP) : 14'd0 )), 14'h3FFF, auto_activate_row};
            cmd_bank =   {3'h7, active_cmd[8] ? bank : 3'h7, 3'h7, auto_activate_bank};
        
      end else begin
            cmd_ras_n = {active_cmd[0], !auto_precharge, 1'b1, !auto_activate};
            cmd_cas_n = {active_cmd[1], 1'b1, 1'b1, 1'b1};
            cmd_we_n = {active_cmd[2], !auto_precharge, 1'b1, 1'b1};
            cmd_cs_n = {active_cmd[3], !auto_precharge, 1'b1, !auto_activate};
            cmd_address =  {(active_cmd[10:9] == 2'b01) ? row : ((active_cmd[10:9] == 2'b10) ? {3'd0,ap_bit,col} : ((active_cmd[10:9] == 2'b11) ? (14'd1 << AP) : 14'd0 )), {14{!auto_precharge}}, 14'h3FFF, auto_activate_row};
            cmd_bank =  {active_cmd[8] ? bank : 3'h7, selected_auto_precharge_bank, 3'h7, auto_activate_bank};
        
      end
    end else begin
      // set pins to NOP mode
            cmd_ras_n = {1'b1, !auto_precharge, 1'b1, !auto_activate};
            cmd_cas_n = {1'b1, 1'b1, 1'b1, 1'b1};
            cmd_we_n = {1'b1, !auto_precharge, 1'b1, 1'b1};
            cmd_cs_n = {1'b1, !auto_precharge, 1'b1, !auto_activate};
            cmd_address =  {14'h3FFF, {14{!auto_precharge}}, 14'h3FFF, auto_activate_row};
            cmd_bank =  {3'h7, selected_auto_precharge_bank, 3'h7, auto_activate_bank};
            cmd_cmd = 3'd4;
            cmd_wrdata_en = 1'b0;
    end
  end
endmodule