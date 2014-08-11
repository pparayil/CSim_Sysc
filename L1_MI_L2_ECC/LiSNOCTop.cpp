#include "systemc.h"
#include <pthread.h>

#include "LISNoC_mesh4x4_vc2.h"
#include "LisNoCProcessingElementunit.h"

//#include <LISNoC_mesh4x4_vc2_trace.h>
#include <verilated_vcd_c.h>

SC_MODULE(tracemon) {
	sc_in<bool> clk;

	SC_CTOR(tracemon) : clk("clk") {
		vcd = NULL;
		SC_METHOD(dump);
		sensitive << clk;
	}

	void dump() {
		if (vcd) {
			; //vcd->dump((uint32_t)(sc_time_stamp().value()/1000));
		}
	}

	VerilatedVcdC *vcd;
};



int sc_main (int argc, char * argv[])
{
	LISNoC_mesh4x4_vc2 lisnoc_net("lisnoc_net");
	long long total_noc_packet_counter = 0x0;
	
	if(argv[1] == NULL)
	argv[1] = "./traces/facesim_trace_15c.txt";

	LisNoCProcessingElement pe0("pe0",false,0, argv[1]);
	LisNoCProcessingElement pe1("pe1",false,1, argv[1]);
	LisNoCProcessingElement pe2("pe2",false,2, argv[1]);
	LisNoCProcessingElement pe3("pe3",false,3, argv[1]);	
	LisNoCProcessingElement pe4("pe4",false,4, argv[1]);
	LisNoCProcessingElement pe5("pe5",false,5, argv[1]);
	LisNoCProcessingElement pe6("pe6",false,6, argv[1]);
	LisNoCProcessingElement pe7("pe7",false,7, argv[1]);
	LisNoCProcessingElement pe8("pe8",false,8, argv[1]);
	LisNoCProcessingElement pe9("pe9",false,9, argv[1]);
	LisNoCProcessingElement pe10("pe10",false,10, argv[1]);
	LisNoCProcessingElement pe11("pe11",false,11, argv[1]);
	LisNoCProcessingElement pe12("pe12",false,12, argv[1]);
	LisNoCProcessingElement pe13("pe13",false,13, argv[1]);
	LisNoCProcessingElement pe14("pe14",false,14, argv[1]);
	LisNoCProcessingElement pe15("pe15",true,15, argv[1]);

	sc_clock clk("clk",5,SC_NS,0.5);
	sc_signal <bool> rst;
	sc_signal<vluint64_t>	link0_in_flit_i;
	sc_signal<vluint64_t>	link0_out_flit_o;
	sc_signal<vluint64_t>	link1_in_flit_i;
	sc_signal<vluint64_t>	link1_out_flit_o;
	sc_signal<vluint64_t>	link2_in_flit_i;
	sc_signal<vluint64_t>	link2_out_flit_o;
	sc_signal<vluint64_t>	link3_in_flit_i;
	sc_signal<vluint64_t>	link3_out_flit_o;
	sc_signal<vluint64_t>	link4_in_flit_i;
	sc_signal<vluint64_t>	link4_out_flit_o;
	sc_signal<vluint64_t>	link5_in_flit_i;
	sc_signal<vluint64_t>	link5_out_flit_o;
	sc_signal<vluint64_t>	link6_in_flit_i;
	sc_signal<vluint64_t>	link6_out_flit_o;
	sc_signal<vluint64_t>	link7_in_flit_i;
	sc_signal<vluint64_t>	link7_out_flit_o;
	sc_signal<vluint64_t>	link8_in_flit_i;
	sc_signal<vluint64_t>	link8_out_flit_o;
	sc_signal<vluint64_t>	link9_in_flit_i;
	sc_signal<vluint64_t>	link9_out_flit_o;
	sc_signal<vluint64_t>	link10_in_flit_i;
	sc_signal<vluint64_t>	link10_out_flit_o;
	sc_signal<vluint64_t>	link11_in_flit_i;
	sc_signal<vluint64_t>	link11_out_flit_o;
	sc_signal<vluint64_t>	link12_in_flit_i;
	sc_signal<vluint64_t>	link12_out_flit_o;
	sc_signal<vluint64_t>	link13_in_flit_i;
	sc_signal<vluint64_t>	link13_out_flit_o;
	sc_signal<vluint64_t>	link14_in_flit_i;
	sc_signal<vluint64_t>	link14_out_flit_o;
	sc_signal<vluint64_t>	link15_in_flit_i;
	sc_signal<vluint64_t>	link15_out_flit_o;
	sc_signal<sc_bv<2> >	link0_in_valid_i;
	sc_signal<sc_bv<2> >	link0_in_ready_o;
	sc_signal<sc_bv<2> >	link0_out_valid_o;
	sc_signal<sc_bv<2> >	link0_out_ready_i;
	sc_signal<sc_bv<2> >	link1_in_valid_i;
	sc_signal<sc_bv<2> >	link1_in_ready_o;
	sc_signal<sc_bv<2> >	link1_out_valid_o;
	sc_signal<sc_bv<2> >	link1_out_ready_i;
	sc_signal<sc_bv<2> >	link2_in_valid_i;
	sc_signal<sc_bv<2> >	link2_in_ready_o;
	sc_signal<sc_bv<2> >	link2_out_valid_o;
	sc_signal<sc_bv<2> >	link2_out_ready_i;
	sc_signal<sc_bv<2> >	link3_in_valid_i;
	sc_signal<sc_bv<2> >	link3_in_ready_o;
	sc_signal<sc_bv<2> >	link3_out_valid_o;
	sc_signal<sc_bv<2> >	link3_out_ready_i;
	sc_signal<sc_bv<2> >	link4_in_valid_i;
	sc_signal<sc_bv<2> >	link4_in_ready_o;
	sc_signal<sc_bv<2> >	link4_out_valid_o;
	sc_signal<sc_bv<2> >	link4_out_ready_i;
	sc_signal<sc_bv<2> >	link5_in_valid_i;
	sc_signal<sc_bv<2> >	link5_in_ready_o;
	sc_signal<sc_bv<2> >	link5_out_valid_o;
	sc_signal<sc_bv<2> >	link5_out_ready_i;
	sc_signal<sc_bv<2> >	link6_in_valid_i;
	sc_signal<sc_bv<2> >	link6_in_ready_o;
	sc_signal<sc_bv<2> >	link6_out_valid_o;
	sc_signal<sc_bv<2> >	link6_out_ready_i;
	sc_signal<sc_bv<2> >	link7_in_valid_i;
	sc_signal<sc_bv<2> >	link7_in_ready_o;
	sc_signal<sc_bv<2> >	link7_out_valid_o;
	sc_signal<sc_bv<2> >	link7_out_ready_i;
	sc_signal<sc_bv<2> >	link8_in_valid_i;
	sc_signal<sc_bv<2> >	link8_in_ready_o;
	sc_signal<sc_bv<2> >	link8_out_valid_o;
	sc_signal<sc_bv<2> >	link8_out_ready_i;
	sc_signal<sc_bv<2> >	link9_in_valid_i;
	sc_signal<sc_bv<2> >	link9_in_ready_o;
	sc_signal<sc_bv<2> >	link9_out_valid_o;
	sc_signal<sc_bv<2> >	link9_out_ready_i;
	sc_signal<sc_bv<2> >	link10_in_valid_i;
	sc_signal<sc_bv<2> >	link10_in_ready_o;
	sc_signal<sc_bv<2> >	link10_out_valid_o;
	sc_signal<sc_bv<2> >	link10_out_ready_i;
	sc_signal<sc_bv<2> >	link11_in_valid_i;
	sc_signal<sc_bv<2> >	link11_in_ready_o;
	sc_signal<sc_bv<2> >	link11_out_valid_o;
	sc_signal<sc_bv<2> >	link11_out_ready_i;
	sc_signal<sc_bv<2> >	link12_in_valid_i;
	sc_signal<sc_bv<2> >	link12_in_ready_o;
	sc_signal<sc_bv<2> >	link12_out_valid_o;
	sc_signal<sc_bv<2> >	link12_out_ready_i;
	sc_signal<sc_bv<2> >	link13_in_valid_i;
	sc_signal<sc_bv<2> >	link13_in_ready_o;
	sc_signal<sc_bv<2> >	link13_out_valid_o;
	sc_signal<sc_bv<2> >	link13_out_ready_i;
	sc_signal<sc_bv<2> >	link14_in_valid_i;
	sc_signal<sc_bv<2> >	link14_in_ready_o;
	sc_signal<sc_bv<2> >	link14_out_valid_o;
	sc_signal<sc_bv<2> >	link14_out_ready_i;
	sc_signal<sc_bv<2> >	link15_in_valid_i;
	sc_signal<sc_bv<2> >	link15_in_ready_o;
	sc_signal<sc_bv<2> >	link15_out_valid_o;
	sc_signal<sc_bv<2> >	link15_out_ready_i;

	lisnoc_net.clk(clk);
	lisnoc_net.rst(rst);
	lisnoc_net.link0_in_flit_i.bind(link0_in_flit_i);
	lisnoc_net.link0_out_flit_o.bind(link0_out_flit_o );
	lisnoc_net.link1_in_flit_i.bind(link1_in_flit_i );
	lisnoc_net.link1_out_flit_o.bind(link1_out_flit_o );
	lisnoc_net.link2_in_flit_i.bind(link2_in_flit_i );
	lisnoc_net.link2_out_flit_o.bind(link2_out_flit_o );
	lisnoc_net.link3_in_flit_i.bind(link3_in_flit_i );
	lisnoc_net.link4_in_flit_i.bind(link4_in_flit_i );
	lisnoc_net.link4_out_flit_o.bind(link4_out_flit_o );
	lisnoc_net.link5_in_flit_i.bind(link5_in_flit_i );
	lisnoc_net.link5_out_flit_o.bind(link5_out_flit_o );
	lisnoc_net.link6_in_flit_i.bind(link6_in_flit_i );
	lisnoc_net.link6_out_flit_o.bind(link6_out_flit_o );
	lisnoc_net.link7_in_flit_i.bind(link7_in_flit_i );
	lisnoc_net.link7_out_flit_o.bind(link7_out_flit_o );
	lisnoc_net.link8_in_flit_i.bind(link8_in_flit_i );
	lisnoc_net.link8_out_flit_o.bind(link8_out_flit_o );
	lisnoc_net.link9_in_flit_i.bind(link9_in_flit_i );
	lisnoc_net.link9_out_flit_o.bind(link9_out_flit_o );
	lisnoc_net.link10_in_flit_i.bind(link10_in_flit_i );
	lisnoc_net.link10_out_flit_o.bind(link10_out_flit_o );
	lisnoc_net.link11_in_flit_i.bind(link11_in_flit_i );
	lisnoc_net.link11_out_flit_o.bind(link11_out_flit_o );
	lisnoc_net.link12_in_flit_i.bind(link12_in_flit_i );
	lisnoc_net.link12_out_flit_o.bind(link12_out_flit_o );
	lisnoc_net.link13_in_flit_i.bind(link13_in_flit_i );
	lisnoc_net.link13_out_flit_o.bind(link13_out_flit_o );
	lisnoc_net.link14_in_flit_i.bind(link14_in_flit_i );
	lisnoc_net.link14_out_flit_o.bind(link14_out_flit_o );
	lisnoc_net.link15_in_flit_i.bind(link15_in_flit_i );
	lisnoc_net.link15_out_flit_o.bind(link15_out_flit_o );

	lisnoc_net.link3_out_flit_o.bind( link3_out_flit_o);
	lisnoc_net.link0_in_valid_i.bind(link0_in_valid_i );
	lisnoc_net.link0_in_ready_o.bind( link0_in_ready_o);
	lisnoc_net.link0_out_valid_o.bind( link0_out_valid_o);
	lisnoc_net.link0_out_ready_i.bind(link0_out_ready_i );
	lisnoc_net.link1_in_valid_i.bind( link1_in_valid_i);
	lisnoc_net.link1_in_ready_o.bind( link1_in_ready_o);
	lisnoc_net.link1_out_valid_o.bind( link1_out_valid_o);
	lisnoc_net.link1_out_ready_i.bind(link1_out_ready_i );
	lisnoc_net.link2_in_valid_i.bind(link2_in_valid_i );
	lisnoc_net.link2_in_ready_o.bind(link2_in_ready_o );
	lisnoc_net.link2_out_valid_o(link2_out_valid_o );
	lisnoc_net.link2_out_ready_i(link2_out_ready_i );
	lisnoc_net.link3_in_valid_i.bind(link3_in_valid_i );
	lisnoc_net.link3_in_ready_o.bind(link3_in_ready_o );
	lisnoc_net.link3_out_valid_o.bind( link3_out_valid_o);
	lisnoc_net.link3_out_ready_i.bind( link3_out_ready_i);
	lisnoc_net.link4_in_valid_i.bind( link4_in_valid_i);
	lisnoc_net.link4_in_ready_o.bind( link4_in_ready_o);
	lisnoc_net.link4_out_valid_o.bind( link4_out_valid_o);
	lisnoc_net.link4_out_ready_i.bind(link4_out_ready_i );
	lisnoc_net.link5_in_valid_i.bind( link5_in_valid_i);
	lisnoc_net.link5_in_ready_o.bind( link5_in_ready_o);
	lisnoc_net.link5_out_valid_o.bind( link5_out_valid_o);
	lisnoc_net.link5_out_ready_i.bind(link5_out_ready_i );
	lisnoc_net.link6_in_valid_i.bind( link6_in_valid_i);
	lisnoc_net.link6_in_ready_o.bind( link6_in_ready_o);
	lisnoc_net.link6_out_valid_o.bind( link6_out_valid_o);
	lisnoc_net.link6_out_ready_i.bind(link6_out_ready_i );
	lisnoc_net.link7_in_valid_i.bind( link7_in_valid_i);
	lisnoc_net.link7_in_ready_o.bind( link7_in_ready_o);
	lisnoc_net.link7_out_valid_o.bind( link7_out_valid_o);
	lisnoc_net.link7_out_ready_i.bind(link7_out_ready_i );
	lisnoc_net.link8_in_valid_i.bind( link8_in_valid_i);
	lisnoc_net.link8_in_ready_o.bind( link8_in_ready_o);
	lisnoc_net.link8_out_valid_o.bind( link8_out_valid_o);
	lisnoc_net.link8_out_ready_i.bind(link8_out_ready_i );
	lisnoc_net.link9_in_valid_i.bind( link9_in_valid_i);
	lisnoc_net.link9_in_ready_o.bind( link9_in_ready_o);
	lisnoc_net.link9_out_valid_o.bind( link9_out_valid_o);
	lisnoc_net.link9_out_ready_i.bind(link9_out_ready_i );
	lisnoc_net.link10_in_valid_i.bind( link10_in_valid_i);
	lisnoc_net.link10_in_ready_o.bind( link10_in_ready_o);
	lisnoc_net.link10_out_valid_o.bind( link10_out_valid_o);
	lisnoc_net.link10_out_ready_i.bind(link10_out_ready_i );
	lisnoc_net.link11_in_valid_i.bind( link11_in_valid_i);
	lisnoc_net.link11_in_ready_o.bind( link11_in_ready_o);
	lisnoc_net.link11_out_valid_o.bind( link11_out_valid_o);
	lisnoc_net.link11_out_ready_i.bind(link11_out_ready_i );
	lisnoc_net.link12_in_valid_i.bind( link12_in_valid_i);
	lisnoc_net.link12_in_ready_o.bind( link12_in_ready_o);
	lisnoc_net.link12_out_valid_o.bind( link12_out_valid_o);
	lisnoc_net.link12_out_ready_i.bind(link12_out_ready_i );
	lisnoc_net.link13_in_valid_i.bind( link13_in_valid_i);
	lisnoc_net.link13_in_ready_o.bind( link13_in_ready_o);
	lisnoc_net.link13_out_valid_o.bind( link13_out_valid_o);
	lisnoc_net.link13_out_ready_i.bind(link13_out_ready_i );
	lisnoc_net.link14_in_valid_i.bind( link14_in_valid_i);
	lisnoc_net.link14_in_ready_o.bind( link14_in_ready_o);
	lisnoc_net.link14_out_valid_o.bind( link14_out_valid_o);
	lisnoc_net.link14_out_ready_i.bind(link14_out_ready_i );
	lisnoc_net.link15_in_valid_i.bind( link15_in_valid_i);
	lisnoc_net.link15_in_ready_o.bind( link15_in_ready_o);
	lisnoc_net.link15_out_valid_o.bind( link15_out_valid_o);
	lisnoc_net.link15_out_ready_i.bind(link15_out_ready_i );
	pe0.clk(clk);
	pe0.rst(rst);
	pe0.link_in_flit_i.bind(link0_in_flit_i);
	pe0.link_out_flit_o.bind(link0_out_flit_o );
	pe0.link_in_valid_i(link0_in_valid_i );
	pe0.link_in_ready_o( link0_in_ready_o);
	pe0.link_out_valid_o( link0_out_valid_o);
	pe0.link_out_ready_i(link0_out_ready_i );

	pe1.clk(clk);
	pe1.rst(rst);
	pe1.link_in_flit_i.bind(link1_in_flit_i);
	pe1.link_out_flit_o.bind(link1_out_flit_o );
	pe1.link_in_valid_i(link1_in_valid_i );
	pe1.link_in_ready_o( link1_in_ready_o);
	pe1.link_out_valid_o( link1_out_valid_o);
	pe1.link_out_ready_i(link1_out_ready_i );

	pe2.clk(clk);
	pe2.rst(rst);
	pe2.link_in_flit_i.bind(link2_in_flit_i);
	pe2.link_out_flit_o.bind(link2_out_flit_o );
	pe2.link_in_valid_i(link2_in_valid_i );
	pe2.link_in_ready_o( link2_in_ready_o);
	pe2.link_out_valid_o( link2_out_valid_o);
	pe2.link_out_ready_i(link2_out_ready_i );

	pe3.clk(clk);
	pe3.rst(rst);
	pe3.link_in_flit_i.bind(link3_in_flit_i);
	pe3.link_out_flit_o.bind(link3_out_flit_o );
	pe3.link_in_valid_i(link3_in_valid_i );
	pe3.link_in_ready_o( link3_in_ready_o);
	pe3.link_out_valid_o( link3_out_valid_o);
	pe3.link_out_ready_i(link3_out_ready_i );

	pe4.clk(clk);
	pe4.rst(rst);
	pe4.link_in_flit_i.bind(link4_in_flit_i);
	pe4.link_out_flit_o.bind(link4_out_flit_o );
	pe4.link_in_valid_i(link4_in_valid_i );
	pe4.link_in_ready_o( link4_in_ready_o);
	pe4.link_out_valid_o( link4_out_valid_o);
	pe4.link_out_ready_i(link4_out_ready_i );

	pe5.clk(clk);
	pe5.rst(rst);
	pe5.link_in_flit_i.bind(link5_in_flit_i);
	pe5.link_out_flit_o.bind(link5_out_flit_o );
	pe5.link_in_valid_i(link5_in_valid_i );
	pe5.link_in_ready_o( link5_in_ready_o);
	pe5.link_out_valid_o( link5_out_valid_o);
	pe5.link_out_ready_i(link5_out_ready_i );

	pe6.clk(clk);
	pe6.rst(rst);
	pe6.link_in_flit_i.bind(link6_in_flit_i);
	pe6.link_out_flit_o.bind(link6_out_flit_o );
	pe6.link_in_valid_i(link6_in_valid_i );
	pe6.link_in_ready_o( link6_in_ready_o);
	pe6.link_out_valid_o( link6_out_valid_o);
	pe6.link_out_ready_i(link6_out_ready_i );

	pe7.clk(clk);
	pe7.rst(rst);
	pe7.link_in_flit_i.bind(link7_in_flit_i);
	pe7.link_out_flit_o.bind(link7_out_flit_o );
	pe7.link_in_valid_i(link7_in_valid_i );
	pe7.link_in_ready_o( link7_in_ready_o);
	pe7.link_out_valid_o( link7_out_valid_o);
	pe7.link_out_ready_i(link7_out_ready_i );

	pe8.clk(clk);
	pe8.rst(rst);
	pe8.link_in_flit_i.bind(link8_in_flit_i);
	pe8.link_out_flit_o.bind(link8_out_flit_o );
	pe8.link_in_valid_i(link8_in_valid_i );
	pe8.link_in_ready_o( link8_in_ready_o);
	pe8.link_out_valid_o( link8_out_valid_o);
	pe8.link_out_ready_i(link8_out_ready_i );

	pe9.clk(clk);
	pe9.rst(rst);
	pe9.link_in_flit_i.bind(link9_in_flit_i);
	pe9.link_out_flit_o.bind(link9_out_flit_o );
	pe9.link_in_valid_i(link9_in_valid_i );
	pe9.link_in_ready_o( link9_in_ready_o);
	pe9.link_out_valid_o( link9_out_valid_o);
	pe9.link_out_ready_i(link9_out_ready_i );

	pe10.clk(clk);
	pe10.rst(rst);
	pe10.link_in_flit_i.bind(link10_in_flit_i);
	pe10.link_out_flit_o.bind(link10_out_flit_o );
	pe10.link_in_valid_i(link10_in_valid_i );
	pe10.link_in_ready_o( link10_in_ready_o);
	pe10.link_out_valid_o( link10_out_valid_o);
	pe10.link_out_ready_i(link10_out_ready_i );

	pe11.clk(clk);
	pe11.rst(rst);
	pe11.link_in_flit_i.bind(link11_in_flit_i);
	pe11.link_out_flit_o.bind(link11_out_flit_o );
	pe11.link_in_valid_i(link11_in_valid_i );
	pe11.link_in_ready_o( link11_in_ready_o);
	pe11.link_out_valid_o( link11_out_valid_o);
	pe11.link_out_ready_i(link11_out_ready_i );
	
	pe12.clk(clk);
	pe12.rst(rst);
	pe12.link_in_flit_i.bind(link12_in_flit_i);
	pe12.link_out_flit_o.bind(link12_out_flit_o );
	pe12.link_in_valid_i(link12_in_valid_i );
	pe12.link_in_ready_o( link12_in_ready_o);
	pe12.link_out_valid_o( link12_out_valid_o);
	pe12.link_out_ready_i(link12_out_ready_i );

	pe13.clk(clk);
	pe13.rst(rst);
	pe13.link_in_flit_i.bind(link13_in_flit_i);
	pe13.link_out_flit_o.bind(link13_out_flit_o );
	pe13.link_in_valid_i(link13_in_valid_i );
	pe13.link_in_ready_o( link13_in_ready_o);
	pe13.link_out_valid_o( link13_out_valid_o);
	pe13.link_out_ready_i(link13_out_ready_i );

	pe14.clk(clk);
	pe14.rst(rst);
	pe14.link_in_flit_i.bind(link14_in_flit_i);
	pe14.link_out_flit_o.bind(link14_out_flit_o );
	pe14.link_in_valid_i(link14_in_valid_i );
	pe14.link_in_ready_o( link14_in_ready_o);
	pe14.link_out_valid_o( link14_out_valid_o);
	pe14.link_out_ready_i(link14_out_ready_i );

	pe15.clk(clk);
	pe15.rst(rst);
	pe15.link_in_flit_i.bind(link15_in_flit_i);
	pe15.link_out_flit_o.bind(link15_out_flit_o );
	pe15.link_in_valid_i(link15_in_valid_i );
	pe15.link_in_ready_o( link15_in_ready_o);
	pe15.link_out_valid_o( link15_out_valid_o);
	pe15.link_out_ready_i(link15_out_ready_i );


	tracemon trace("trace");
	trace.clk(clk);

	Verilated::traceEverOn(true);

	VerilatedVcdC vcd;
	//lisnoc_net.trace(&vcd,99,0);
	vcd.open("sim.vcd");
	trace.vcd = &vcd;
	
	rst.write(1);
	sc_start(2,SC_NS);

	rst.write(0);
	sc_start((int)1000,SC_US); //2000000,SC_NS);
	cout<<" Simulationinitialization!!"<<endl;
	total_noc_packet_counter = (pe0.total_noc_count_in () + 
					pe1.total_noc_count_in ()+ 
					pe2.total_noc_count_in ()+ 
					pe3.total_noc_count_in ()+ 
					pe4.total_noc_count_in ()+ 
					pe5.total_noc_count_in ()+ 
					pe6.total_noc_count_in ()+ 
					pe7.total_noc_count_in ()+ 
					pe8.total_noc_count_in ()+ 
					pe9.total_noc_count_in ()+ 
					pe10.total_noc_count_in ()+ 
					pe11.total_noc_count_in ()+ 
					pe12.total_noc_count_in ()+ 
					pe13.total_noc_count_in ()+ 
					pe14.total_noc_count_in ()+ 
					pe15.total_noc_count_in ());	
	cout<<" total_flits_into_noc:" <<(int)total_noc_packet_counter<<endl;	
	cout<<" $$$$$$diff"<<endl;
	total_noc_packet_counter = (pe0.total_noc_count_out () + 
					pe1.total_noc_count_out ()+ 
					pe2.total_noc_count_out ()+ 
					pe3.total_noc_count_out ()+ 
					pe4.total_noc_count_out ()+ 
					pe5.total_noc_count_out ()+ 
					pe6.total_noc_count_out ()+ 
					pe7.total_noc_count_out ()+ 
					pe8.total_noc_count_out ()+ 
					pe9.total_noc_count_out ()+ 
					pe10.total_noc_count_out ()+ 
					pe11.total_noc_count_out ()+ 
					pe12.total_noc_count_out ()+ 
					pe13.total_noc_count_out ()+ 
					pe14.total_noc_count_out ()+ 
					pe15.total_noc_count_out ());	
	cout<<" total_flits_in_noc_now:" <<(int)total_noc_packet_counter<<endl;
	for( int j = 0; j< 30000;j++){
		sc_start((int)1,SC_US); 
		total_noc_packet_counter = (pe0.total_noc_count_in () + 
					pe1.total_noc_count_in ()+ 
					pe2.total_noc_count_in ()+ 
					pe3.total_noc_count_in ()+ 
					pe4.total_noc_count_in ()+ 
					pe5.total_noc_count_in ()+ 
					pe6.total_noc_count_in ()+ 
					pe7.total_noc_count_in ()+ 
					pe8.total_noc_count_in ()+ 
					pe9.total_noc_count_in ()+ 
					pe10.total_noc_count_in ()+ 
					pe11.total_noc_count_in ()+ 
					pe12.total_noc_count_in ()+ 
					pe13.total_noc_count_in ()+ 
					pe14.total_noc_count_in ()+ 
					pe15.total_noc_count_in ());	
		cout<<" total_flits_into_noc:" <<(int)total_noc_packet_counter<<endl;	
		cout<<" $$$$$$diff"<<endl;
		total_noc_packet_counter = (pe0.total_noc_count_out () + 
						pe1.total_noc_count_out ()+ 
						pe2.total_noc_count_out ()+ 
						pe3.total_noc_count_out ()+ 
						pe4.total_noc_count_out ()+ 
						pe5.total_noc_count_out ()+ 
						pe6.total_noc_count_out ()+ 
						pe7.total_noc_count_out ()+ 
						pe8.total_noc_count_out ()+ 
						pe9.total_noc_count_out ()+ 
						pe10.total_noc_count_out ()+ 
						pe11.total_noc_count_out ()+ 
						pe12.total_noc_count_out ()+ 
						pe13.total_noc_count_out ()+ 
						pe14.total_noc_count_out ()+ 
						pe15.total_noc_count_out ());	
		cout<<" total_flits_in_noc_now:" <<(int)total_noc_packet_counter<<endl;
	}			
	cout<<" $$$$$$Simulation_Results$$$$$$"<<endl;
	cout<<pe0.node_internals->pr0.time_taken<<endl;
	cout<<pe1.node_internals->pr0.time_taken<<endl;
	cout<<pe2.node_internals->pr0.time_taken<<endl;
	cout<<pe3.node_internals->pr0.time_taken<<endl;
	cout<<pe4.node_internals->pr0.time_taken<<endl;
	cout<<pe5.node_internals->pr0.time_taken<<endl;
	cout<<pe6.node_internals->pr0.time_taken<<endl;
	cout<<pe7.node_internals->pr0.time_taken<<endl;
	cout<<pe8.node_internals->pr0.time_taken<<endl;
	cout<<pe9.node_internals->pr0.time_taken<<endl;
	cout<<pe10.node_internals->pr0.time_taken<<endl;
	cout<<pe11.node_internals->pr0.time_taken<<endl;
	cout<<pe12.node_internals->pr0.time_taken<<endl;
	cout<<pe13.node_internals->pr0.time_taken<<endl;
	cout<<pe14.node_internals->pr0.time_taken<<endl;
	cout<<" $$$$$$ " << endl; 
	cout<<pe0.node_internals->l1.l1_miss_count<<endl;
	cout<<pe1.node_internals->l1.l1_miss_count<<endl;
	cout<<pe2.node_internals->l1.l1_miss_count<<endl;
	cout<<pe3.node_internals->l1.l1_miss_count<<endl;
	cout<<pe4.node_internals->l1.l1_miss_count<<endl;
	cout<<pe5.node_internals->l1.l1_miss_count<<endl;
	cout<<pe6.node_internals->l1.l1_miss_count<<endl;
	cout<<pe7.node_internals->l1.l1_miss_count<<endl;
	cout<<pe8.node_internals->l1.l1_miss_count<<endl;
	cout<<pe9.node_internals->l1.l1_miss_count<<endl;
	cout<<pe10.node_internals->l1.l1_miss_count<<endl;
	cout<<pe11.node_internals->l1.l1_miss_count<<endl;
	cout<<pe12.node_internals->l1.l1_miss_count<<endl;
	cout<<pe13.node_internals->l1.l1_miss_count<<endl;
	cout<<pe14.node_internals->l1.l1_miss_count<<endl;
	cout<<" $$$$$$ " << endl; 
	cout<<pe0.node_internals->l1.l1_hit_count<<endl;
	cout<<pe1.node_internals->l1.l1_hit_count<<endl;
	cout<<pe2.node_internals->l1.l1_hit_count<<endl;
	cout<<pe3.node_internals->l1.l1_hit_count<<endl;
	cout<<pe4.node_internals->l1.l1_hit_count<<endl;
	cout<<pe5.node_internals->l1.l1_hit_count<<endl;
	cout<<pe6.node_internals->l1.l1_hit_count<<endl;
	cout<<pe7.node_internals->l1.l1_hit_count<<endl;
	cout<<pe8.node_internals->l1.l1_hit_count<<endl;
	cout<<pe9.node_internals->l1.l1_hit_count<<endl;
	cout<<pe10.node_internals->l1.l1_hit_count<<endl;
	cout<<pe11.node_internals->l1.l1_hit_count<<endl;
	cout<<pe12.node_internals->l1.l1_hit_count<<endl;
	cout<<pe13.node_internals->l1.l1_hit_count<<endl;
	cout<<pe14.node_internals->l1.l1_hit_count<<endl;
	cout<<" $$$$$$ " << endl; 
	cout<<pe0.node_internals->l2.l2_miss_count<<endl;
	cout<<pe1.node_internals->l2.l2_miss_count<<endl;
	cout<<pe2.node_internals->l2.l2_miss_count<<endl;
	cout<<pe3.node_internals->l2.l2_miss_count<<endl;
	cout<<pe4.node_internals->l2.l2_miss_count<<endl;
	cout<<pe5.node_internals->l2.l2_miss_count<<endl;
	cout<<pe6.node_internals->l2.l2_miss_count<<endl;
	cout<<pe7.node_internals->l2.l2_miss_count<<endl;
	cout<<pe8.node_internals->l2.l2_miss_count<<endl;
	cout<<pe9.node_internals->l2.l2_miss_count<<endl;
	cout<<pe10.node_internals->l2.l2_miss_count<<endl;
	cout<<pe11.node_internals->l2.l2_miss_count<<endl;
	cout<<pe12.node_internals->l2.l2_miss_count<<endl;
	cout<<pe13.node_internals->l2.l2_miss_count<<endl;
	cout<<pe14.node_internals->l2.l2_miss_count<<endl;
	cout<<" $$$$$$ " << endl; 
	cout<<pe0.node_internals->l2.l2_hit_count<<endl;
	cout<<pe1.node_internals->l2.l2_hit_count<<endl;
	cout<<pe2.node_internals->l2.l2_hit_count<<endl;
	cout<<pe3.node_internals->l2.l2_hit_count<<endl;
	cout<<pe4.node_internals->l2.l2_hit_count<<endl;
	cout<<pe5.node_internals->l2.l2_hit_count<<endl;
	cout<<pe6.node_internals->l2.l2_hit_count<<endl;
	cout<<pe7.node_internals->l2.l2_hit_count<<endl;
	cout<<pe8.node_internals->l2.l2_hit_count<<endl;
	cout<<pe9.node_internals->l2.l2_hit_count<<endl;
	cout<<pe10.node_internals->l2.l2_hit_count<<endl;
	cout<<pe11.node_internals->l2.l2_hit_count<<endl;
	cout<<pe12.node_internals->l2.l2_hit_count<<endl;
	cout<<pe13.node_internals->l2.l2_hit_count<<endl;
	cout<<pe14.node_internals->l2.l2_hit_count<<endl;
	cout<<" $$$$$$ " << endl; 
	cout<<pe15.memnode_internals->ram.mem_access_count_rd<<endl;
	cout<<" $$$$$$ " << endl;
	cout<<(pe15.memnode_internals->ram.mem_access_count_rd + pe15.memnode_internals->ram.mem_access_count_wr)<<endl;
	cout<<" $$$$$$Simulation_Results$$$$$$"<<endl;
//	sc_start();
	return 0;
}
