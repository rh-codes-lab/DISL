module jtag_chip_manager(
clk,rst,control
);

parameter JTAG_USER_REG_ID = 4;

input 				clk;
input 				rst;
output reg [95:0] control;

wire tap_reset;
wire tap_idle;
wire tap_capture;
wire tap_update;
wire bscan_tck;
wire bscan_tdi;
wire bscan_tdo;
wire data_valid;
wire [7:0] cmd;
wire control_trigger;
reg [103:0] jtag_write_data;

initial control = 0;
initial jtag_write_data = 0;
assign cmd = jtag_write_data[103:96];
assign control_trigger = cmd[5];
assign bscan_tdo = 0;

jtag_phy #(.JTAG_USER_REG_ID(JTAG_USER_REG_ID)) 
jphy(.tap_reset(tap_reset),.tap_idle(tap_idle),.tap_capture(tap_capture),.tap_update(tap_update),.bscan_tck(bscan_tck),.bscan_tdi(bscan_tdi),.bscan_tdo(bscan_tdo),.data_valid(data_valid));

always @(posedge tap_idle) begin
control <= control_trigger ? jtag_write_data[95:0] : control;
end

always @(posedge bscan_tck) begin
    if (tap_idle) begin
        jtag_write_data <= 0;
	end else if (data_valid) begin
		jtag_write_data <= {bscan_tdi, jtag_write_data[103:1]};
	end
end
endmodule