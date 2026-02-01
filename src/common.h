/*
 * Neumo blindscan (C) 2019-2026 deeptho@gmail.com
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
#include "util.h"
#include <string>
#include <assert.h>

enum class api_type_t {UNDEFINED, DVBAPI, NEUMO};

struct driver_info_t {
	bool valid{false};
	api_type_t api_type { api_type_t::UNDEFINED };
	int api_version{5000}; //1000 times the floating point value of version
	std::string driver_git_rev{"unknown"};
	std::string driver_git_tag{"unknown"};
	std::string driver_git_branch{"unknown"};

	void update();
	inline void invalidate() {
		valid = false;
	}

	inline auto get_api_type_and_version() {
		if(!valid)
			update();
		return std::tuple{ api_type, api_version };
	}
};


bool show_api_version(bool show_on_console);
