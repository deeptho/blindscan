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

#pragma once
#include "dtlogger.h"
#include <format>
#include <sys/epoll.h>

class epoll_t {
	int _fd{-1};
#ifdef DTDEBUG
	pid_t owner = (pid_t)-1;
#endif
	int32_t handle{0};

	void init();

 public:
	epoll_t() {
		init();
	}

	epoll_t(epoll_t&& other)  = default;
	epoll_t(const epoll_t& other)  = delete;
	epoll_t& operator=(const epoll_t& other)  = delete;

	~epoll_t() {
		close();
	}
	operator int() const {
		return _fd;
	}

	bool matches(const epoll_event* event, int fd) const {
		bool fd_matches = (event->data.u64 & 0xffffffff) == fd;
		bool handle_matches = (event->data.u64 >>32) == handle;
		if(fd_matches && ! handle_matches)
			printf("Ignored bad match\n");
		return (event->data.u64 & 0xffffffff) == fd && (event->data.u64 >>32) == handle;
	}

	int close();

	int remove_fd(int fd);
	int add_fd(int fd, int mask);
	int mod_fd(int fd, int mask);
	int wait(struct epoll_event* events, int maxevents=16, int timeout=-1);
#ifdef DTDEBUG
	void set_owner(pid_t pid);
	void set_owner() {
		set_owner(gettid());
	}
#endif
};
