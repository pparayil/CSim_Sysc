CC = g++

SYSTEMC = /nfs/lis/tools/systemc/2.3.0
LISNOC = /home/ga93soj/dev/lisnoc/
VERILATOR_ROOT = /nfs/lis/tools/veripool/verilator

ARCHIVES=$(SYSTEMC)/lib-linux64/libsystemc.a \
         $(LISNOC)/sysc/src/modules/LISNoC_mesh4x4_vc2/LISNoC_mesh4x4_vc2__ALL.a

INCSYSTEMC = $(SYSTEMC)/include
INCSYSTEMCMESH = $(LISNOC)/sysc/include/modules/LISNoC_mesh4x4_vc2/
INCMESH = $(LISNOC)/sysc/src/modules/LISNoC_mesh4x4_vc2_trace/
INCVERILATOR = $(VERILATOR_ROOT)/include

INCLUDE = -I $(INCSYSTEMC) -I$(INCSYSTEMCMESH) -I $(INCMESH) -I $(INCVERILATOR)

SRC = ./datatype_ch.cpp ./Req_generator_Prsr.cpp ./PRIV_L1.cpp ./LLC.cpp ./Directory.cpp ./mem.cpp ./common_lsu.cpp ./NPC.cpp\
	./mem_lsu.cpp ./node_internal.cpp ./memnode_internal.cpp ./LisNoCProcessingElementunit.cpp ./LiSNOCTop.cpp \
	  $(LISNOCLIB) $(VERILATORLIB) $(LIB)

all: LiSNOCTop

verilated.o:
	$(CC) -c $(INCVERILATOR)/verilated.cpp -o verilated.o

verilated_vcd_c.o:
	$(CC) -c $(INCVERILATOR)/verilated_vcd_c.cpp -o verilated_vcd_c.o

LiSNOCTop: 
	$(CC) $(SRC) $(INCLUDE) $< verilated.o verilated_vcd_c.o $(ARCHIVES) -g  -lpthread -o $@

clean: 
	rm ./sim.vcd ./LiSNOCTop
