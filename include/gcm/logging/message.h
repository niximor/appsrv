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

#include <sstream>

namespace gcm {
namespace logging {

class Logger;

enum class MessageType {
	Critical = 50,
	Error = 40,
	Warning = 30,
	Info = 20,
	Debug = 10
};

class Message {
public:
    friend class Logger;

    Message(Message &&other):
		logger(other.logger), type(other.type), message(other.message.str()),
		file(std::move(other.file)), line(other.line), moved(false)
	{
		other.moved = true;
	}

    template<typename T>
    Message &operator <<(T msg) {
        message << msg;
		return *this;
    }

    std::string get_message() const {
        return message.str();
    }

    std::string get_file() const {
        return file;
    }

    int get_line() {
        return line;
    }

	MessageType get_severenity() {
		return type;
	}

	Logger &get_logger() {
		return logger;
	}

    ~Message();

protected:
    Message(Logger &logger, MessageType type, const std::string file, int line):
        logger(logger), type(type), file(file), line(line), moved(false)
    {}

    Logger &logger;
    MessageType type;
    std::stringstream message;
    std::string file;
    int line;
	bool moved;
};

} // namespace logging
} // namespace gcm
