/*
 * Neumo dvb (C) 2019 deeptho@gmail.com
 * Copyright notice:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <stdint.h>
#include <resolv.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <values.h>
#include <string.h>
#include <syslog.h>
#include <getopt.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <linux/dvb/version.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/limits.h>
#include <pthread.h>
#include <algorithm>
#include <iomanip>
#include <cassert>
#include <iostream>
#include<regex>
#include <boost/program_options.hpp>
#include "CLI/CLI.hpp"
#include "neumofrontend.h"


using namespace boost;
namespace po = boost::program_options;

static constexpr int	make_code (int pls_mode, int pls_code, int timeout=0) {
	return (timeout&0xff) | ((pls_code & 0x3FFFF)<<8) | (((pls_mode) & 0x3)<<26);
}

struct options_t {
	int start_freq = 10700000; //in kHz
	int end_freq = 12750000; //in kHz
	int step_freq = 6000; //in kHz
	int search_range{10000}; //in kHz
	int max_symbol_rate{45000}; //in kHz
	int pol =3;
	std::string pls;
	std::vector<uint32_t> pls_codes = {
		//In use on 5.0W
		make_code(0, 16416),
		make_code(0, 8),
		make_code(1, 121212),
		make_code(1, 262140),
		make_code(1, 50416)
		};
	int start_pls_code{-1};
	int end_pls_code{-1};
	int adapter_no{0};
	int frontend_no{0};
	std::string diseqc{"UC"};
	int uncommitted{0};
	int committed{0};

	options_t() = default;
	void parse_pls(const std::vector<std::string>& pls_entries);
	int parse_options(int argc, char**argv);
};

options_t options;



void options_t::parse_pls(const std::vector<std::string>& pls_entries)
{
	const std::regex base_regex("(ROOT|GOLD|COMBO)\\+([0-9]{1,5})");
	std::smatch base_match;
	for(auto m: pls_entries) {
		int mode;
		int code;
		if (std::regex_match(m, base_match, base_regex)) {
			// The first sub_match is the whole string; the next
			// sub_match is the first parenthesized expression.
			if (base_match.size() >= 2) {
				std::ssub_match base_sub_match = base_match[1];
				auto mode_ = base_sub_match.str();
				if(!mode_.compare("ROOT"))
					mode=0;
				else if(!mode_.compare("GOLD"))
					mode=1;
				else if(!mode_.compare("COMBO"))
					mode=2;
				else {
					printf("mode=/%s/\n", mode_.c_str());
					throw std::runtime_error("Invalid PLS mode");
				}
			}
			if (base_match.size() >= 3) {
				std::ssub_match base_sub_match = base_match[2];
				auto code_ = base_sub_match.str();
				if(sscanf(code_.c_str(), "%d", &code)!=1)
					throw std::runtime_error("Invalid PLS code");
			}
			pls_codes.push_back(make_code(mode, code));
		}
		printf(" %d:%d", mode, code);
	}
	printf("\n");

}
int options_t::parse_options(int argc, char**argv)
{

	//Level level{Level::Low};
	CLI::App app{"Blind scanner for tbs cards"};
	std::map<std::string, int> pol_map{{"V", 2}, {"H", 1}, {"BOTH",3}};
	std::map<std::string, int> pls_map{{"ROOT", 0}, {"GOLD", 1}, {"COMBO", 1}};
	std::vector<std::string> pls_entries;

	app.add_option("-a,--adapter", adapter_no, "Adapter number", true);
	app.add_option("--frontend", frontend_no, "Frontend number", true);

	app.add_option("-s,--start-freq", start_freq, "Start of frequenc range to scan (kHz)", true);
		//->required();
	app.add_option("-e,--end-freq", end_freq, "End of frequency range to scan (kHz)", true);
	app.add_option("-S,--step-freq", step_freq, "Frequency step (kHz)", true);
	app.add_option("-M,--max-symbol-rate", max_symbol_rate, "Maximal symbolrate (kHz)", true);
	app.add_option("-R,--search-range", search_range, "search range (kHz)", true);

	app.add_option("-p,--pol", pol, "Polarisation to scan", true)
		->transform(CLI::CheckedTransformer(pol_map, CLI::ignore_case));

	app.add_option("--pls-modes", pls_entries, "PLS modes (ROOT, GOLD, COMBO) and code to scan, separated by +", true);
	app.add_option("--start-pls-code", start_pls_code, "Start of PLS code range to start (mode=ROOT!)", true);
	app.add_option("--end-pls-code", end_pls_code, "End of PLS code range to start (mode=ROOT!)", true);

	app.add_option("-d,--diseqc", diseqc, "diseqc command string (C: send committed command; "
								 "U: send uncommitted command", true);
	app.add_option("-U,--uncommitted", uncommitted,  "uncommitted switch number (lowest is 0)", true);
	app.add_option("-C,--committed", committed,  "committed switch number (lowest is 0)", true);

	try {
    app.parse(argc, argv);
	} catch (const CLI::ParseError &e) {
		app.exit(e);
		return -1;
	}
	parse_pls(pls_entries);
	printf("adapter=%d\n", adapter_no);
	printf("frontend=%d\n", frontend_no);

	printf("start-freq=%d\n", start_freq);
	printf("end-freq=%d\n", end_freq);
	printf("step-freq=%d\n", step_freq);

	printf("pol=%d\n", pol);

	printf("pls_codes[%ld]={ ", pls_codes.size());
	for(auto c: pls_codes)
		printf("%d, ",c);
	printf("}\n");

	printf("diseqc=%s: U=%d C=%d\n", diseqc.c_str(), uncommitted, committed);


	return 0;

	po::options_description desc("NeumoDVB receiver test program");
	try {
		desc.add_options()
			("help,h", "show usage")
			("start-pls-code", po::value(&start_pls_code), "Start of PLS code range to start (mode=ROOT!)")
			("end-pls-code", po::value(&end_pls_code), "End of PLS code range to start (mode=ROOT!)")
			("adapter,a", po::value(&adapter_no), "Adapter number")
			("frontend", po::value(&frontend_no), "frontend number")
			("diseqc,d", "diseqc command string (C: send committed command; U: send uncommitted command")
			("uncommitted,U", po::value(&uncommitted)->default_value(0), "uncommitted switch number (lowest is 0)")
			("committed,C", po::value(&committed)->default_value(0), "ccommitted switch number (lowest is 0)")
			;

		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc)
							.run(), vm);
		po::notify(vm);
		if (vm.count("help")) {
			std::cerr << desc << "\n";
			return -1;
		}

	} catch(std::exception& e) {
		std::cerr << e.what() << "\n";
		std::cerr << desc << "\n";
		return -1;
	}

	if(options.end_freq < options.start_freq)
		options.end_freq = options.start_freq+1; //scan single freq
	return 0;
}


static int epoll_timeout = 5000000; //in ms


//#pragma GCC diagnostic ignored "-Waddress-of-packed-member"


/** @brief Print the status
 * Print the status contained in festatus, this status says if the card is lock, sync etc.
 *
 * @param festatus the status to display
 */
void print_tuner_status(fe_status_t festatus) {
//printf("FE_STATUS:");
	if (festatus & FE_HAS_SIGNAL) printf("     FE_HAS_SIGNAL : found something above the noise level");
	if (festatus & FE_HAS_CARRIER) printf("     FE_HAS_CARRIER : found a DVB signal");
	if (festatus & FE_HAS_VITERBI) printf("     FE_HAS_VITERBI : FEC is stable");
	if (festatus & FE_HAS_SYNC) printf("     FE_HAS_SYNC : found sync bytes");
	if (festatus & FE_HAS_LOCK) printf("     FE_HAS_LOCK : everything's working...");
	if (festatus & FE_TIMEDOUT) printf("     FE_TIMEDOUT : no lock within the last about 2 seconds");
	if (festatus & FE_REINIT) printf("     FE_REINIT : frontend was reinitialized");
	printf("---");
/*
	fe_status & FE_HAS_LOCK : GOOD
	fe_status & (FE_HAS_SYNC | FE_HAS_VITERBI | FE_HAS_CARRIER)) : BAD
	fe_status & FE_HAS_SIGNAL : FAINT

*/

}

int check_lock_status(int fefd)
{
	fe_status_t status;
	while(ioctl(fefd, FE_READ_STATUS, &status) < 0){
		if (errno == EINTR) {
			continue;
		}
		printf("FE_READ_STATUS: %s", strerror(errno));
		return -1;
	}
	if(status & FE_TIMEDOUT) {

	}
	if(status & FE_HAS_LOCK) {
	}
	return status & FE_HAS_LOCK;
}




/** The structure for a diseqc command*/
struct diseqc_cmd {
	struct dvb_diseqc_master_cmd cmd;
	uint32_t wait;
};



/** @brief Wait msec miliseconds
 */
static inline void msleep(uint32_t msec)
{
	struct timespec req = { msec / 1000, 1000000 * (msec % 1000) };
	while (nanosleep(&req, &req));
}

int lo_frequency = 0;


#define FREQ_MULT 1000

#define CBAND_LOF 5150

std::tuple<int, int> getinfo(int fefd, int polarisation)
{
	ioctl(fefd, FE_READ_SIGNAL_STRENGTH, &signal);

	struct dtv_property p[] = {
		{ .cmd = DTV_DELIVERY_SYSTEM},  // 0 DVB-S, 9 DVB-S2
		{ .cmd = DTV_FREQUENCY },
		{ .cmd = DTV_VOLTAGE },         // 0 - 13V H, 1 - 18V V, 2 - Voltage OFF
		{ .cmd = DTV_SYMBOL_RATE },
		{ .cmd = DTV_STAT_SIGNAL_STRENGTH	},
		{ .cmd = DTV_STAT_CNR	},
		{ .cmd = DTV_MODULATION },      // 5 - QPSK, 6 - 8PSK
		{ .cmd = DTV_INNER_FEC },
		{ .cmd = DTV_INVERSION },
		{ .cmd = DTV_ROLLOFF },
		{ .cmd = DTV_PILOT },            // 0 - ON, 1 - OFF
		{ .cmd = DTV_TONE },
		{ .cmd = DTV_STREAM_ID },
		{ .cmd = DTV_SCRAMBLING_SEQUENCE_INDEX },
		{ .cmd = DTV_ISI_LIST },
		//		{ .cmd = DTV_BANDWIDTH_HZ },    // Not used for DVB-S
	};
	struct dtv_properties cmdseq = {
		.num = sizeof(p)/sizeof(p[0]),
		.props = p
	};
	ioctl(fefd, FE_GET_PROPERTY, &cmdseq);
	int i=0;
	int dtv_delivery_system_prop = cmdseq.props[i++].u.data;
	int dtv_frequency_prop = cmdseq.props[i++].u.data; //in kHz (DVB-S)  or in Hz (DVB-C and DVB-T)
	int dtv_voltage_prop = cmdseq.props[i++].u.data;
	int dtv_symbol_rate_prop = cmdseq.props[i++].u.data; //in Hz

	auto dtv_stat_signal_strength_prop = cmdseq.props[i++].u.st;
	auto dtv_stat_cnr_prop = cmdseq.props[i++].u.st;

	int dtv_modulation_prop = cmdseq.props[i++].u.data;
	int dtv_inner_fec_prop = cmdseq.props[i++].u.data;
	int dtv_inversion_prop = cmdseq.props[i++].u.data;
	int dtv_rolloff_prop = cmdseq.props[i++].u.data;
	int dtv_pilot_prop = cmdseq.props[i++].u.data;
#if 0
	int dtv_tone_prop = cmdseq.props[i++].u.data;
#else
	i++;
#endif
	int dtv_stream_id_prop = cmdseq.props[i++].u.data;
	int dtv_scrambling_sequence_index_prop = cmdseq.props[i++].u.data;

	int num_isi = cmdseq.props[i].u.buffer.len;
	uint8_t* isi_list = cmdseq.props[i++].u.buffer.data; //TODO: we can only return 32 out of 256 entries...

	assert(i== cmdseq.num);
//int dtv_bandwidth_hz_prop = cmdseq.props[12].u.data;
	int currentfreq;
	int currentpol;
	int currentsr;
	int currentsys;
	int currentfec;
	int currentmod;
	int currentinv;
	int currentrol;
	int currentpil;

	currentfreq = (dtv_frequency_prop  +(signed)lo_frequency);
	currentpol = dtv_voltage_prop;
	currentsr = dtv_symbol_rate_prop;
	currentsys = dtv_delivery_system_prop;
	currentfec = dtv_inner_fec_prop;
	currentmod = dtv_modulation_prop;
	currentinv = dtv_inversion_prop;
	currentrol = dtv_rolloff_prop;
	currentpil = dtv_pilot_prop;
	assert(currentpol == 1-polarisation);
	if (dtv_frequency_prop != 0)
		printf("RESULT: freq=%-8.3f%c ", currentfreq/ (double)FREQ_MULT, polarisation? 'V': 'H');
	else
		printf("RESULT: freq=%-8.3f ", dtv_frequency_prop/(double)FREQ_MULT);


	printf("Symrate=%-5d ", currentsr/FREQ_MULT);

	printf("Stream=%-5d pls_mode=%2d:%5d ", dtv_stream_id_prop&0xff,
				 (dtv_stream_id_prop>>26) & 0x3, (dtv_stream_id_prop>>8) & 0x3FFFF);
	if(num_isi>0)  {
		printf("ISI list:");
		for(int i=0; i< num_isi;++i) {
			printf(" %d", isi_list[i]);
		}
		printf("\n");
	}
	for(int i=0; i < dtv_stat_signal_strength_prop.len; ++i) {
		if (dtv_stat_signal_strength_prop.stat[i].scale== FE_SCALE_DECIBEL)
			printf("SIG=%4.2lfdB ", dtv_stat_signal_strength_prop.stat[i].svalue/1000.);
		else if (dtv_stat_signal_strength_prop.stat[i].scale== FE_SCALE_RELATIVE)
			printf("SIG=%3lld%% ", (dtv_stat_signal_strength_prop.stat[i].uvalue*100)/65535);
		else if (dtv_stat_signal_strength_prop.stat[i].scale== FE_SCALE_NOT_AVAILABLE)
			printf("SIG=%3lld?? ", (dtv_stat_signal_strength_prop.stat[i].uvalue*100)/65536);
	}

	for(int i=0; i < dtv_stat_cnr_prop.len; ++i) {
		if (dtv_stat_cnr_prop.stat[i].scale== FE_SCALE_DECIBEL)
			printf("CNR=%4.2lfdB ", dtv_stat_cnr_prop.stat[i].svalue/1000.);
		else if (dtv_stat_cnr_prop.stat[i].scale== FE_SCALE_RELATIVE)
			printf("CNR=%3lld%% ", (dtv_stat_cnr_prop.stat[i].uvalue*100)/65535);
		else if (dtv_stat_cnr_prop.stat[i].scale== FE_SCALE_NOT_AVAILABLE)
			printf("CNR=%3lld?? ", (dtv_stat_cnr_prop.stat[i].uvalue*100)/65537);
	}


	switch (dtv_delivery_system_prop) {
	case 4:  printf("DSS    ");  break;
	case 5:  printf("DVB-S  ");  break;
	case 6:  printf("DVB-S2 "); break;
	default: printf("SYS(%d) ", dtv_delivery_system_prop); break;
	}

	switch (dtv_modulation_prop) {
	case 0: printf("QPSK "); break;
	case 9: printf("8PSK "); break;
	default: printf("MOD(%d) ", dtv_modulation_prop); break;
	}

	switch (dtv_inner_fec_prop) {
	case 0: printf("FEC_NONE ");  break;
	case 1: printf("FEC_1_2  ");   break;
	case 2: printf("FEC_2_3  ");   break;
	case 3: printf("FEC_3_4  ");   break;
	case 4: printf("FEC_4_5  ");   break;
	case 5: printf("FEC_5_6  ");   break;
	case 6: printf("FEC_6_7  ");   break;
	case 7: printf("FEC_7_8  ");   break;
	case 8: printf("FEC_8_9  ");   break;
	case 9: printf("FEC_AUTO ");  break;
	case 10: printf("FEC_3_5  ");  break;
	case 11: printf("FEC_9_10 "); break;
	default: printf("FEC (%d)  ", dtv_inner_fec_prop); break;
	}

	switch (dtv_inversion_prop) {
	case 0:  printf("INV_OFF ");  break;
	case 1:  printf("INV_ON  ");   break;
	case 2:  printf("INVAUTO "); break;
	default: printf("INV (%d) ", dtv_inversion_prop); break;
	}


	switch (dtv_pilot_prop) {
	case 0:  printf("PIL_ON  ");   break;
	case 1:  printf("PIL_OFF ");  break;
	case 2:  printf("PILAUTO "); break;
	default: printf("PIL (%d) ", dtv_pilot_prop); break;
	}

	switch (dtv_rolloff_prop) {
	case 0:  printf("ROL_35\n");   break;
	case 1:  printf("ROL_20\n");   break;
	case 2:  printf("ROL_25\n");   break;
	case 3:  printf("ROL_AUTO\n"); break;
	default: printf("ROL (%d)\n", dtv_rolloff_prop); break;
	}

	return std::make_tuple(currentfreq, (135*(currentsr/FREQ_MULT)) / (2 *100));
}


void close_frontend(int fefd);

int get_frontend_info(int fefd)
{
	struct dvb_frontend_info fe_info{}; //front_end_info
	//auto now =time(NULL);
	//This does not produce anything useful. Driver would have to be adapted

	int res;
	if ( (res = ioctl(fefd, FE_GET_INFO, &fe_info) < 0)){
		printf("FE_GET_INFO: %s", strerror(errno));
		close_frontend(fefd);
		return -1;
	}

	/*fe_info.frequency_min
		fe_info.frequency_max
		fe_info.symbolrate_min
		fe_info.symbolrate_max
		fe_info.caps:
	*/


	struct dtv_property properties[16];
	memset(properties, 0, sizeof(properties));
	unsigned int i=0;
	properties[i++].cmd      = DTV_ENUM_DELSYS;
	properties[i++].cmd      = DTV_DELIVERY_SYSTEM;
	struct dtv_properties props ={
		.num=i,
		.props = properties
	};

	if ((ioctl(fefd, FE_GET_PROPERTY, &props)) == -1) {
		printf("FE_GET_PROPERTY failed: %s", strerror(errno));
		//set_interrupted(ERROR_TUNE<<8);
		close_frontend(fefd);
		return -1;
	}

	//auto current_fe_type = chdb::linuxdvb_fe_delsys_to_type (fe_info.type);
	//auto& supported_delsys = properties[0].u.buffer.data;
//	int num_delsys =  properties[0].u.buffer.len;
#if 0
	auto tst =dump_caps((chdb::fe_caps_t)fe_info.caps);
	printf("CAPS: %s", tst);
	fe.delsys.resize(num_delsys);
	for(int i=0 ; i<num_delsys; ++i) {
		auto delsys = (chdb::fe_delsys_t) supported_delsys[i];
		//auto fe_type = chdb::delsys_to_type (delsys);
		auto* s = enum_to_str(delsys);
		printf("delsys[" << i << "]=" << s);
		changed |= (i >= fe.delsys.size() || fe.delsys[i].fe_type!= delsys);
		fe.delsys[i].fe_type = delsys;
	}
#endif
	return 0;
}

int open_frontend(const char* frontend_fname)
{
	const bool rw=true;
	int rw_flag = rw?  O_RDWR : O_RDONLY;
	int fefd = open(frontend_fname, rw_flag | O_NONBLOCK);
	if (fefd < 0) {
		printf("open_frontend failed: %s\n", strerror(errno));
		return -1;
	}
	return fefd;
}

void close_frontend(int fefd)
{
	if(fefd<0)
		return;
	if (::close(fefd) < 0) {
		printf("close_frontend failed: %s: ", strerror(errno));
		return;
	}
}



int clear(int fefd)
{

	struct dtv_property pclear[] = {
		{ .cmd = DTV_CLEAR,},		//RESET frontend's cached data

	};
	struct dtv_properties cmdclear = {
		.num = 1,
		.props = pclear
	};
	if ((ioctl(fefd, FE_SET_PROPERTY, &cmdclear)) == -1) {
		printf("FE_SET_PROPERTY clear failed: %s\n", strerror(errno));
		//set_interrupted(ERROR_TUNE<<8);
		return -1;
	}
	return 0;
}



int tune_it(int fefd, int frequency_, int polarisation, int pls_mode, int pls_code);
int do_lnb_and_diseqc(int fefd, int frequency, int polarisation);

int tune(int fefd, int frequency, int polarisation, int pls_mode, int pls_code)
{
	printf("Tuning to DVBS %.3f%c\n", frequency/1000., polarisation? 'V': 'H');
	if(clear(fefd)<0)
		return -1;

	do_lnb_and_diseqc(fefd, frequency, polarisation);
	return tune_it(fefd, frequency, polarisation, pls_mode, pls_code);
}

//static constexpr uint32_t lnb_lof_standard =DEFAULT_LOF_STANDARD;
static constexpr uint32_t  lnb_slof = 11700*1000UL;
static constexpr uint32_t lnb_lof_low = 9750*1000UL;
static constexpr uint32_t lnb_lof_high = 10600*1000UL;


uint32_t get_lo_frequency(uint32_t frequency)		{
	if (frequency < lnb_slof) {
		return  lnb_lof_low;
	} else {
		return lnb_lof_high;
	}
}



struct cmdseq_t {
	struct dtv_properties cmdseq{};
	std::array<struct dtv_property, 16> props;

	cmdseq_t() {
		cmdseq.props = & props[0];
	}
	template<typename T>
	void add (int cmd, T data) {
		assert(cmdseq.num < props.size()-1);
		memset(&cmdseq.props[cmdseq.num], 0, sizeof(cmdseq.props[cmdseq.num]));
		cmdseq.props[cmdseq.num].cmd = cmd;
		cmdseq.props[cmdseq.num].u.data = (int)data;
		cmdseq.num++;
	};

	void add_pls_codes(int cmd, uint32_t* codes, int num_codes) {
		printf("adding pls_codes\n");
		assert(cmdseq.num< props.size()-1);
		auto* tvp = &cmdseq.props[cmdseq.num];
		memset(tvp, 0, sizeof(cmdseq.props[cmdseq.num]));
		tvp->cmd = cmd;
		num_codes =std::min(num_codes,  (int)(sizeof(tvp->u.buffer.data)/sizeof(uint32_t)));
		printf("adding %d scramble codes\n", num_codes);
		for(int i=0; i< num_codes; ++i)
			memcpy(&tvp->u.buffer.data[i*sizeof(uint32_t)], &codes[i], sizeof(codes[0]));
		tvp->u.buffer.len = num_codes*sizeof(uint32_t);
		cmdseq.num++;
	};

	void add_pls_range(int cmd, uint32_t pls_start, uint32_t pls_end) {
		assert(cmdseq.num< props.size()-1);
		auto* tvp = &cmdseq.props[cmdseq.num];
		memset(tvp, 0, sizeof(cmdseq.props[cmdseq.num]));
		tvp->cmd = cmd;
		printf("adding scramble code range:%d-%d\n", pls_start, pls_end);
		memcpy(&tvp->u.buffer.data[0*sizeof(uint32_t)], &pls_start, sizeof(pls_start));
		memcpy(&tvp->u.buffer.data[1*sizeof(uint32_t)], &pls_end, sizeof(pls_end));
		tvp->u.buffer.len = 2*sizeof(uint32_t);
		cmdseq.num++;
	};

	int tune(int fefd, bool dotune=true) {
		if(dotune)
			add(DTV_TUNE, 0);
		if ((ioctl(fefd, FE_SET_PROPERTY, &cmdseq)) == -1) {
			printf("FE_SET_PROPERTY failed: %s\n", strerror(errno));
			return -1;
		}
		return 0;
	}
};


/*
	pls_mode>=0 means that only this pls will be scanned (no unscrambled transponders)
	Usually it is better to use pls_modes; these will be used in addition to unscrambled
 */
int tune_it(int fefd, int frequency_, int polarisation, int pls_mode, int pls_code)
{
	cmdseq_t cmdseq;

	lo_frequency = get_lo_frequency(frequency_);
	auto frequency= (long)(frequency_-(signed)lo_frequency);
	printf("BLIND SCAN search-range=%d\n", options.search_range);
	cmdseq.add(DTV_DELIVERY_SYSTEM,  (int) SYS_AUTO);
	assert(polarisation==0 || polarisation==1);
	cmdseq.add(DTV_VOLTAGE,  1-polarisation);
	cmdseq.add(DTV_ALGORITHM,  ALGORITHM_BLIND);
	cmdseq.add(DTV_SEARCH_RANGE,  options.search_range*1000); //how far carrier may shift
	cmdseq.add(DTV_SYMBOL_RATE,  options.max_symbol_rate*1000); //controls tuner bandwidth
	//cmdseq.add(DTV_DELIVERY_SYSTEM,  SYS_DVBS2);
	cmdseq.add(DTV_FREQUENCY,  frequency); //For satellite delivery systems, it is measured in kHz.

	if(options.pls_codes.size()>0)
		cmdseq.add_pls_codes(DTV_PLS_SEARCH_LIST, &options.pls_codes[0], options.pls_codes.size());

	if(options.end_pls_code> options.start_pls_code)
		cmdseq.add_pls_range(DTV_PLS_SEARCH_RANGE, options.start_pls_code, options.end_pls_code);



	int stream_id = pls_mode < 0  ? -1 :
		(stream_id&0xff) | ((pls_code & 0x3FFFF)<<8) | (((pls_mode) & 0x3)<<26);
	cmdseq.add(DTV_STREAM_ID,  stream_id);


	return cmdseq.tune(fefd);
}

int tune_next(int fefd)
{
	cmdseq_t cmdseq;

	printf("NEXT SCAN\n");
	cmdseq.add(DTV_ALGORITHM,  ALGORITHM_SEARCH_NEXT);
	//cmdseq.add(DTV_DELIVERY_SYSTEM,  SYS_DVBS);
	return cmdseq.tune(fefd);
}









/** @brief generate and diseqc message for a committed or uncommitted switch
 * specification is available from http://www.eutelsat.com/
 * @param extra: extra bits to set polarisation and band; not sure if this does anything useful
 */
int send_diseqc_message(int fefd, char switch_type, unsigned char port, unsigned char extra, bool repeated)
{

	struct dvb_diseqc_master_cmd cmd{};
	//Framing byte : Command from master, no reply required, first transmission : 0xe0
	cmd.msg[0] = repeated ? 0xe1: 0xe0;
	//Address byte : Any LNB, switcher or SMATV
	cmd.msg[1] = 0x10;
	//Command byte : Write to port group 1 (Uncommited switches)
	//Command byte : Write to port group 0 (Committed switches) 0x38
	if( switch_type == 'U' )
		cmd.msg[2] = 0x39;
	else if (switch_type == 'C')
		cmd.msg[2] = 0x38;
	else if (switch_type == 'X' ) {
		cmd.msg[2] = 0x6B; // positioner goto
		return 0;
	}
	/* param: high nibble: reset bits, low nibble set bits,
	 * bits are: option, position, polarisation, band */
	cmd.msg[3] =
		0xf0 | (port & 0x0f) | extra;

	//
	cmd.msg[4] = 0x00;
	cmd.msg[5] = 0x00;
	cmd.msg_len=4;

	int err;
	if((err = ioctl(fefd, FE_DISEQC_SEND_MASTER_CMD, &cmd)))
	{
		printf("problem sending the DiseqC message\n");
		return -1;
	}
	return 0;
}

int hi_lo(unsigned int frequency)
{
	return (frequency >= lnb_slof);

}



bool tone_change_needed()
{
	return true;
}

bool voltage_change_needed()
{
	return true;
}

bool diseqc10_change_needed()
{
#ifdef TODO
	return last_confirmed_lnb_valid  ?
		(current_lnb.r.diseqc10 != last_confirmed_lnb.r.diseqc10) : true;
#else
	return true;
#endif
}


bool mini_diseqc_change_needed()
{
#ifdef TODO
	return last_confirmed_lnb_valid  ?
		(current_lnb.r.mini_diseqc != last_confirmed_lnb.r.mini_diseqc) : true;
#else
	return true;
#endif
}

bool diseqc11_change_needed()
{
#ifdef TODO
	return last_confirmed_lnb_valid  ?
		(current_lnb.r.diseqc11 != last_confirmed_lnb.r.diseqc11) : true;
#else
	return true;
#endif
}

bool diseqc12_change_needed()
{
#ifdef TODO
	return last_confirmed_lnb_valid  ?
		(current_lnb.r.positioner_type != last_confirmed_lnb.r.positioner_type ||
		 current_lnb.k.sat.position != last_confirmed_lnb.k.sat.position
			) : true;
#else
	return true;
#endif
}

bool diseqc13_change_needed()
{
#ifdef TODO
	return last_confirmed_lnb_valid  ?
		(current_lnb.r.positioner_type != last_confirmed_lnb.r.positioner_type ||
		 current_lnb.sat_pos != last_confirmed_lnb.sat_pos
			) : true;
#else
	return true;
#endif
}


/** @brief Send a diseqc message and also control band/polarisation in the right order*
		DiSEqC 1.0, which allows switching between up to 4 satellite sources
		DiSEqC 1.1, which allows switching between up to 16 sources
		DiSEqC 1.2, which allows switching between up to 16 sources, and control of a single axis satellite motor
		DiSEqC 1.3, Usals
		DiSEqC 2.0, which adds bi-directional communications to DiSEqC 1.0
		DiSEqC 2.1, which adds bi-directional communications to DiSEqC 1.1
		DiSEqC 2.2, which adds bi-directional communications to DiSEqC 1.2

		Diseqc string:
		M = mini_diseqc
		C = committed   = 1.0
		U = uncommitted = 1.1
		X = goto position = 1.2
		P = positoner  = 1.3 = usals
		" "= 50 ms pause

		Returns <0 on error, 0 of no diseqc command was sent, 1 if at least 1 diseqc command was sent


*/
int diseqc(int fefd)
{
/*
	turn off tone to not interfere with diseqc
*/
	bool tone_off_called=false;
	auto tone_off = [&]() {
		int err;
		if(tone_off_called)
			return 0;
		tone_off_called = true;
		if((err = ioctl(fefd, FE_SET_TONE, SEC_TONE_OFF))) {
			printf("problem Setting the Tone OFF");
			return -1;
		}
		return 1;
	};

	int ret;
	bool must_pause= false; //do we need a long pause before the next diseqc command?
	int diseqc_num_repeats = 2;
	for(int repeated=0; repeated <= diseqc_num_repeats; ++repeated) {

		for(const char& command: options.diseqc) {
			switch(command) {
			case 'M': {
				if(! mini_diseqc_change_needed())
					continue;

				if(tone_off()<0)
					return -1;
				msleep(must_pause ? 100: 15);
				/*
					tone burst commands deal with simpler equipment.
					They use a 12.5 ms duration 22kHz burst for transmitting a 1
					and 9 shorter bursts within a 12.5 ms interval for a 0
					for an on signal.
					They allow swithcing between two satelites only
				*/

				must_pause = ! repeated;
			}
				break;
			case 'C': {
				if(! diseqc10_change_needed())
					continue;
				//committed
					if(tone_off()<0)
						return -1;
					msleep(must_pause ? 100: 15);
					ret = send_diseqc_message(fefd, 'C', options.committed, 0,  repeated);
					if(ret<0) {
						printf("Sending Committed DiseqC message failed");
					}
					must_pause = ! repeated;
			}
				break;
			case 'U': {
				//uncommitted
				if(! diseqc11_change_needed())
					continue;
				if(tone_off()<0)
					return -1;

				msleep(must_pause ? 100: 15);
				ret=send_diseqc_message(fefd, 'U', options.uncommitted, 0, repeated);
				if(ret<0) {
					printf("Sending Uncommitted DiseqC message failed");
				}
				must_pause = ! repeated;
			}
				break;
			case ' ': {
				msleep(50);
				must_pause =false;
			}
				break;
			}
			if(ret<0)
				return ret;
		}
	}
	return tone_off_called ? 1 : 0;

}


int do_lnb_and_diseqc(int fefd, int frequency, int polarisation)
{

	/*TODO: compute a new diseqc_command string based on
		last tuned lnb, such that needless switching is avoided
		This needs:
		-after successful tuning: old_lnb... needs to be stored
		-after unsuccessful tuning, second attempt should use full diseqc
	*/
	int ret;
	/*

		22KHz: off = low band; on = high band
		13V = vertical or right-hand  18V = horizontal or low-hand
		TODO: change this to 18 Volt when using positioner
	*/
	if(voltage_change_needed()) {;
		int pol_v_r = ((int)polarisation &1);
		fe_sec_voltage_t lnb_voltage = pol_v_r ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18;
		if((ret = ioctl(fefd, FE_SET_VOLTAGE, lnb_voltage))) {
			printf("problem Setting the Voltage\n");
			return -1;
		}
	}

	//TODO: diseqc_command_string should be read from lnb

	//Note: the following is a NOOP in case no diseqc needs to be sent
	ret= diseqc(fefd);
	if(ret<0)
		return ret;

	bool tone_turned_off = ret>0;

	/*select the proper lnb band
		22KHz: off = low band; on = high band
	*/
	if(tone_turned_off || tone_change_needed()) {
		fe_sec_tone_mode_t tone =	hi_lo(frequency) ? SEC_TONE_ON : SEC_TONE_OFF;
		ret = ioctl(fefd, FE_SET_TONE, tone);
		if(ret < 0) {
			printf("problem Setting the Tone back\n");
			return -1;
		}
	}
	return 0;
}






uint32_t scan_freq(int fefd, int efd,
									 int frequency, int polarisation,
									 int pls_mode, int pls_code)
{
	int ret=0;
	printf("==========================\n");
	printf("SEARCH: %.3f-%.3f\n", (frequency-options.search_range/2)/1000., (frequency+options.search_range/2)/1000.);

	while(1)  {
		struct dvb_frontend_event event{};
		if (ioctl(fefd, FE_GET_EVENT, &event)<0)
			break;
	}

	ret = tune(fefd, frequency, polarisation, pls_mode, pls_code);
	if(ret!=0) {
		printf("Tune FAILED\n");
		exit(1);
	}

	struct dvb_frontend_event event{};
	bool timedout=false;
	bool locked=false;
	int count=0;
	while(count<3 && !timedout && !locked) {
		struct epoll_event events[1]{{}};
		auto s = epoll_wait(efd, events, 1, epoll_timeout);
		if(s<0)
			printf("\tEPOLL failed: err=%s\n", strerror(errno));
		if(s==0) {
			//printf("TIMEOUT\n");
			auto old = frequency;
			printf("\tTIMEOUT freq: old=%.3f new=%.3f\n", old/(float)FREQ_MULT, frequency/(float)FREQ_MULT);
			timedout=true;
			break;
		}
		int r = ioctl(fefd, FE_GET_EVENT, &event);
		if(r<0)
			printf("\tFE_GET_EVENT stat=%d err=%s\n", event.status, strerror(errno));
		else {
			timedout = event.status & FE_TIMEDOUT;
			locked = event.status & FE_HAS_VITERBI;
			if(count>=1)
				printf("\tFE_GET_EVENT: stat=%d, timedout=%d locked=%d\n", event.status, timedout, locked);
			count++;
		}
	}

	if(timedout)
		return frequency + options.step_freq;

	if(check_lock_status(fefd)) {
		auto old = frequency;
		auto [found_freq, bw2] = getinfo(fefd, polarisation);
		frequency = found_freq + bw2;
		frequency += options.search_range/2;
	} else
		printf("\tnot locked\n");
	//printf("-------------------------------------------------\n");
	return frequency;
}




int main_blindscan(int fefd)
{
	int uncommitted = 0;
	std::string diseqc_command{"U"};

	clear(fefd);
	int efd = epoll_create1 (0);	//create an epoll instance

	struct epoll_event ep;
	memset(&ep, 0, sizeof(ep));

	ep.data.fd = fefd; //user data
	ep.events = EPOLLIN|EPOLLERR|EPOLLHUP|EPOLLET; //edge triggered!
	int s = epoll_ctl(efd, EPOLL_CTL_ADD, fefd, &ep);
	if(s<0)
		printf("EPOLL Failed: err=%s\n", strerror(errno));
	assert(s==0);\
	for(int polarisation=0; polarisation<2; ++polarisation) {
		//0=H 1=V
		if(!((1<<polarisation) & options.pol))
			continue; //this pol not needed
		for(auto frequency=options.start_freq; frequency < options.end_freq;) {
			const int pls_mode = -1;
			const int pls_code = -1;
			auto new_frequency = scan_freq(fefd, efd, frequency, polarisation, pls_mode, pls_code);
			frequency = new_frequency> frequency ? new_frequency: frequency + options.step_freq;
		}
	}
	return 0;
}


enum class Level : int { High, Medium, Low };

int main(int argc, char**argv)
{
	if(options.parse_options(argc, argv)<0)
		return -1;

	char dev[512];
	sprintf(dev,"/dev/dvb/adapter%d/frontend%d", options.adapter_no, options.frontend_no);
	int fefd = open_frontend(dev);
	if(fefd<0) {
		exit(1);
	}
	int ret=0;
	ret |= main_blindscan(fefd);
	return ret;
}
/*pol: 0=vertical=13volt 1=horizontal=18 volt
5.0W 45Min. 28.2E 15 min (no pls codes)
ebspro: 10min op 5.0W
*/