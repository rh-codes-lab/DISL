module jpeg_reorder_buffer(
input clk,
input rst,
input ivalid,
input [7:0] ipixel,
output  [7:0] opixel,
output reg [31:0] pixel_id,
output reg ovalid
);

parameter IMAGE_W = 320;
parameter IMAGE_H = 240;
localparam BUFFER_SIZE = IMAGE_W * 8;
localparam PIXELS = IMAGE_W * IMAGE_H;
localparam BLOCKS_PER_ROW = IMAGE_W >> 3;

reg [7:0] mem_1 [0:(BUFFER_SIZE)-1];
reg [7:0] mem_2 [0:(BUFFER_SIZE)-1];
reg [7:0] mem_1_rdata;
reg [7:0] mem_2_rdata;
reg [15:0] write_pointer;
reg [15:0] x;
reg [15:0] y;
reg [15:0] block;
reg start;
reg wrbuf;
reg [31:0] prev_pixel_id;

wire rbuf = !wrbuf;
wire wen_1 = ivalid && !wrbuf;
wire wen_2 = ivalid && wrbuf;
wire [15:0] read_pointer = (block<<6) + (y << 3) + x;
assign opixel = (rbuf) ? mem_2_rdata : mem_1_rdata;

wire reset = (wen_1 || wen_2) && ((write_pointer+1) == BUFFER_SIZE);
wire pause = ((y == 7) && (x == 7) && (block == (BLOCKS_PER_ROW-1)))  ? 1'b1 : 1'b0;


always @(posedge clk) begin
    mem_1_rdata <= mem_1[read_pointer];
    mem_2_rdata <= mem_2[read_pointer];
    if (wen_1)
        mem_1[write_pointer] <= ipixel;
    if (wen_2)
        mem_2[write_pointer] <= ipixel;
end




always @(posedge clk) begin
    if (rst) begin
        wrbuf <= 0;
        x <= 0;
        y <= 0;
        block <= 0;
        pixel_id <= 0;
        prev_pixel_id <= 0; 
        ovalid <= 0;
        write_pointer <= 0;
        start <= 0;

    end else if (reset) begin
        x <= 0;
        y <= 0;
        block <= 0;
        write_pointer <= 0;
        wrbuf <= ~wrbuf;   
        pixel_id <= pixel_id + 1;
        prev_pixel_id <= prev_pixel_id + 1;
        ovalid <= 1;

    end else if (pause) begin
        x <= x;
        y <= y;
        block <= block;
        pixel_id <= pixel_id;
        prev_pixel_id <= (prev_pixel_id < pixel_id) ? prev_pixel_id + 1 : prev_pixel_id;
        write_pointer <= (wen_1 || wen_2) ? write_pointer + 1 : write_pointer;
        wrbuf <= wrbuf; 
        ovalid <= 0;
        start <= 1;

    end else begin
        ovalid <= start;
        write_pointer <= (wen_1 || wen_2) ? write_pointer + 1 : write_pointer;
        wrbuf <= wrbuf;  
        pixel_id <= pixel_id + 1;
        prev_pixel_id <= (prev_pixel_id < pixel_id) ? prev_pixel_id + 1 : prev_pixel_id;
        if (x < 7) begin
            x <= x + 1;
        end else begin
            x <= 0;
            if (block < BLOCKS_PER_ROW-1) begin
                block <= block + 1;
            end else begin
                block <= 0;
                y <= y + 1;
            end
        end
    end
end
endmodule