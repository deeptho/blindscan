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
#include "common.h"
#include <libconfig.h++>
#include <string.h>

driver_info_t driver_info;

void driver_info_t::update() {
	if(valid)
		return;
	valid = true;

	using namespace libconfig;
	Config cfg;
	try {
		cfg.readFile("/sys/module/dvb_core/info/version");
	} catch (const FileIOException& fioex) {
		api_type = api_type_t::DVBAPI;
		api_version = 5000;
		dtdebugf("non-Neumo dvbapi detected");
		return;
	}

	try {
		std::string type = cfg.lookup("type");
		if (strcmp(type.c_str(), "neumo") != 0)
			return;
	} catch (const SettingNotFoundException& nfex) {
		return;
	}
	api_type = api_type_t::NEUMO;

	try {
		std::string version = cfg.lookup("version");
		dtdebugf("Neumo dvbapi detected; version={}", version);
		api_version = 1000*std::stof(version);
	} catch (const SettingNotFoundException& nfex) {
		return;
	}

	try {
		std::string driver_git_rev = cfg.lookup("GIT-REV");
			std::string driver_git_tag = cfg.lookup("GIT-TAG");
			std::string driver_git_branch = cfg.lookup("GIT-BRANCH");
			dtdebugf("DRIVERS: GIT-REV={} GIT-TAG={} GIT-BRANCH={}", driver_git_rev, driver_git_tag, driver_git_branch);
	}
	catch (const SettingNotFoundException& nfex) {
		dtdebugf("DRIVERS: unknown");
	}
}

bool show_api_version(bool show) {
	auto [api_type, api_version] = driver_info.get_api_type_and_version();
	bool has_blindscan{false};

	if(api_type == api_type_t::NEUMO) {
		dtdebugf("Blindscan drivers found");
		has_blindscan = true;
	} else {
		show=true;
		dtdebugf("!!!!Blindscan drivers not installed  - only regular tuning will work!!!!");
	}

	if(show) {
		set_logconfig(nullptr);
		if(api_type == api_type_t::NEUMO)
			dtdebugf("Neumo Drivers: api_version={}", api_version);
		else
			dtdebugf("No Neumo Drivers: api_version=%f", api_version);
	}
	return has_blindscan;
}
