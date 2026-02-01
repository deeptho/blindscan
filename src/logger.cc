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
#include <assert.h>
#include <string>
#include <format>
#include <sys/timeb.h>
#include <signal.h>
#include <fmt/chrono.h>
#include <filesystem>
#include "dtlogger.h"
#include <log4cxx/logger.h>
#include <log4cxx/helpers/pool.h>
#include <log4cxx/basicconfigurator.h>
#include "log4cxx/propertyconfigurator.h"
#include "log4cxx/xml/domconfigurator.h"
#include <log4cxx/fileappender.h>
#include <log4cxx/patternlayout.h>
#include "log4cxx/consoleappender.h"
#include "log4cxx/ndc.h"

using namespace log4cxx;
using namespace log4cxx;
using namespace log4cxx::xml;
using namespace log4cxx::helpers;

#include <ctime>
#include <iomanip>
#include <iostream>
#include <time.h>

namespace fs = std::filesystem;

LoggerPtr create_log() {
	const char* pattern = "%d %.40F:%L\n  %m %n";
	auto l = LayoutPtr(new PatternLayout(pattern));
	ConsoleAppender* consoleAppender = new ConsoleAppender(l);
	helpers::Pool p;
	BasicConfigurator::configure(AppenderPtr(consoleAppender));
	// TRACE < DEBUG < INFO < WARN < ERROR < FATAL.
	Logger::getRootLogger()->setLevel(Level::getDebug());
	LoggerPtr logger = Logger::getLogger("logger");
	logger->setLevel(Level::getDebug());
	return logger;
}

void set_logconfig(const char* logconfig_) {
	// Block signals so that the main process inherits them...
	sigset_t sigset, old;
	sigfillset(&sigset);
	sigprocmask(SIG_BLOCK, &sigset, &old);
	if (!logconfig_ || !logconfig_[0]) {
		const char* pattern = "%m %n\n";
		auto l = LayoutPtr(new PatternLayout(pattern));
		ConsoleAppender* consoleAppender = new ConsoleAppender(l);
		helpers::Pool p;
		BasicConfigurator::configure(AppenderPtr(consoleAppender));
	} else {
		std::string logconfig{logconfig_};
		auto p = fs::canonical("/proc/self/exe").parent_path() /
			 fs::path("../../config") / fs::path(logconfig);
		p +=".xml";
		DOMConfigurator::configureAndWatch(p, 1000);
		dtdebugf("STARTING");
	}
	sigprocmask(SIG_SETMASK, &old, &sigset);
}


// Name to identify the console appender in the root logger
static const LogString CONSOLE_APPENDER_NAME = LOG4CXX_STR("DynamicConsole");

void set_console_logging(bool enable) {
	LoggerPtr rootLogger = Logger::getRootLogger();
	AppenderPtr consoleAppender = rootLogger->getAppender(CONSOLE_APPENDER_NAME);

	if (enable) {
		if (!consoleAppender) {
			// Create ConsoleAppender
			ConsoleAppenderPtr newAppender = std::make_shared<ConsoleAppender>();
			newAppender->setName(CONSOLE_APPENDER_NAME);

			// Set Layout
			LayoutPtr layout = std::make_shared<PatternLayout>(LOG4CXX_STR("%-5p %c - %m%n"));
			newAppender->setLayout(layout);

			// Activate options
			Pool pool;
			newAppender->activateOptions(pool);

			// Add to root logger
			rootLogger->addAppender(newAppender);
		}
	} else {
		if (consoleAppender) {
            // Remove appender from root logger
			rootLogger->removeAppender(consoleAppender);
			consoleAppender->close();
		}
	}
}

LoggerPtr mainlogger = create_log();
thread_local LoggerPtr logger = Logger::getLogger("main");
thread_local log4cxx::NDC global_ndc("");
