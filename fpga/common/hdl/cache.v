`timescale 1ps/1ps

module cache (
	clk,
	rst,

	config_valid,
	config_word,
	config_addr,

	cpu_read,
	cpu_write,
	cpu_address,
	cpu_wrdata,
	cpu_wrstrb,
	cpu_rdvalid,
	cpu_rddata,
	cpu_rdaddress,
	cpu_ready,

	mem_read,
	mem_write,
	mem_address,
	mem_rddata,
	mem_rdvalid,
	mem_rdaddress,
	mem_wrdata,
	mem_ready,
	mem_wrstrb,

	state
	
);

	parameter CACHE_SIZE = 32; // in kb (kilobits)
	parameter SET_SIZE = 2; // should be a power of 2 (ie. 1, 2, 4, 8, 16 .... CACHE_SIZE)
	parameter ADDR_WIDTH = 32; // should be 32 or 64
	parameter CPU_DATA_WIDTH = 32; // should be 32 or 64
	parameter MEM_DATA_WIDTH = 128; // should be >= CPU_DATA_WIDTH
	parameter CACHE_POLICY_RESET = 1;
	parameter CONFIGURATION_DATA_BITS = 32;
	parameter CONFIGURATION_ADDR_BITS = 32;
	parameter CONFIGADDR_START_CACHE_POLICY = 0;
	parameter CONFIGADDR_END_CACHE_POLICY = 0;


	localparam integer BLOCK_SIZE = MEM_DATA_WIDTH;
	localparam integer TOTAL_SETS = (CACHE_SIZE << 10) / (SET_SIZE * BLOCK_SIZE);
	localparam integer CPUWORDS_PER_MEMWORD = MEM_DATA_WIDTH /  CPU_DATA_WIDTH;
	localparam integer BYTES_PER_CPUWORD = CPU_DATA_WIDTH/8;

	localparam OFFSET_BITS = $clog2(CPUWORDS_PER_MEMWORD);
	localparam INDEX_BITS = $clog2(TOTAL_SETS);
	localparam TAG_BITS = ADDR_WIDTH - $clog2(BYTES_PER_CPUWORD) - INDEX_BITS - OFFSET_BITS;
	localparam LRU_COUNTER_WIDTH = $clog2(SET_SIZE) + 1; // could be optimized to save 1 bit per counter

    localparam TAG_START_BIT = OFFSET_BITS+$clog2(BYTES_PER_CPUWORD)+INDEX_BITS;

	input clk;
	input rst;
	input config_valid;
	input [CONFIGURATION_DATA_BITS-1:0] config_word;
	input [CONFIGURATION_ADDR_BITS-1:0] config_addr;

	input cpu_read;
	input cpu_write;
	input [ADDR_WIDTH-1:0] cpu_address;
	input [CPU_DATA_WIDTH-1:0] cpu_wrdata;
	input [(CPU_DATA_WIDTH>>3)-1:0] cpu_wrstrb;
	output reg [CPU_DATA_WIDTH-1:0] cpu_rddata;
	output reg cpu_rdvalid;

	output 	   cpu_ready;
	output reg mem_read;
	output reg mem_write;
	output  [ADDR_WIDTH-1:0] mem_address;
	output  [MEM_DATA_WIDTH-1:0] mem_wrdata;
	input [MEM_DATA_WIDTH-1:0] mem_rddata;
	input mem_rdvalid;
	input mem_ready;


	output reg [7:0] state = 0;



	output cpu_rdaddress;
	input mem_rdaddress;
	output  [(MEM_DATA_WIDTH>>3)-1:0] mem_wrstrb;
	
	assign cpu_rdaddress = 0;
	assign mem_wrstrb = 0;
	
	

	reg [4:0] policy; // should be "WT" (Write Through) -> 1, "WB" (Write Back) -> 2 or "RO" (Read Only) -> 0 


	reg [(BLOCK_SIZE*SET_SIZE)-1:0] 		cache_ram 	[0:TOTAL_SETS-1];
	reg [(LRU_COUNTER_WIDTH*SET_SIZE)-1:0] 	lru_ram 	[0:TOTAL_SETS-1];
	reg [(TAG_BITS*SET_SIZE)-1:0] 			tag_ram 	[0:TOTAL_SETS-1];
	reg [(1*SET_SIZE)-1:0]					valid_ram 	[0:TOTAL_SETS-1];

	
	reg	write_enable_ram = 0;
	reg [(BLOCK_SIZE*SET_SIZE)-1:0] 			cache_ram_read_word;
	reg [(BLOCK_SIZE*SET_SIZE)-1:0] 			cache_ram_write_word;
	reg [(LRU_COUNTER_WIDTH*SET_SIZE)-1:0] 		lru_ram_read_word;
	reg [(LRU_COUNTER_WIDTH*SET_SIZE)-1:0] 		lru_ram_write_word;
	reg [(TAG_BITS*SET_SIZE)-1:0] 				tag_ram_read_word;
	reg [(TAG_BITS*SET_SIZE)-1:0] 				tag_ram_write_word;
	reg [(1*SET_SIZE)-1:0] 						valid_ram_read_word;
	reg [(1*SET_SIZE)-1:0] 						valid_ram_write_word;

	reg [SET_SIZE-1:0]	lru_block_select;
	reg [LRU_COUNTER_WIDTH-1:0]	lru_smallest_value;
	reg [BLOCK_SIZE-1:0] selected_cache_block;
	reg [BLOCK_SIZE-1:0] selected_lru_block;
	reg [SET_SIZE-1:0] tag_match;

	 


	integer i;
	integer ii;
	integer iii;
	integer iiii;
	integer iiiii;

	reg [$clog2(TOTAL_SETS):0] ram_reset_counter = 0;
	reg cpu_read_buff;
	reg cpu_write_buff;
	reg [ADDR_WIDTH-1:0] cpu_address_buff;
	reg [CPU_DATA_WIDTH-1:0] cpu_wrdata_buff;
	reg [CPU_DATA_WIDTH-1:0] cpu_wrstrb_buff;
	reg [MEM_DATA_WIDTH-1:0] cpu_wrdata_full_block;
	reg [MEM_DATA_WIDTH-1:0] mem_rddata_buff;

	reg cache_ram_write_block_sel; // 0 for cpu_wrdata_buff, 1 for mem_rddata_buff 
	reg mem_address_sel; // 0 for cpu_address or 1 for address reconstructed from tags 
	reg mem_wrdata_sel; // 0 for cpu_wrdata_buff, 1 for selected_cache_block
	reg [TAG_BITS-1:0] kicked_block_tag;
	wire [ADDR_WIDTH-1:0] kicked_block_address = {kicked_block_tag, cpu_address_buff[0 +: TAG_START_BIT]} ; //{cpu_address_buff[ADDR_WIDTH - 1:TEMP],tag_ram_read_word[0+: TAG_BITS],cpu_address_buff[TEMP2:0] };

	wire mem_busy;
	wire cpu_busy;

	assign cpu_busy = (state == 0) ? 0 : 1;

	assign mem_busy = !mem_ready;
	assign cpu_ready = !cpu_busy;

	assign mem_wrdata = (mem_wrdata_sel) ?  selected_lru_block  : cpu_wrdata_full_block;
	assign mem_address = (mem_address_sel) ? kicked_block_address :  cpu_address_buff;


    always @(posedge clk) begin
        if (rst) 
            policy <= CACHE_POLICY_RESET;
        else if (config_valid) begin
			if ((config_addr >= CONFIGADDR_START_CACHE_POLICY) && (config_addr <= CONFIGADDR_END_CACHE_POLICY))
				policy <= config_word[4:0];
	    end
    end


	always @(*) begin
		tag_match = 0;
		selected_cache_block = cache_ram_read_word[0+:BLOCK_SIZE];
		selected_lru_block = cache_ram_read_word[0+:BLOCK_SIZE];
		lru_ram_write_word  = lru_ram_read_word;
		kicked_block_tag = tag_ram_read_word[0 +: TAG_BITS];
		cache_ram_write_word = 	cache_ram_read_word;
		tag_ram_write_word = 	tag_ram_read_word;
		valid_ram_write_word = 	valid_ram_read_word;
		lru_block_select = 1;
	    lru_smallest_value = lru_ram_read_word[0+:LRU_COUNTER_WIDTH];
		
		for (i=1;i<SET_SIZE;i=i+1) begin
			if ( (lru_ram_read_word[(i*LRU_COUNTER_WIDTH)+:LRU_COUNTER_WIDTH]) < lru_smallest_value ) begin
				lru_block_select = (1 << i);
				lru_smallest_value = lru_ram_read_word[(i*LRU_COUNTER_WIDTH)+:LRU_COUNTER_WIDTH];
			end
		end
		
		
		for (i=0;i<SET_SIZE;i=i+1) begin
			// Any tag matches in the current selected set of blocks (or default to 0 if no match)
			if (tag_ram_read_word[i*TAG_BITS +: TAG_BITS] == cpu_address_buff[(OFFSET_BITS+$clog2(BYTES_PER_CPUWORD)+ INDEX_BITS) +: TAG_BITS])
				tag_match[i] = 1'b1;				
		end
		
		
		for (i=0;i<SET_SIZE;i=i+1) begin
			// Get the selected cache block from the selected set
			if (tag_match[i] == 1'b1)
				selected_cache_block = cache_ram_read_word[i*BLOCK_SIZE +: BLOCK_SIZE];		
		end
		
		
		for (i=0;i<SET_SIZE;i=i+1) begin
			// Get the selected lru block from the selected set
			if (lru_block_select[i] == 1'b1)
				selected_lru_block = cache_ram_read_word[i*BLOCK_SIZE +: BLOCK_SIZE];		
		end
		
		
		for (i=0;i<SET_SIZE;i=i+1) begin
			if (lru_block_select[i] && (tag_match==0))
				lru_ram_write_word[i*LRU_COUNTER_WIDTH+:LRU_COUNTER_WIDTH] = SET_SIZE;
			else if ((tag_match==0) &&  (lru_ram_read_word[i*LRU_COUNTER_WIDTH+:LRU_COUNTER_WIDTH] > 0))
				lru_ram_write_word[i*LRU_COUNTER_WIDTH+:LRU_COUNTER_WIDTH] = lru_ram_read_word[i*LRU_COUNTER_WIDTH+:LRU_COUNTER_WIDTH] - 1;		
		end
		
		
		for (i=0;i<SET_SIZE;i=i+1) begin
			if (lru_block_select[i])
				kicked_block_tag = tag_ram_read_word[i*TAG_BITS +: TAG_BITS];		
		end
		
		
		for (i=0;i<SET_SIZE;i=i+1) begin
			if (tag_match[i] || (lru_block_select[i] && (tag_match==0)))
				valid_ram_write_word[i] = 1;		
		end
		
		 
		cpu_rddata = selected_cache_block[0+:CPU_DATA_WIDTH];
		cpu_wrdata_full_block = selected_cache_block;
		
		for (ii=0;ii<CPUWORDS_PER_MEMWORD;ii=ii+1) begin
			if (ii == cpu_address_buff[$clog2(BYTES_PER_CPUWORD) +: OFFSET_BITS])
				cpu_rddata = selected_cache_block[ii*CPU_DATA_WIDTH +: CPU_DATA_WIDTH];

			for (iii=0; iii < CPU_DATA_WIDTH; iii=iii+1) begin
			     if (ii == cpu_address_buff[$clog2(BYTES_PER_CPUWORD) +: OFFSET_BITS]) begin
			        if (cpu_wrstrb_buff[iii])
					   cpu_wrdata_full_block[ii*CPU_DATA_WIDTH + iii] = cpu_wrdata_buff[iii];
				end
			end
		end
		
		
		for (i=0;i<SET_SIZE;i=i+1) begin
			if (tag_match[i] || (lru_block_select[i] && (tag_match==0)))
				cache_ram_write_word[i*BLOCK_SIZE +: BLOCK_SIZE] = cache_ram_write_block_sel ? mem_rddata_buff : cpu_wrdata_full_block;		
		end
		
		
		for (i=0;i<SET_SIZE;i=i+1) begin
			if (tag_match[i] || (lru_block_select[i] && (tag_match==0)))
				tag_ram_write_word[i*TAG_BITS +: TAG_BITS] =  cpu_address_buff[(OFFSET_BITS+$clog2(BYTES_PER_CPUWORD)+ INDEX_BITS) +: TAG_BITS];

		end
	
	end

	



	always @(posedge clk) begin
		cache_ram_read_word <=  cache_ram[cpu_address_buff[(OFFSET_BITS+$clog2(BYTES_PER_CPUWORD)) +: INDEX_BITS]];
		lru_ram_read_word   <=  lru_ram  [cpu_address_buff[(OFFSET_BITS+$clog2(BYTES_PER_CPUWORD)) +: INDEX_BITS]];
		tag_ram_read_word   <= 	tag_ram  [cpu_address_buff[(OFFSET_BITS+$clog2(BYTES_PER_CPUWORD)) +: INDEX_BITS]];
		valid_ram_read_word <=  valid_ram[cpu_address_buff[(OFFSET_BITS+$clog2(BYTES_PER_CPUWORD)) +: INDEX_BITS]];

		if (mem_rdvalid)	// buffer data input from memory if read valid signal is high
			mem_rddata_buff <= mem_rddata;
		
		if (state == 0) begin 	// keep buffering inputs till a valid signal moves the system out of idle state
				cpu_address_buff <= cpu_address;
				cpu_read_buff <= cpu_read;
				cpu_write_buff <= cpu_write;
				cpu_wrdata_buff <= cpu_wrdata;
				for (iiiii=0; iiiii < (CPU_DATA_WIDTH >> 3);iiiii = iiiii + 1) begin 
				    cpu_wrstrb_buff[iiiii*8 +: 8] = {8{cpu_wrstrb[iiiii]}};
				end
		end


		if (rst) begin
			valid_ram[ram_reset_counter] <= 0; 
			lru_ram[ram_reset_counter] <= 0; 
			ram_reset_counter <= ram_reset_counter + 1;
		end else if (write_enable_ram) begin			
			cache_ram[cpu_address_buff[(OFFSET_BITS+$clog2(BYTES_PER_CPUWORD)) +: INDEX_BITS]]  <=  cache_ram_write_word;
			lru_ram  [cpu_address_buff[(OFFSET_BITS+$clog2(BYTES_PER_CPUWORD)) +: INDEX_BITS]]  <=  lru_ram_write_word;
			tag_ram  [cpu_address_buff[(OFFSET_BITS+$clog2(BYTES_PER_CPUWORD)) +: INDEX_BITS]]  <=  tag_ram_write_word;
			valid_ram[cpu_address_buff[(OFFSET_BITS+$clog2(BYTES_PER_CPUWORD)) +: INDEX_BITS]]  <=  valid_ram_write_word;
		end


		if (rst) begin
			cpu_rdvalid <= 0;
			mem_read <= 0;
			mem_write <= 0;
			write_enable_ram <= 0;
			cache_ram_write_block_sel <= 0;
			mem_address_sel <= 0;
			mem_wrdata_sel <= 0;
			state <= 0;	

		end else if (state == 0) begin
			cpu_rdvalid <= 0;
			mem_read <= 0;
			mem_write <= 0;
			write_enable_ram <= 0;
			cache_ram_write_block_sel <= 0;
			mem_address_sel <= 0;
			mem_wrdata_sel <= 0;
			if (cpu_read || (cpu_write && (policy != 0)))
				state <= 8'd1;	

		end else if (state == 8'd1) begin
			if (cpu_read_buff)
				state <= 8'd2;
			else if (cpu_write_buff && (policy != 0) && (policy != 5'd3)) 
				state <= 8'd10;
			else 
				state <= 0;	

		end else if (state == 8'd2) begin
			if (tag_match & valid_ram_read_word) begin          // if tag matches and data is valid, return it. nothing else to do
				cpu_rdvalid <= 1'b1;
				state <= 0;
			end else if (!(lru_block_select & valid_ram_read_word)) begin 			// if not a valid word, return 										
				state <= 8'd7;
			    mem_address_sel <= 1'b0;
				mem_read <= 1'b1;		
			end else if ((lru_block_select & valid_ram_read_word) && (policy == 5'd2)) begin // if tag doesn't match, and we have a valid data in the cache and policy is WB, we must write this data to mem before reading. 
				mem_address_sel <= 1'b1;
				mem_wrdata_sel <= 1'b1;
				mem_write <= 1'b1;
				state <= 8'd4;
			end else begin
				state <= 8'd7;
			    mem_address_sel <= 1'b0;
				mem_read <= 1'b1;
			end
			
			
		end else if (state == 8'd4) begin
		    if (mem_busy == 0) begin
			     mem_write <= 1'b0;								
				 state <= 8'd5;
			end

		end else if (state == 8'd5) begin		
			if (mem_busy == 0) begin								
				state <= 8'd7;
			    mem_address_sel <= 1'b0;
				mem_read <= 1'b1;
			end

		end else if (state == 8'd7) begin
		    if (mem_busy == 0)
			    mem_read <= 1'b0;		
			if (mem_rdvalid == 1'b1) begin								
				state <= 8'd8;
				cache_ram_write_block_sel <= 1;
			end

		end else if (state == 8'd8) begin		
			write_enable_ram <= 1;
			state <= 8'd9;
			

		end else if (state == 8'd9) begin		
			write_enable_ram <= 0;
			state <= 8'd1;
			

		end else if (state == 8'd10) begin
			if (tag_match) begin
			     state <= 8'd17;
			end else if ((lru_block_select & valid_ram_read_word) && (policy == 5'd2)) begin  
				state <= 8'd11;
				mem_address_sel <= 1'b1;
			    mem_wrdata_sel <= 1'b1; 
				mem_write <= 1'b1;	
			end else begin
			    mem_address_sel <= 1'b0;
			    mem_read <= 1'b1;								
				state <= 8'd14;
			end

		end else if (state == 8'd11) begin
		    if (mem_busy == 0) begin
			    mem_write <= 1'b0;
			    mem_address_sel <= 1'b0;
			    mem_read <= 1'b1;								
				state <= 8'd14;
			end
			 
        
 	
 		end else if (state == 8'd14) begin
 		    if (mem_busy == 1'b0)
			     mem_read <= 1'b0;		
			if (mem_rdvalid == 1'b1) begin								
				state <= 8'd15;
				cache_ram_write_block_sel <= 1;
			end

		end else if (state == 8'd15) begin		
			write_enable_ram <= 1;
			state <= 8'd16;
			
		end else if (state == 8'd16) begin		
			write_enable_ram <= 0;
			state <= 8'd17;
	
 
		end else if (state == 8'd17) begin		
			write_enable_ram <= 0;
			if (policy == 5'd2) begin
				state <= 8'd0;
				write_enable_ram <= 1;
				cache_ram_write_block_sel <= 0;
			end else begin	
				write_enable_ram <= 1;
				cache_ram_write_block_sel <= 0;							
				state <=   8'd18 ;
				mem_write <=  1'b1;
				mem_address_sel <= 1'b0;
				mem_wrdata_sel <= 1'b0;
			end

		end else if (state == 8'd18) begin
			write_enable_ram <= 0;
			if (mem_busy == 1'b0) begin	
			    mem_write <= 1'b0;									
				state <= 8'd0;
			end

		end else begin 
			state <= 0;

		end
		
	end
endmodule


module cache_line_builder (
	clk,
	rst,

	host_read,
	host_write,
	host_address,
	host_rddata,
	host_wrdata,
	host_wrstrb,
	host_rdaddress,
	host_rdvalid,
	host_ready,

	device_read,
	device_write,
	device_address,
	device_rddata,
	device_rdvalid,
	device_wrdata,
	device_ready,
	device_wrstrb,
	device_rdaddress,
	
	state
	
);


	parameter ADDR_WIDTH = 32; 
	parameter DEVICE_DATA_WIDTH = 128;  
	parameter CACHE_LINE_SIZE = 128; 
    parameter DEVICE_DATA_BUS_WIDTH = 16;

	
    localparam integer BURST_SIZE = DEVICE_DATA_WIDTH/DEVICE_DATA_BUS_WIDTH;
	localparam integer NUM_TRASACTIONS = CACHE_LINE_SIZE/DEVICE_DATA_WIDTH;
	localparam integer OFFSET_BITS = $clog2(CACHE_LINE_SIZE >> 3);
	

	input clk;
	input rst;

	input host_read;
	input host_write;
	input [ADDR_WIDTH-1:0] host_address;
	input [CACHE_LINE_SIZE-1:0] host_wrdata;
	output reg [CACHE_LINE_SIZE-1:0] host_rddata;
	output reg host_rdvalid;
	output reg host_ready;

	output reg device_read;
	output reg device_write;
	output reg  [ADDR_WIDTH-1:0] device_address;
	output reg  [DEVICE_DATA_WIDTH-1:0] device_wrdata;
	input [DEVICE_DATA_WIDTH-1:0] device_rddata;
	input device_rdvalid;
	input device_ready;

	output reg [7:0] state;
	
	
	
	input [(CACHE_LINE_SIZE>>3)-1:0] host_wrstrb;
	output [ADDR_WIDTH-1:0] host_rdaddress;
	output [(DEVICE_DATA_WIDTH>>3)-1:0] device_wrstrb;
	input [ADDR_WIDTH-1:0] device_rdaddress;
	
	assign device_wrstrb = 0;
	assign host_rdaddress = 0;
	

	wire [ADDR_WIDTH-1:0] address_map = (host_address >> OFFSET_BITS) << (OFFSET_BITS - ((DEVICE_DATA_BUS_WIDTH >> 3) - 1));

	reg [31:0] rd_valid_counter;
	reg [31:0] transaction_counter;

	wire host_busy;
	wire device_busy;


	reg [CACHE_LINE_SIZE-DEVICE_DATA_WIDTH-1:0] host_wrdata_buff;

	initial state = 0;


	generate 

		if (NUM_TRASACTIONS <= 1) begin
			always @(*) begin
				device_read = host_read;
				device_write = host_write;
				device_address = address_map;
				device_wrdata = host_wrdata;
				host_rddata = device_rddata;
				host_rdvalid = device_rdvalid;
				host_ready = device_ready;
			end
		end else begin
			assign device_busy = !device_ready;
		    always @(*) begin
			     host_ready = (state == 0) ? 1'b1 : 1'b0;
			end
			always @(posedge clk) begin
				if (state == 0) begin
					rd_valid_counter <= 32'd0;
				end else if (device_rdvalid)  begin
					rd_valid_counter <= rd_valid_counter + 32'd1;
					host_rddata <= {device_rddata,host_rddata[DEVICE_DATA_WIDTH+:(CACHE_LINE_SIZE-DEVICE_DATA_WIDTH)]};
				end

				if (rst) begin
					device_read <= 0;
					device_write <= 0;
					device_address <= address_map;
					device_wrdata <= 0;
					host_rddata <= 0;
					host_rdvalid <= 0;
					state <= 0;
					transaction_counter <= 32'd1;
					host_wrdata_buff <= 0;
				end else if (state == 0) begin
					device_read <= 0;
					device_write <= 0;
					device_address <= address_map;
					host_wrdata_buff <= host_wrdata[CACHE_LINE_SIZE-1:DEVICE_DATA_WIDTH];
					host_rddata <= 0;
					host_rdvalid <= 0;
					transaction_counter <= 32'd1;
					device_wrdata <= 0;
					if (host_read == 1'b1) begin 
						state <= 8'b1;
						device_read <= 1'b1;
					end else if (host_write == 1'b1) begin
						state <= 8'd3;
						device_write <= 1'b1;
						device_wrdata <= host_wrdata[DEVICE_DATA_WIDTH-1:0];
					end

				end else if (state == 8'd1) begin
					if (device_busy == 0) begin
						transaction_counter <= transaction_counter + 32'd1;
						device_address <= device_address + (1 << $clog2(BURST_SIZE));
						if (transaction_counter == NUM_TRASACTIONS) begin
							device_read <= 1'b0;
							state <= 8'd2;
						end 
					end

				end else if (state == 8'd2) begin
					if (rd_valid_counter == NUM_TRASACTIONS) begin
							host_rdvalid <= 1'b1;
							state <= 8'd0;
					end
					
				end else if (state == 8'd3) begin
					if (device_busy == 0) begin
						transaction_counter <= transaction_counter + 32'd1;
						device_address <= device_address + (1 << $clog2(BURST_SIZE));
						host_wrdata_buff <=  host_wrdata_buff >> DEVICE_DATA_WIDTH;
						device_wrdata <= host_wrdata_buff[DEVICE_DATA_WIDTH-1:0];
						if (transaction_counter == NUM_TRASACTIONS) begin
							device_write <= 1'b0;
							state <= 8'd0;
						end 
					end
				end

			end
		end

	endgenerate

endmodule


module bram (
	clk,
	rst,

	cpu_read,
	cpu_write,
	cpu_address,
	cpu_wrdata,
	cpu_wrstrb,
	cpu_rdvalid,
	cpu_rddata,
	cpu_rdaddress,
	cpu_ready
);
	parameter MEMORY_SIZE = 256; // in kb (kilobits)
	parameter ADDR_WIDTH = 32;
	parameter DATA_WIDTH = 32; 
	localparam integer MEM_ARRAYS = DATA_WIDTH >> 3; // 4
	localparam integer LOG_MEM_ARRAYS = $clog2(MEM_ARRAYS); // 2
	
	localparam integer DEPTH = (MEMORY_SIZE << 7) / (MEM_ARRAYS); // 32768 / 4 = 8192
	
	input clk;
	input rst;
	input cpu_read;
	input cpu_write;
	input [ADDR_WIDTH-1:0] cpu_address;
	output [ADDR_WIDTH-1:0] cpu_rdaddress;
	input [DATA_WIDTH-1:0] cpu_wrdata;
	input [(DATA_WIDTH>>3)-1:0] cpu_wrstrb;
	output  [DATA_WIDTH-1:0] cpu_rddata;
	output  cpu_rdvalid;
	output 	   cpu_ready;
	
	wire [MEM_ARRAYS-1:0] cpu_rdvalid_array;
	wire [MEM_ARRAYS-1:0] cpu_ready_array;
	
	assign cpu_rdaddress = cpu_address;
	assign cpu_rdvalid  = |cpu_rdvalid_array;
	assign cpu_ready  = |cpu_ready_array;
	
	genvar i;
	generate
	for (i=0; i < MEM_ARRAYS; i=i+1) begin
		bram_single_array #(.DEPTH(DEPTH)) bsa(
		.clk(clk),
		.rst(rst),
		.cpu_read(cpu_read),
		.cpu_write(cpu_write),
		.cpu_address(cpu_address >> LOG_MEM_ARRAYS), 
		.cpu_wrdata(cpu_wrdata[i*8+:8]),
		.cpu_wrstrb(cpu_wrstrb[i]),
		.cpu_rdvalid(cpu_rdvalid_array[i]),
		.cpu_rddata(cpu_rddata[i*8+:8]),
		.cpu_ready(cpu_ready_array[i])
		);
	end
	endgenerate
endmodule

module bram_single_array (
	clk,
	rst,
	cpu_read,
	cpu_write,
	cpu_address,
	cpu_rdaddress,
	cpu_wrdata,
	cpu_wrstrb,
	cpu_rdvalid,
	cpu_rddata,
	cpu_ready
);
	parameter DEPTH = 32;
	localparam integer  ADDR_WIDTH = $clog2(DEPTH);
	input clk;
	input rst;
	input cpu_read;
	input cpu_write;
	input [ADDR_WIDTH-1:0] cpu_address;
	output [ADDR_WIDTH-1:0] cpu_rdaddress;
	input [7:0] cpu_wrdata;
	input cpu_wrstrb;
	output reg [7:0] cpu_rddata;
	output reg cpu_rdvalid;
	output 	   cpu_ready;
	
	reg [7:0] mem [0:DEPTH - 1];
	
	assign cpu_rdaddress = cpu_address;
	assign cpu_ready = !rst;
	always @(posedge clk) begin
		cpu_rddata <= mem[cpu_address];
		if (rst) begin
			cpu_rdvalid <= 0;
		end else begin
			cpu_rdvalid <= cpu_read;
			if (cpu_write && cpu_wrstrb)
				mem[cpu_address] <= cpu_wrdata;
		end
	end
endmodule
