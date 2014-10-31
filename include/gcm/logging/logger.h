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

#include <map>

#include "message.h"
#include "handler.h"

namespace gcm {
namespace logging {

class Logger {
public:
    friend class Message;

	Logger(Logger &&other) = default;
	Logger(const Logger &other) = delete;

    using HandlerList = std::vector<std::shared_ptr<Handler>>;

    static constexpr const char *DefaultFileName = "unknown";
    static constexpr int DefaultLineNo = 0;

    Message msg(MessageType type, const std::string &file = DefaultFileName, int line = DefaultLineNo) {
        return std::move(Message(*this, type, file, line));
    }

    Message critical(const std::string &file = DefaultFileName, int line = DefaultLineNo) {
        return std::move(msg(MessageType::Critical, file, line));
    }

    Message error(const std::string &file = DefaultFileName, int line = DefaultLineNo) {
        return std::move(msg(MessageType::Error, file, line));
    }

    Message warning(const std::string &file = DefaultFileName, int line = DefaultLineNo) {
        return std::move(msg(MessageType::Warning, file, line));
    }

    Message info(const std::string &file = DefaultFileName, int line = DefaultLineNo) {
        return std::move(msg(MessageType::Info, file, line));
    }

    Message debug(const std::string &file = DefaultFileName, int line = DefaultLineNo) {
        return std::move(msg(MessageType::Info, file, line));
    }

    static Logger &get_root() {
        return root;
    }

    void add_handler(std::unique_ptr<Handler> new_handler) {
        handlers.emplace_back(std::move(new_handler));
    }

	template<typename H>
	void add_handler(H &&other) {
		handlers.emplace_back(std::make_unique<H>(std::move(other)));
	}

    HandlerList &get_handlers() {
        if (handlers.empty() && &(parent) != this) {
            return parent.get_handlers();
        }
        return handlers;
    }

	const std::string &get_name() {
		return name;
	}

	static Logger &get(const std::string &name) {
		Logger *parent{&(Logger::get_root())};
		std::stringstream part_name;
		bool first{true};

		if (name.empty()) return *parent;

		std::string::size_type pos = std::string::npos;
		do {
			std::string::size_type oldpos = pos;
			if (oldpos == std::string::npos) {
				oldpos = 0;
			}

			pos = name.find(".", pos);

			std::string module{name.substr(
					oldpos, (pos != std::string::npos)?(pos - oldpos):pos)};

			if (!first) {
				part_name << ".";
				first = false;
			}
			part_name << module;

			auto it = parent->childs.find(module);
			if (it == parent->childs.end()) {
				Logger new_logger(*parent, part_name.str());
				auto inserted = parent->childs.emplace(std::make_pair(module, std::move(new_logger)));
				parent = &(inserted.first->second);
			} else {
				parent = &(it->second);
			}
		} while (pos != std::string::npos);

		return *parent;
	}

private:
    Logger(): parent(*this), name("RootLogger")
	{
		std::cout << "Created root logger." << std::endl;
	}

    Logger(Logger &parent, const std::string &name): parent(parent), name(name)
	{}

    static Logger root;

protected:
	void write(Message &msg) {
		for (auto handler: get_handlers()) {
            handler->write(msg);
        }
	}

protected:
    Logger &parent;
    std::map<std::string, Logger> childs;
    std::vector<std::shared_ptr<Handler>> handlers;
	std::string name;
};

} // namespace logging
} // namespace gcm
