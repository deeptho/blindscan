/*
 * Neumo dvb (C) 2019-2025 deeptho@gmail.com
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
#include "CLI/CLI.hpp"
#include "neumofrontend.h"
#include "neumodmx.h"
#include "util.h"
#include <algorithm>
#include <format>
#include <cassert>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <linux/dvb/version.h>
#include <linux/limits.h>
#include <pthread.h>
#include <regex>
#include <resolv.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <values.h>

struct options_t {
	std::vector<int> pids = {};
	std::string filename_pattern{"/tmp/%s_a%d_%.3f%c.dat"};
	int adapter_no{0};
	int demux_no{0};
	bool fe_stream{false};
	int stid_pid = -1;
	int stid_isi = -1;
	int t2mi_pid{-1};
	int32_t t2mi_plp{T2MI_UNSPECIFIED_PLP}; //

	options_t() = default;
	int parse_options(int argc, char** argv);
};

options_t options;

int options_t::parse_options(int argc, char** argv) {
	CLI::App app{
		"Stream dvb data from a demux to standard output"
		, "DVB demux program"};

	app.add_option("-a,--adapter", adapter_no, "Adapter number", true);
	app.add_option("-d,--demux", demux_no, "Demux number", true);
	app.add_flag("--fe-stream", fe_stream, "directly address the frontend");
	app.add_option("--stid-pid", stid_pid, "pid in which stid bbframes are embedded", true);
	app.add_option("--stid-isi", stid_isi, "stid isi", true);
	app.add_option("--t2mi-pid", t2mi_pid, "pid in+ which t2mi stream is embedded", true);
	app.add_option("--t2mi-plp", t2mi_plp, "t2mi isi to extract", true);
	app.add_option("--pid", pids, "pid (omit for full transport stream)", true);

	try {
		app.parse(argc, argv);
	} catch (const CLI::ParseError& e) {
		app.exit(e);
		return -1;
	}
#if 0
	dtdebugf(sterr, "adapter=%d\n", adapter_no);
	dtdebugf(stderr, "demux=%d\n", demux_no);
	dtdebugf(stderr, "fe_stream=%d\n", fe_stream);
#endif
	return 0;
}

/** @brief Wait msec miliseconds
 */
static inline void msleep(uint32_t msec) {
	struct timespec req = {msec / 1000, 1000000 * (msec % 1000)};
	while (nanosleep(&req, &req))
		;
}


void close_demux(int demuxfd);

int get_extended_frontend_info(int demuxfd) {
	struct dvb_frontend_extended_info fe_info {}; // front_end_info
	// auto now =time(NULL);
	// This does not produce anything useful. Driver would have to be adapted

	int res;
	if ((res = ioctl(demuxfd, FE_GET_EXTENDED_INFO, &fe_info) < 0)) {
		printf("FE_GET_EXTENDED_INFO failed: blindscan drivers probably not installed\n");
		close_demux(demuxfd);
		return -1;
	}
	printf("Name of card: %s\n", fe_info.card_name);
	printf("Name of adapter: %s\n", fe_info.adapter_name);
	printf("Name of frontend: %s\n", fe_info.card_name);
	/*fe_info.frequency_min
		fe_info.frequency_max
		fe_info.symbolrate_min
		fe_info.symbolrate_max
		fe_info.caps:
	*/

	struct dtv_property properties[16];
	memset(properties, 0, sizeof(properties));
	unsigned int i = 0;
	properties[i++].cmd = DTV_ENUM_DELSYS;
	properties[i++].cmd = DTV_DELIVERY_SYSTEM;
	struct dtv_properties props = {.num = i, .props = properties};

	if ((ioctl(demuxfd, FE_GET_PROPERTY, &props)) == -1) {
		printf("FE_GET_PROPERTY failed: %s", strerror(errno));
		close_demux(demuxfd);
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


int open_demux(const char* dmx_fname) {
	const bool rw = true;
	int fefd = open(dmx_fname, O_RDONLY|O_NONBLOCK);
	if (fefd < 0) {
		printf("open_demux failed: %s\n", strerror(errno));
		return -1;
	}
	return fefd;
}

void close_demux(int demuxfd) {
	if (demuxfd < 0)
		return;
	if (::close(demuxfd) < 0) {
		printf("close_frontend failed: %s: ", strerror(errno));
		return;
	}
}

char buffer[1024*1024*128];
ssize_t bufsize{sizeof(buffer)};

int dmx_set_pes_filter(int demuxfd, int pid) {
	struct dmx_pes_filter_params pars;
	memset(&pars,0,sizeof(pars));
	pars.pid = pid;
	pars.input = DMX_IN_FRONTEND;
	pars.output = DMX_OUT_TSDEMUX_TAP;//DMX_OUT_TS_TAP;
	pars.pes_type = DMX_PES_OTHER;
	pars.flags = 0; //DMX_IMMEDIATE_START;
	dtdebugf("PES: Adding pid={}\n", pid);
	if (ioctl(demuxfd, DMX_SET_PES_FILTER, &pars) < 0) {
		dterrorf("DMX_SET_PES_FILTER  pid={} failed: {}", pid, strerror(errno));
		return -1;
	}
	return 0;
}

int dmx_add_pid(int demuxfd, int pid) {
	struct dmx_pes_filter_params pars;
	dtdebugf("PES: Adding pid={}\n", pid);
	if (ioctl(demuxfd, DMX_ADD_PID, &pid) < 0) {
		dterrorf("DMX_ADD_PID  pid={} failed: {}", pid, strerror(errno));
		return -1;
	}
	return 0;
}

int dmx_set_fe_stream(int demuxfd) {
	dtdebugf("set fe stream\n");
	if (ioctl(demuxfd, DMX_SET_FE_STREAM) < 0) {
		dterrorf("DMX_SET_FE_STREAM failed: {}", strerror(errno));
		return -1;
	}
	return 0;
}

int dmx_set_stid_stream(int demuxfd, int stid_pid, int stid_isi) {
	struct dmx_stid_stream_params pars;
	memset(&pars,0,sizeof(pars));
	pars.embedding_pid = stid_pid;
	pars.isi = stid_isi;
	dtdebugf("STID: Adding pid=0x{:x}\n", stid_pid);
	if (ioctl(demuxfd, DMX_SET_STID_STREAM, &pars) < 0) {
		dterrorf("DMX_SET_STID_STREAM  pid={} isi={} failed: {}", stid_pid, stid_isi,
						 strerror(errno));
		return -1;
	}
	return 0;
}

int dmx_set_t2mi_stream(int demuxfd, int t2mi_pid, int t2mi_plp) {
	struct dmx_t2mi_stream_params pars;
	memset(&pars,0,sizeof(pars));
	pars.embedding_pid = t2mi_pid;
	pars.plp = t2mi_plp; //not that drivers currently do not support setting this
	dtdebugf("T2MI: Adding pid=0x{:x}\n", t2mi_pid);
	if (ioctl(demuxfd, DMX_SET_T2MI_STREAM, &pars) < 0) {
		dterrorf("DMX_SET_T2MI_STREAM  pid={} plp={} failed: {}", t2mi_pid, t2mi_plp,
						 strerror(errno));
		return -1;
	}
	return 0;
}

int main_dmx(int demuxfd) {
	int ret;
	epoll_t epx;
	constexpr int max_events{8};
	int timeout_ms{1000};
	std::array<struct epoll_event, max_events> events;
	epx.add_fd(demuxfd, EPOLLIN|EPOLLERR|EPOLLHUP);
	int dmx_buffer_size = 32 * 1024 * 1024;
	if(ioctl(demuxfd, DMX_SET_BUFFER_SIZE, dmx_buffer_size)) {
		dterrorf("DMX_SET_BUFFER_SIZE failed: {}", strerror(errno));
	}
	if (options.fe_stream) {
		ret = dmx_set_fe_stream(demuxfd);
		if(ret<0)
			return ret;
		}
	if (options.stid_pid >= 0) {
		ret = dmx_set_stid_stream(demuxfd, options.stid_pid, options.stid_isi);
		if(ret<0)
			return ret;
		}
	if (options.t2mi_pid >= 0) {
		ret = dmx_set_t2mi_stream(demuxfd, options.t2mi_pid, options.t2mi_plp);
		if(ret<0)
			return ret;
	}
	if (options.pids.size()==0)
		ret = dmx_set_pes_filter(demuxfd, 0x2000);
	else {
		ret = dmx_set_pes_filter(demuxfd, options.pids[0]);
		for(auto it = options.pids.begin()+1; it != options.pids.end(); ++it) {
			auto pid = *it;
			printf("adding pid=%d\n", pid);
			ret = dmx_add_pid(demuxfd, pid);
			if(ret<0)
				return ret;
		}
	}
	if(ret<0)
		return ret;
	if(ioctl (demuxfd, DMX_START)<0) {
		dterrorf("DMX_START FAILED: {}", strerror(errno));
	}
	bool must_exit {false};
	for(;!must_exit;) {
		epx.wait(&events[0], max_events, timeout_ms);
		ssize_t ret = 1;
		for(;; ) {
			ret = ::read(demuxfd, buffer, bufsize);
			if (ret < 0) {
				if (errno == EINTR) {
					continue;
				} else  if (errno == EOVERFLOW) {
					dterrorf("OVERFLOW");
					continue;
				} else  if (errno == EAGAIN || errno == EWOULDBLOCK) {
					break; // no more data
				} else {
					dterrorf("error while reading: {}", strerror(errno));
					must_exit = true;
					break;
				}
			}
			if(ret>0) {
				::write(fileno(stdout), buffer, ret);
				//dtdebugf("Wrote {} bytes\n", ret);
			}
		}
	}
	return 0;
}

int main(int argc, char** argv) {
	bool has_blindscan{false};
	if (options.parse_options(argc, argv) < 0)
		return -1;
	if(std::filesystem::exists("/sys/module/dvb_core/info/version")) {
		printf("Blindscan drivers found\n");
	} else {
		printf("Blindscan drivers NOT found\n");
		assert(0);
	}

	char dev[512];
	sprintf(dev, "/dev/dvb/adapter%d/demux%d", options.adapter_no, options.demux_no);
	int demuxfd = open_demux(dev);
	if (demuxfd < 0) {
		exit(1);
	}
	auto ret = main_dmx(demuxfd);
	return ret;
}



/*

 1. Optionally select a stream for internal decoding
    DMX_SET_BBFRAMES_STREAM: reads a single pid and transforms it into a transport stream
		                         (which is emdedded in the pid according to an stid-specific format)

    DMX_SET_T2MI_STREAM: reads a single pid and transforms it into a transport stream
		                         (which is emdedded in the pid according to t2mi)

		DMX_SET_GSE_STREAM: future expansion

		The end result is always a transportstream

 2. Now repeat 2 if desired. e.g. RAI mux 5 on 12606V:
       DMX_SET_PES_GFILTER (to define where output goes. DMX_SET_FILTER is not supported (too complex)
			 DMX_SET_BBFRAMES_STREAM to extract stream 5 into a 2nd level transport stream
			 DMX_SET_T2MI_STREAM to extract a t2mi stream into a 3d level transport stream

 3. Select desired output:
    DMX_SET_FILTER: sections
		DMX_SET_PES_FILTER: a transport stream (possibly containing bbframes)

		Legacy code requires also specifying a PID, which is confusing.
		Best create a new ioctl

 4. Add additional pids desired as output using DMX_ADD_PID
    We could make a new ioctl that combines several of these calls


   Instead of the DMX_SET_FILTER, we could add an output format to ADD_PID, which can
	 e.g., extract sections or raw bbframes

 */
