/**
 * Copyright 2014 Michal Kuchta <niximor@gmail.com>
 *
 * This file is part of GCM::AppSrv.
 *
 * GCM::AppSrv is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * GCM::AppSrv is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with GCM::AppSrv. If not, see http://www.gnu.org/licenses/.
 *
 * @author Michal Kuchta <niximor@gmail.com>
 * @date 2014-10-28
 *
 */

#pragma once

#include <iostream>
#include <sstream>
#include <memory>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstdlib>

#include "message.h"
#include "formatter.h"

#include <gcm/io/exception.h>

namespace gcm {
namespace logging {

struct Levels {
    bool critical;
    bool error;
    bool warning;
    bool info;
    bool debug;
};

class Handler {
public:
    Handler(Formatter formatter):
        formatter(formatter),
        levels{true, true, true, true, false}
    {}

	Handler(Handler &&) = default;
    Handler(const Handler &) = default;

    virtual void write(Message &msg) = 0;
    virtual ~Handler();

    void enable_type(MessageType type) {
        set_type(type, true);
    }

    void disable_type(MessageType type) {
        set_type(type, false);
    }

    void set_type(MessageType type, bool enabled) {
        switch (type) {
            case MessageType::Critical: levels.critical = enabled; break;
            case MessageType::Error: levels.error = enabled; break;
            case MessageType::Warning: levels.warning = enabled; break;
            case MessageType::Info: levels.info = enabled; break;
            case MessageType::Debug: levels.debug = enabled; break;
        }
    }

    bool is_enabled(MessageType type) {
        switch (type) {
            case MessageType::Critical: return levels.critical;
            case MessageType::Error: return levels.error;
            case MessageType::Warning: return levels.warning;
            case MessageType::Info: return levels.info;
            case MessageType::Debug: return levels.debug;
        }
        return false;
    }

protected:
    Formatter formatter;
    Levels levels;
};

class FileHandler: public Handler {
public:
    FileHandler(const std::string &fileName, Formatter formatter):
		Handler(std::forward<Formatter>(formatter)),
		//stream(std::make_shared<std::ofstream>(fileName, std::ios_base::app))
        out(open(fileName.c_str(), O_WRONLY | O_APPEND | O_CREAT))
    {
        if (out <= 0) {
            throw gcm::io::IOException(errno);
        }
    }

    FileHandler(const FileHandler &other): Handler(other), out(dup(other.out)) {
        if (out <= 0) {
            throw gcm::io::IOException(errno);
        }
    }

    FileHandler(FileHandler &&other): Handler(std::forward<Handler>(other)), out(other.out) {
        other.out = 0;
    }

    ~FileHandler() {
        if (out > 0) {
            ::close(out);
        }
    }

    virtual void write(Message &msg) {
        if (out <= 0) return;

        if (is_enabled(msg.get_severenity())) {
            // Need to buffer the message first, to write it to the file
            // at once, because otherwise other threads would interrupt
            // our custom message processing and write another data
            // to the file between field writes.
            std::stringstream ss;
            formatter.write(msg, ss);
            ss << std::endl;
            
            std::string out_str{ss.str()};
            if (::write(out, out_str.c_str(), out_str.length()) < 0) {
                std::cerr << "Unable to write to log file: " << strerror(errno) << std::endl;
            } else if (fsync(out) < 0) {
                std::cerr << "Unable to sync log file: " << strerror(errno) << std::endl;
            }
        }
    }

protected:
    int out;
};

class StdErrHandler: public Handler {
public:
	StdErrHandler(Formatter formatter):
		Handler(formatter),
		stream(std::cerr), color(false)
	{}

	virtual void write(Message &msg) {
        if (is_enabled(msg.get_severenity())) {
            // Need to buffer the message first, to write it to the file
            // at once, because otherwise other threads would interrupt
            // our custom message processing and write another data
            // to the file between field writes.
            std::stringstream ss;
            formatter.write(msg, ss);
            ss << std::endl;
            stream << ss.str();
        }
	}

protected:
	std::ostream &stream;
    bool color;
};

} // namespace logging
} // namespace gcm
