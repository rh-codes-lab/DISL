module jtag_phy(
	output tap_reset,
	output tap_idle,
	output tap_capture,
	output tap_update,
	output bscan_tck,
	output bscan_tdi,
	input bscan_tdo,
	output data_valid
);
parameter JTAG_USER_REG_ID = 4;
wire shift;
wire sel;
assign data_valid = shift&sel;

BSCANE2 #(
    .JTAG_CHAIN(JTAG_USER_REG_ID)
)
bse2_inst (
    .CAPTURE(tap_capture),
    .DRCK(),
    .RESET(tap_reset),
    .RUNTEST(tap_idle),
    .SEL(sel),
    .SHIFT(shift),
    .TCK(bscan_tck),
    .TDI(bscan_tdi),
    .TMS(bascan_tms),
    .UPDATE(tap_update),
    .TDO(bscan_tdo)
);
endmodule
