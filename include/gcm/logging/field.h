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

#include <string>
#include <iostream>

#include <time.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "message.h"

namespace gcm {
namespace logging {

// Forward
class Message;

namespace field {

using Msg = gcm::logging::Message;

class Field {
public:
	Field() = default;
	Field(const Field &) = default;
	Field(Field &&) = default;

    virtual void format(Msg &msg, std::ostream &stream) = 0;
    virtual ~Field();
};

class LiteralField: public Field {
public:
    LiteralField(const std::string &literal): literal(literal)
    {}
	LiteralField(const LiteralField &) = default;
	LiteralField(LiteralField &&) = default;

    void format(Msg &, std::ostream &stream) {
        stream << literal;
    }

protected:
    std::string literal;
};

class File: public Field {
public:
    void format(Msg &msg, std::ostream &stream) {
        stream << msg.get_file();
    }
};

class Pid: public Field {
public:
	void format(Msg &, std::ostream &stream) {
        pid_t pid = getpid();
        pid_t tid = syscall(SYS_gettid);
		stream << pid;

        if (tid != pid) {
            stream << ":" << tid;
        }
	}
};

class Severenity: public Field {
public:
	void format(Msg &msg, std::ostream &stream) {
		switch (msg.get_severenity()) {
			case MessageType::Critical: stream << "CRIT"; break;
			case MessageType::Error: stream << "ERR"; break;
			case MessageType::Warning: stream << "WARN"; break;
			case MessageType::Info: stream << "INFO"; break;
			case MessageType::Debug: stream << "DEBUG"; break;
		}
	}
};

class Line: public Field {
public:
    void format(Msg &msg, std::ostream &stream) {
        stream << msg.get_line();
    }
};

class Message: public Field {
public:
    void format(Msg &msg, std::ostream &stream) {
        stream << msg.get_message();
    }
};

class Date: public Field {
public:
    Date(const std::string &format = "%Y-%m-%d %H:%M:%S"): fmt(format)
    {}
	Date(const Date &) = default;
	Date(Date &&) = default;

    void format(Msg &, std::ostream &stream) {
		tm tm_snapshot;
		time_t now{time(NULL)};
		localtime_r(&now, &tm_snapshot);

		char formatted[128];
		strftime(formatted, sizeof(formatted), fmt.c_str(), &tm_snapshot);

		stream << formatted;
    }

protected:
    std::string fmt;
};

class Name: public Field {
public:
	void format(Msg &, std::ostream &);
};

} // namespace field
} // namespace logging
} // namespace gcm
