module ring_buffer (
clk,
rst,
data_in_data,
data_in_valid,
data_in_ready,
data_out_data,
data_out_ready,
data_out_valid);

parameter DATA_WIDTH = 8;
parameter BUFFER_SIZE = 32;

localparam BUFFER_LOG_SIZE = $clog2(BUFFER_SIZE);

input 				clk;
input 				rst;
input [DATA_WIDTH-1:0] 		data_in_data;
input 				data_in_valid;
output 				data_in_ready;
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


always @(negedge clk) begin
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
