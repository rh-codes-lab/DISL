module SimpleMux(
	sel,
	out,
	in
	);
parameter NUM_INPUTS = 4;
parameter DATA_SIZE = 8;

localparam SEL = $clog(NUM_INPUTS);

input [SEL-1:0] sel;
input [(NUM_INPUTS*DATA_SIZE)-1:0] in;
output reg [DATA_SIZE-1:0] out;
integer idx;
always @(*) begin
	out = in[0+:DATA_SIZE];
	for (idx=1;idx<NUM_INPUTS;idx=idx+1) begin
		if (sel == idx) 
			out = in[(idx*DATA_SIZE)+:DATA_SIZE];
	end
end
endmodule


module OHEMux(
	clk,
	rst,
	sel,
	in,
	out
	);
parameter REGISTERED = 0;
parameter NUM_INPUTS = 4;
parameter DATA_SIZE = 8;
input clk;
input rst;
input [NUM_INPUTS-1:0] sel;
input [(NUM_INPUTS*DATA_SIZE)-1:0] in;
output reg [DATA_SIZE-1:0] out;
reg [DATA_SIZE-1:0] out_int;
integer i;
always @(*) begin
	out_int = in[0+:DATA_SIZE];
	for (i=1;i<NUM_INPUTS;i=i+1) begin
		if (sel[i]) 
			out_int = in[(i*DATA_SIZE)+:DATA_SIZE];
	end
end
generate 
	if (REGISTERED) begin
		always @(posedge clk) begin
			if (rst)
				out <= 0;
			else
				out <= out_int;	
			
		end
	end else begin
		always @(*)
			out = out_int;
	end
endgenerate
endmodule


module OHEDemux(
	sel,
	in,
	out
	);
parameter NUM_OUTPUTS = 4;
parameter DATA_SIZE = 1;
input [NUM_OUTPUTS-1:0] sel;
input [DATA_SIZE-1:0] in;
output reg [(NUM_OUTPUTS*DATA_SIZE)-1:0] out;
integer i;
always @(*) begin
	out = 0;
	for (i=0;i<NUM_OUTPUTS;i=i+1) begin
		if (sel[i]) 
			out[(i*DATA_SIZE)+:DATA_SIZE]= in;
	end
end
endmodule



/////////////////////////////////////////////////////

module tb_OHEDemux;

reg [7:0] sel;
wire [3:0] out;


reg clk;
reg rst;

initial begin
	clk = 0;
	rst = 1;
	sel = 0;
	#100;
	rst = 0;
	#100;
	sel= 1;
end

always #5 clk = ~clk;

always #500 sel = {sel[6:0],sel[7]};

OHEDemux 
#(.NUM_OUTPUTS(8), .DATA_SIZE(8))
uut(
	.clk(clk),
	.rst(rst),

	.sel(sel),
	.in(10),
	.out(out)
	);
endmodule



module tb_OHEMUX;
reg [3:0] sel_reg;
wire [7:0] out_reg;


reg [6:0] sel_noreg;
wire [4:0] out_noreg;


reg clk;
reg rst;

initial begin
	clk = 0;
	rst = 1;
	sel_reg = 0;
	sel_noreg = 0;
	#100;
	rst = 0;
	#100;
	sel_reg = 1;
	sel_noreg = 64;
end

always #5 clk = ~clk;

always #500 sel_noreg = {sel_noreg[5:0],sel_noreg[6]};
always #700 sel_reg = {sel_reg[0],sel_reg[3:1]};

OHEMux 
#(.REGISTERED(1), .NUM_INPUTS(4), .DATA_SIZE(8))
uut_reg(
	.clk(clk),
	.rst(rst),

	.sel(sel_reg),
	.in({8'd33,8'd1,8'd123,8'd255}),
	.out(out_reg)
	);


OHEMux 
#(.REGISTERED(0), .NUM_INPUTS(7), .DATA_SIZE(5))
uut_noreg(
	.clk(clk),
	.rst(rst),

	.sel(sel_noreg),
	.in({5'd7,5'd6,5'd5,5'd4,5'd3,5'd2,5'd1}),
	.out(out_noreg)
	);






endmodule





