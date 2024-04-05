`timescale 1ps/1ps

module ddr_phy(
    // CLOCKS AND RESETS
    input clk_in,
    input rst_in,
    output clk_out,
    output rst_out,
    // DATA PATH
    input [3:0]                     cmd_ras_n,
    input [3:0]                     cmd_cas_n,
    input [3:0]                     cmd_we_n,
    input [3:0]                     cmd_cs_n,
    input [1:0]                     cmd_odt,
    input [2:0]                     cmd_cmd,
    input [3:0]                     cmd_aux_out0,
    input [3:0]                     cmd_aux_out1,
    input [1:0]                     cmd_rank_cnt,
    input                           cmd_idle,
    input [5:0]                     cmd_data_offset,
    input [5:0]                     cmd_data_offset_1,
    input [5:0]                     cmd_data_offset_2,
    input                           cmd_wrdata_en,
    input [55:0]                    cmd_address,
    input [11:0]                    cmd_bank,
    input [15:0]                    cmd_wr_mask,
    input [127:0]                   cmd_wr_data,

    output [31:0]                   debug,

    output  [127:0]         ddr_rd_data,
    output                  ddr_rd_valid,
    output  [31:0]                status_reg,
    // DDR SIGNALS
    output ddr3_reset_n, output [0:0] ddr3_cke, output [0:0] ddr3_ck_p, output [0:0]  ddr3_ck_n,
    output [0:0] ddr3_cs_n, output ddr3_ras_n, output ddr3_cas_n, output ddr3_we_n,
    output [2:0] ddr3_ba, output [13:0] ddr3_addr, output [0:0] ddr3_odt, output [1:0] ddr3_dm,
    inout [1:0] ddr3_dqs_p, inout [1:0] ddr3_dqs_n, inout [15:0] ddr3_dq
    );



//////////////////////////////////  Read/Write/Status Assignment  /////////////////////////////////////////////
    assign status_reg = {5'd0,phy_mc_data_full,phy_mc_cmd_full,phy_mc_ctl_full,2'd0,calib_rd_data_offset_2,2'd0,calib_rd_data_offset_1,2'd0,calib_rd_data_offset_0};
    assign ddr_rd_data = phy_rd_data;
    assign ddr_rd_valid = phy_rddata_valid;
    wire [15:0]              mc_wrdata_mask = cmd_wr_mask;
    wire [127:0]             mc_wrdata = cmd_wr_data;


//////////////////////////////////  Command to Port Mapping  /////////////////////////////////////////////
   wire [3:0]               mc_ras_n = cmd_ras_n;
   wire [3:0]               mc_cas_n = cmd_cas_n;
   wire [3:0]               mc_we_n = cmd_we_n;
   wire [3:0]               mc_cs_n = cmd_cs_n;
   wire [1:0]               mc_odt = cmd_odt; 
   wire [2:0]               mc_cmd = cmd_cmd;
   wire [3:0]               mc_aux_out0 = cmd_aux_out0;
   wire [3:0]               mc_aux_out1 = cmd_aux_out1;
   wire [1:0]               mc_rank_cnt = cmd_rank_cnt;
   wire                     idle = cmd_idle;
   wire [5:0]               mc_data_offset = cmd_data_offset;
   wire [5:0]               mc_data_offset_1 = cmd_data_offset_1;
   wire [5:0]               mc_data_offset_2 = cmd_data_offset_2;
   wire [11:0]              mc_bank = cmd_bank;
   wire                     mc_wrdata_en = cmd_wrdata_en;
   wire [55:0]              mc_address = cmd_address;

//////////////////////////////// Clocking and Resets ////////////////////////////////////////////
    wire ppl_0_locked;
    wire ddr_sys_clk;   
    wire clk_ref_in;
    wire clk_ref;

    ip_clk_wiz_0 pll(
        .clk_out1(ddr_sys_clk),
        .clk_out2(),
        .clk_out3(clk_ref_in),
        .locked(ppl_0_locked),
        .clk_in1(clk_in)
    );

    wire clk;
    wire clk_div2;
    wire freq_refclk;
    wire mem_refclk;
    wire mmcm_ps_clk;
    wire poc_sample_pd;
    wire sync_pulse;
    wire rst;
    wire rst_div2;
    wire iddr_rst;
    wire rst_phaser_ref;
    wire pll_locked;
    wire [1:0] iodelay_ctrl_rdy;
    wire sys_rst_o;
    wire mmcm_clk;
    assign clk_out = clk;

    mig_7series_v4_2_iodelay_ctrl #
    (
     .TCQ                       (32'd100),
     .IODELAY_GRP0              ("MIG_7SERIES_0_IODELAY_MIG0"),
     .IODELAY_GRP1              ("MIG_7SERIES_0_IODELAY_MIG1"),
     .REFCLK_TYPE               ("NO_BUFFER"),
     .SYSCLK_TYPE               ("NO_BUFFER"),
     .SYS_RST_PORT              ("FALSE"),
     .RST_ACT_LOW               (1),
     .DIFF_TERM_REFCLK          ("TRUE"),
     .FPGA_SPEED_GRADE          (1),
     .REF_CLK_MMCM_IODELAY_CTRL ("FALSE")
     )
    u_iodelay_ctrl
      (
       // Outputs
       .iodelay_ctrl_rdy (iodelay_ctrl_rdy),
       .sys_rst_o        (sys_rst_o),
       .clk_ref          (clk_ref),
       // Inputs
       .clk_ref_p        (0),
       .clk_ref_n        (0),
       .clk_ref_i        (clk_ref_in),
       .sys_rst          (!rst_in)
       );

  mig_7series_v4_2_clk_ibuf #
    (
     .SYSCLK_TYPE      ("NO_BUFFER"),
     .DIFF_TERM_SYSCLK ("TRUE")
     )
    u_ddr3_clk_ibuf
      (
       .sys_clk_p        (0),
       .sys_clk_n        (0),
       .sys_clk_i        (ddr_sys_clk),
       .mmcm_clk         (mmcm_clk)
       );


    
    mig_7series_v4_2_infrastructure #
    (
     .TCQ                (32'd100),
     .nCK_PER_CLK        (32'd4),
     .CLKIN_PERIOD       (32'd3000),
     .SYSCLK_TYPE        ("NO_BUFFER"),
     .CLKFBOUT_MULT      (32'd4),
     .DIVCLK_DIVIDE      (32'd1),
     .CLKOUT0_PHASE      (0.0),
     .CLKOUT0_DIVIDE     (32'd2),
     .CLKOUT1_DIVIDE     (32'd4),
     .CLKOUT2_DIVIDE     (32'd64),
     .CLKOUT3_DIVIDE     (32'd16),
     .MMCM_VCO           (32'd666),
     .MMCM_MULT_F        (32'd8),
     .MMCM_DIVCLK_DIVIDE (32'd1),
     .RST_ACT_LOW        (32'd1),
     .tCK                (32'd3000),
     .MEM_TYPE           ("DDR3")
     )
    u_ddr3_infrastructure
      (
       .rstdiv0          (rst),
       .clk              (clk),
       .clk_div2         (clk_div2),
       .rst_div2         (rst_div2),
       .mem_refclk       (mem_refclk),
       .freq_refclk      (freq_refclk),
       .sync_pulse       (sync_pulse),
       .mmcm_ps_clk      (mmcm_ps_clk),
       .poc_sample_pd    (poc_sample_pd),
       .iddr_rst         (iddr_rst),
       .pll_locked       (pll_locked),
       .rst_phaser_ref   (rst_phaser_ref),
       .psen             (0),
       .psincdec         (0),
       .mmcm_clk         (mmcm_clk),
       .sys_rst          (sys_rst_o),
       .iodelay_ctrl_rdy (iodelay_ctrl_rdy),
       .ref_dll_lock     (ppl_0_locked)
       );


    // Turn of temperature monitoring
    wire tempmon_sample_en;
    assign tempmon_sample_en = 0;
 
///////////////////////////////////////// Reset /////////////////////////////////////////////////////////////
    wire   init_calib_complete;
    assign rst_out = ~init_calib_complete; // hold system in reset till calib complete. Will save resources. 

///////////////////////////////////////  Outputs ////////////////////////////////////////////////////////////
    wire  rst_tg_mc, calib_tap_req, psen, psincdec;
    wire [255:0]                      dbg_calib_top;
    wire [6*2*1-1:0]                  dbg_cpt_first_edge_cnt;
    wire [6*2*1-1:0]                  dbg_cpt_second_edge_cnt;
    wire [6*2*1-1:0]                  dbg_cpt_tap_cnt;
    wire [5*2*1-1:0]                  dbg_dq_idelay_tap_cnt;
    wire [255:0]                      dbg_phy_rdlvl;
    wire [99:0]                       dbg_phy_wrcal;
    wire [6*2-1:0]                    dbg_final_po_fine_tap_cnt;
    wire [3*2-1:0]                    dbg_final_po_coarse_tap_cnt;
    wire [2-1:0]                      dbg_rd_data_edge_detect;
    wire [2*4*16-1:0]                 dbg_rddata;
    wire                              dbg_rddata_valid;
    wire [1:0]                        dbg_rdlvl_done;
    wire [1:0]                        dbg_rdlvl_err;
    wire [1:0]                        dbg_rdlvl_start;
    wire [5:0]                        dbg_tap_cnt_during_wrlvl;
    wire                              dbg_wl_edge_detect_valid;
    wire                              dbg_wrlvl_done;
    wire                              dbg_wrlvl_err;
    wire                              dbg_wrlvl_start;
    wire [6*2-1:0]                    dbg_wrlvl_fine_tap_cnt;
    wire [3*2-1:0]                    dbg_wrlvl_coarse_tap_cnt;
    wire [255:0]                      dbg_phy_wrlvl;
    wire                              dbg_pi_phaselock_start;
    wire                              dbg_pi_phaselocked_done;
    wire                              dbg_pi_phaselock_err;
    wire [11:0]                       dbg_pi_phase_locked_phy4lanes;
    wire                              dbg_pi_dqsfound_start;
    wire                              dbg_pi_dqsfound_done;
    wire                              dbg_pi_dqsfound_err;
    wire [11:0]                       dbg_pi_dqs_found_lanes_phy4lanes;
    wire                              dbg_wrcal_start;
    wire                              dbg_wrcal_done;
    wire                              dbg_wrcal_err;
    wire [1023:0]                     dbg_poc;
    wire                              init_wrcal_complete;    
    wire                              ref_dll_lock;
    wire [5:0]                        dbg_rd_data_offset;
    wire [255:0]                      dbg_phy_init;
    wire [255:0]                      dbg_prbs_rdlvl;
    wire [255:0]                      dbg_dqs_found_cal;
    wire [5:0]                        dbg_pi_counter_read_val;
    wire [8:0]                        dbg_po_counter_read_val;
    wire                              dbg_oclkdelay_calib_start;
    wire                              dbg_oclkdelay_calib_done;
    wire [255:0]                      dbg_phy_oclkdelay_cal;
    wire [8*16 -1:0]                  dbg_oclkdelay_rd_data;
    wire [6*2*1-1:0]                  prbs_final_dqs_tap_cnt_r;
    wire [6*2*1-1:0]                  dbg_prbs_first_edge_taps;
    wire [6*2*1-1:0]                  dbg_prbs_second_edge_taps;
    
    // 7
    assign debug [31] = dbg_rdlvl_done[1];
    assign debug [30] = dbg_rdlvl_done[0];
    assign debug [29] = dbg_rdlvl_err[1];
    assign debug [28] = dbg_rdlvl_err[0];
    // 6
    assign debug [27] = dbg_rdlvl_start[1];
    assign debug [26] = dbg_rdlvl_start[0];
    assign debug [25] = dbg_wl_edge_detect_valid;
    assign debug [24] = dbg_wrlvl_done;
    // 5
    assign debug [23] = dbg_wrlvl_err;
    assign debug [22] = dbg_wrlvl_start;
    assign debug [21] = dbg_pi_phaselock_start;
    assign debug [20] = dbg_pi_phaselocked_done;
    // 4
    assign debug [19] = dbg_pi_phaselock_err;
    assign debug [18] = dbg_pi_dqsfound_start;
    assign debug [17] = dbg_pi_dqsfound_done;
    assign debug [16] = dbg_pi_dqsfound_err;
    // 3
    assign debug [15] = dbg_wrcal_start;
    assign debug [14] = dbg_wrcal_done;
    assign debug [13] = dbg_wrcal_err;
    assign debug [12] = init_wrcal_complete;
    // 2
    assign debug [11] = ref_dll_lock;
    assign debug [10] = dbg_oclkdelay_calib_start;
    assign debug [9] = dbg_oclkdelay_calib_done;
    assign debug [8] = init_calib_complete;
    // 1
    assign debug [7] = psincdec;
    assign debug [6] = psen;
    assign debug [5] = ppl_0_locked;
    assign debug [4] = rst_div2;
    // 0
    assign debug [3] = rst_phaser_ref;
    assign debug [2] = rst;
    assign debug [1] = iddr_rst;
    assign debug [0] = !rst_in;
    
//////////////////////////////////////  Control Signals ///////////////////////////////////////////////////
   // Outputs
   wire                    phy_mc_ctl_full;
   wire                    phy_mc_cmd_full;
   wire                    phy_mc_data_full;
   wire [5:0]              calib_rd_data_offset_0;
   wire [5:0]              calib_rd_data_offset_1;
   wire [5:0]              calib_rd_data_offset_2;
   wire [127:0]            phy_rd_data;   
   wire                    phy_rddata_valid;   
   // Constants
   wire                     mc_reset_n;
   wire                     mc_cmd_wren;
   wire                     mc_ctl_wren;
   wire [3:0]               mc_cke;
   wire [1:0]               mc_cas_slot;
   assign  mc_reset_n = 1'b1;
   assign  mc_cmd_wren = 1'b1;
   assign  mc_ctl_wren = 1'b1;
   assign  mc_cke = 4'hF;
   assign  mc_cas_slot = 2'b01;
    
//////////////////////////////////// PHY Instantiation ////////////////////////////////////////////    
  mig_7series_v4_2_ddr_phy_top #
    (
     .TCQ                (100),
     .DDR3_VDD_OP_VOLT   ("135"),
     .REFCLK_FREQ        (200.0),
     .BYTE_LANES_B0      (4'b1111),
     .BYTE_LANES_B1      (4'b0000),
     .BYTE_LANES_B2      (4'b0000),
     .BYTE_LANES_B3      (4'b0000),
     .BYTE_LANES_B4      (4'b0000),
     .PHY_0_BITLANES     (48'b001111111110001111111110111111111111101111111111),
     .PHY_1_BITLANES     (48'b000000000000000000000000000000000000000000000000),
     .PHY_2_BITLANES     (48'b000000000000000000000000000000000000000000000000),
     .CA_MIRROR          ("OFF"),
     .CK_BYTE_MAP        (144'b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000),
     .ADDR_MAP           (192'b000000000000000000000000000000000000000000000010000000000100000000001001000000000111000000000001000000000101000000000110000000000011000000010000000000010010000000010100000000010001000000011010),
     .BANK_MAP           (36'b000000011011000000010111000000010011),
     .CAS_MAP            (12'b000000010101),
     .CKE_ODT_BYTE_MAP   (8'b00000000),
     .CKE_MAP            (96'b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000011001),
     .ODT_MAP            (96'b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001000),
     .CKE_ODT_AUX        ("FALSE"),
     .CS_MAP             (120'b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001011),
     .PARITY_MAP         (12'b000000000000),
     .RAS_MAP            (12'b000000010110),
     .WE_MAP             (12'b000000011000),
     .DQS_BYTE_MAP       (144'b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001000000011),
     .DATA0_MAP          (96'b000000110100000000110010000000111000000000110101000000110001000000110111000000110110000000110011),
     .DATA1_MAP          (96'b000000100011000000100110000000100010000000101000000000100101000000100111000000100001000000100100),
     .DATA2_MAP          (96'b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000),
     .DATA3_MAP          (96'b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000),
     .DATA4_MAP          (96'b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000),
     .DATA5_MAP          (96'b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000),
     .DATA6_MAP          (96'b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000),
     .DATA7_MAP          (96'b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000),
     .DATA8_MAP          (96'b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000),
     .DATA9_MAP          (96'b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000),
     .DATA10_MAP         (96'b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000),
     .DATA11_MAP         (96'b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000),
     .DATA12_MAP         (96'b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000),
     .DATA13_MAP         (96'b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000),
     .DATA14_MAP         (96'b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000),
     .DATA15_MAP         (96'b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000),
     .DATA16_MAP         (96'b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000),
     .DATA17_MAP         (96'b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000),
     .MASK0_MAP          (108'b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000101001000000111001),
     .MASK1_MAP          (108'b000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000),
     .CALIB_ROW_ADD      (16'b0000000000000000),
     .CALIB_COL_ADD      (12'b000000000000),
     .CALIB_BA_ADD       (3'b000),
     .nCS_PER_RANK       (32'd1),
     .CS_WIDTH           (32'd1),
     .nCK_PER_CLK        (32'd4),
     .PRE_REV3ES         ("OFF"),
     .CKE_WIDTH          (32'd1),
     .DATA_CTL_B0        (4'b1100),
     .DATA_CTL_B1        (4'b0000),
     .DATA_CTL_B2        (4'b0000),
     .DATA_CTL_B3        (4'b0000),
     .DATA_CTL_B4        (4'b0000),
     .DDR2_DQSN_ENABLE   ("YES"),
     .DRAM_TYPE          ("DDR3"),
     .BANK_WIDTH         (32'd3),
     .CK_WIDTH           (32'd1),
     .COL_WIDTH          (32'd10),
     .DM_WIDTH           (32'd2),
     .DQ_WIDTH           (32'd16),
     .DQS_CNT_WIDTH      (32'd1),
     .DQS_WIDTH          (32'd2),
     .DRAM_WIDTH         (32'd8),
     .PHYCTL_CMD_FIFO    ("FALSE"),
     .ROW_WIDTH          (32'd14),
     .AL                 ("0"),
     .ADDR_CMD_MODE      ("1T"),
     .BURST_MODE         ("8"),
     .BURST_TYPE         ("SEQ"),
     .CL                 (32'd5),
     .CWL                (32'd5),
     .tRFC               (32'd160000),
     .tREFI              (32'd7800000),
     .tCK                (32'd3000),
     .OUTPUT_DRV         ("HIGH"),
     .RANKS              (32'd1),
     .ODT_WIDTH          (32'd1),
     .REG_CTRL           ("OFF"),
     .RTT_NOM            ("60"),
     .RTT_WR             ("OFF"),
     .SLOT_1_CONFIG      (8'b00000000),
     .WRLVL              ("ON"),
     .BANK_TYPE          ("HR_IO"),
     .DATA_IO_PRIM_TYPE  ("HR_LP"),
     .DATA_IO_IDLE_PWRDWN("ON"),
     .IODELAY_GRP        ("MIG_7SERIES_0_IODELAY_MIG0"),
     .FPGA_SPEED_GRADE   (32'd1),
     .SIM_BYPASS_INIT_CAL ("FAST"),
     .USE_CS_PORT        (32'd1),
     .USE_DM_PORT        (32'd1),
     .USE_ODT_PORT       (32'd1),
     .MASTER_PHY_CTL     (32'd0),
     .DEBUG_PORT         ("OFF"),
     .IDELAY_ADJ         ("OFF"),
     .FINE_PER_BIT       ("OFF"),
     .CENTER_COMP_MODE   ("OFF"),
     .PI_VAL_ADJ         ("OFF"),
     .TAPSPERKCLK        (32'd112),
     .SKIP_CALIB         ("FALSE"),
     .FPGA_VOLT_TYPE     ("N")
     )
    ddr_phy_top0
      (
   .clk(clk),                                                  
   .clk_div2(clk_div2),       
   .rst_div2(rst_div2),     
   .clk_ref(clk_ref),                                            
   .freq_refclk(freq_refclk),   
   .mem_refclk(mem_refclk),     
   .pll_lock(pll_locked),       
   .sync_pulse(sync_pulse),     
   .mmcm_ps_clk(mmcm_ps_clk),    
   .poc_sample_pd(poc_sample_pd),
   .error(1'bz),    
   .device_temp(0),
   .tempmon_sample_en(tempmon_sample_en),
   .dbg_sel_pi_incdec(0),
   .dbg_sel_po_incdec(0),
   .dbg_byte_sel(0),
   .dbg_pi_f_inc(0),
   .dbg_pi_f_dec(0),
   .dbg_po_f_inc(0),
   .dbg_po_f_stg23_sel(0),
   .dbg_po_f_dec(0),
   .dbg_idel_down_all(0),
   .dbg_idel_down_cpt(0),
   .dbg_idel_up_all(0),
   .dbg_idel_up_cpt(0),
   .dbg_sel_all_idel_cpt(0),
   .dbg_sel_idel_cpt(0),
   .rst(rst),
   .iddr_rst(iddr_rst),
   .slot_0_present(8'd1),
   .slot_1_present(8'd0),
   
   .mc_ras_n(mc_ras_n),
   .mc_cas_n(mc_cas_n),
   .mc_we_n(mc_we_n),
   .mc_address(mc_address),
   .mc_bank(mc_bank),
   .mc_cs_n(mc_cs_n),
   .mc_reset_n(mc_reset_n),
   .mc_odt(mc_odt),
   .mc_cke(mc_cke),
   .mc_aux_out0(mc_aux_out0),
   .mc_aux_out1(mc_aux_out1),
   .mc_cmd_wren(mc_cmd_wren),
   .mc_ctl_wren(mc_ctl_wren),
   .mc_cmd(mc_cmd),
   .mc_cas_slot(mc_cas_slot),
   .mc_data_offset(mc_data_offset),
   .mc_data_offset_1(mc_data_offset_1),
   .mc_data_offset_2(mc_data_offset_2),
   .mc_rank_cnt(mc_rank_cnt),
   .mc_wrdata_en(mc_wrdata_en),
   .mc_wrdata(mc_wrdata),
   .mc_wrdata_mask(mc_wrdata_mask),
   
   .idle(idle & init_calib_complete ),
   .calib_tap_addr(0),
   .calib_tap_load(0),
   .calib_tap_val(0),
   .calib_tap_load_done(0),
   .psdone(0),
   .rst_phaser_ref(rst_phaser_ref),

   // Outputs
   .rst_tg_mc(rst_tg_mc),     
   .psen(psen),
   .psincdec(psincdec),
   .calib_tap_req(calib_tap_req),
   .dbg_calib_top(dbg_calib_top),
   .dbg_cpt_first_edge_cnt(dbg_cpt_first_edge_cnt),
   .dbg_cpt_second_edge_cnt(dbg_cpt_second_edge_cnt),
   .dbg_cpt_tap_cnt(dbg_cpt_tap_cnt),
   .dbg_dq_idelay_tap_cnt(dbg_dq_idelay_tap_cnt),
   .dbg_phy_rdlvl(dbg_phy_rdlvl),
   .dbg_phy_wrcal(dbg_phy_wrcal),
   .dbg_final_po_fine_tap_cnt(dbg_final_po_fine_tap_cnt),
   .dbg_final_po_coarse_tap_cnt(dbg_final_po_coarse_tap_cnt),
   .dbg_rd_data_edge_detect(dbg_rd_data_edge_detect),
   .dbg_rddata(dbg_rddata),
   .dbg_rddata_valid(dbg_rddata_valid),
   .dbg_rdlvl_done(dbg_rdlvl_done),
   .dbg_rdlvl_err(dbg_rdlvl_err),
   .dbg_rdlvl_start(dbg_rdlvl_start),
   .dbg_tap_cnt_during_wrlvl(dbg_tap_cnt_during_wrlvl),
   .dbg_wl_edge_detect_valid(dbg_wl_edge_detect_valid),
   .dbg_wrlvl_done(dbg_wrlvl_done),
   .dbg_wrlvl_err(dbg_wrlvl_err),
   .dbg_wrlvl_start(dbg_wrlvl_start),
   .dbg_wrlvl_fine_tap_cnt(dbg_wrlvl_fine_tap_cnt),
   .dbg_wrlvl_coarse_tap_cnt(dbg_wrlvl_coarse_tap_cnt),
   .dbg_phy_wrlvl(dbg_phy_wrlvl),
   .dbg_pi_phaselock_start(dbg_pi_phaselock_start),
   .dbg_pi_phaselocked_done(dbg_pi_phaselocked_done),
   .dbg_pi_phaselock_err(dbg_pi_phaselock_err),
   .dbg_pi_phase_locked_phy4lanes(dbg_pi_phase_locked_phy4lanes),
   .dbg_pi_dqsfound_start(dbg_pi_dqsfound_start),
   .dbg_pi_dqsfound_done(dbg_pi_dqsfound_done),
   .dbg_pi_dqsfound_err(dbg_pi_dqsfound_err),
   .dbg_pi_dqs_found_lanes_phy4lanes(dbg_pi_dqs_found_lanes_phy4lanes),
   .dbg_wrcal_start(dbg_wrcal_start),
   .dbg_wrcal_done(dbg_wrcal_done),
   .dbg_wrcal_err(dbg_wrcal_err),
   .dbg_poc(dbg_poc),
   .phy_mc_ctl_full(phy_mc_ctl_full),
   .phy_mc_cmd_full(phy_mc_cmd_full),
   .phy_mc_data_full(phy_mc_data_full),
   .init_calib_complete(init_calib_complete),
   .init_wrcal_complete(init_wrcal_complete),
   .calib_rd_data_offset_0(calib_rd_data_offset_0),
   .calib_rd_data_offset_1(calib_rd_data_offset_1),
   .calib_rd_data_offset_2(calib_rd_data_offset_2),
   .phy_rddata_valid(phy_rddata_valid),
   .phy_rd_data(phy_rd_data),
   .ref_dll_lock(ref_dll_lock),
   .dbg_rd_data_offset(dbg_rd_data_offset),
   .dbg_phy_init(dbg_phy_init),
   .dbg_prbs_rdlvl(dbg_prbs_rdlvl),
   .dbg_dqs_found_cal(dbg_dqs_found_cal),
   .dbg_pi_counter_read_val(dbg_pi_counter_read_val),
   .dbg_po_counter_read_val(dbg_po_counter_read_val),
   .dbg_oclkdelay_calib_start(dbg_oclkdelay_calib_start),
   .dbg_oclkdelay_calib_done(dbg_oclkdelay_calib_done),
   .dbg_phy_oclkdelay_cal(dbg_phy_oclkdelay_cal),
   .dbg_oclkdelay_rd_data(dbg_oclkdelay_rd_data),
   .prbs_final_dqs_tap_cnt_r(prbs_final_dqs_tap_cnt_r),
   .dbg_prbs_first_edge_taps(dbg_prbs_first_edge_taps),
   .dbg_prbs_second_edge_taps(dbg_prbs_second_edge_taps),
       
       
       //DDR3
       .ddr_ck                      (ddr3_ck_p),
       .ddr_ck_n                    (ddr3_ck_n),
       .ddr_addr                    (ddr3_addr),
       .ddr_ba                      (ddr3_ba),
       .ddr_ras_n                   (ddr3_ras_n),
       .ddr_cas_n                   (ddr3_cas_n),
       .ddr_we_n                    (ddr3_we_n),
       .ddr_cs_n                    (ddr3_cs_n),
       .ddr_cke                     (ddr3_cke),
       .ddr_odt                     (ddr3_odt),
       .ddr_reset_n                 (ddr3_reset_n),
       .ddr_parity                  (),
       .ddr_dm                      (ddr3_dm),
       .ddr_dqs                     (ddr3_dqs_p),
       .ddr_dqs_n                   (ddr3_dqs_n),
       .ddr_dq                      (ddr3_dq)
      );
    
    
    
endmodule
