/*
 * Neumo dvb (C) 2019-2024 deeptho@gmail.com
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
//#include "util/dtassert.h"
//#include <dlfcn.h>
#include <errno.h>
#include <assert.h>
#include <filesystem>
#include <signal.h>
#include <string.h>
#include <string>
#include <atomic>
#include "util.h"

void epoll_t::init() {
	static std::atomic<uint32_t> next_handle{0};
	handle= next_handle.fetch_add(1);
	_fd = epoll_create1(0); // create an epoll instance
	if (_fd < 0) {
		dterrorf("Could not create epoll fd\n");
	}
}

/*
	Not  checking the return value of close() is a common but nevertheless serious programming error.  It is quite
	possible that errors on a previous write(2) operation are first reported at the final close().   Not  checking
	the  return value when closing the file may lead to silent loss of data.  This can especially be observed with
	NFS and with disk quota.  Note that the return value should be  used  only  for  diagnostics.   In  particular
	close()  should  not be retried after an EINTR since this may cause a reused descriptor from another thread to
	be closed.
*/
static int force_close(int fd) {
	if (fd < 0)
		return -1;
	for (;;) {
		int ret = close(fd);
		if (ret == 0)
			return ret;
		else if (ret < 0 && errno != EINTR) {
			dterrorf("Error while closing fd={}: {}", fd, strerror(errno));
			return ret;
		}
	}
}

int epoll_t::close() {
	int ret = -1;
	if (_fd >= 0)
		ret = ::force_close(_fd);
	_fd = -1;
	return ret;
}

int epoll_t::add_fd(int fd, int mask) {
#ifdef DTDEBUG
	auto caller = gettid();
	if (owner == (pid_t)-1)
		owner = caller;
	assert(caller == owner); // needs to be called from owning thread
#endif
	struct epoll_event ev = {};
	ev.data.u64 = fd | (((uint64_t)handle)<<32);
	ev.events = mask;
	int s = epoll_ctl(_fd, EPOLL_CTL_ADD, fd, &ev);
	if (s == -1) {
		std::string msg= std::format("epoll_ctl add failed: _fd={} fd={} {}",
													(int)_fd, (int)fd, strerror(errno));
		dterrorf("{}", msg);
		return -1;
	}
	return 0;
}

int epoll_t::mod_fd(int fd, int mask) {
#ifdef DTDEBUG
	auto caller = gettid();
	if (owner == (pid_t)-1)
		owner = caller;
	assert(caller == owner); // needs to be called from owning thread
#endif
	struct epoll_event ev = {};
	ev.data.u64 = fd | (((uint64_t)handle)<<32);
	ev.events = mask;
	int s = epoll_ctl(_fd, EPOLL_CTL_MOD, fd, &ev);
	if (s == -1) {
		dterrorf("epoll_ctl mod failed: {}", strerror(errno));
		return -1;
	}
	return 0;
}

int epoll_t::remove_fd(int fd) {
#ifdef DTDEBUG
	auto caller = gettid();
	if (owner == (pid_t)-1)
		owner = caller;
	assert(caller == owner); // needs to be called from owning thread
#endif
	int s = epoll_ctl(_fd, EPOLL_CTL_DEL, fd, NULL);
	if (s == -1) {
		dterrorf("epoll_ctl remove failed: _fd={} fd={} {}", (int)_fd, (int) fd, strerror(errno));
		assert(false);
		return -1;
	}
	return 0;
}

int epoll_t::wait(struct epoll_event* events, int maxevents, int timeout) {
#ifdef DTDEBUG
	auto caller = gettid();
	if (owner == (pid_t)-1)
		owner = caller;
	assert(caller == owner); // needs to be called from owning thread
#endif
	for (;;) {
		int n = epoll_pwait(_fd, events, maxevents, timeout, // use a timeout of 0 to just check for events
												NULL);
		if (n < 0) {
			if (errno == EINTR) {
				//LOG4CXX_DEBUG(logger, "epoll_wait was interrupted");
				continue;
			} else {
				dterrorf("epoll_pwait failed: {}", strerror(errno));
				return -1;
			}
		} else
			return n;
	}
}

#ifdef DTDEBUG
void epoll_t::set_owner(pid_t pid) { owner = pid; }
#endif
