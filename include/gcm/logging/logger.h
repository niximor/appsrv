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

class Logger;
Logger &get_root_logger();

class Logger {
friend Logger &get_root_logger();

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
        return std::move(msg(MessageType::Debug, file, line));
    }

    static Logger &get_root() {
        return get_root_logger();
    }

    void add_handler(std::unique_ptr<Handler> new_handler) {
        handlers.emplace_back(std::move(new_handler));
    }

	template<typename H>
	void add_handler(H &&other) {
		handlers.emplace_back(std::make_unique<H>(std::move(other)));
	}

    HandlerList &get_handlers(bool only_direct = false) {
        if (!only_direct && handlers.empty() && &(parent) != this) {
            return parent.get_handlers();
        }
        return handlers;
    }

	const std::string &get_name() {
		return name;
	}

    Logger &get_child(const std::string &name) {
        Logger *parent{this};

        const std::string &root_name{parent->get_name()};
        std::stringstream part_name{};
        bool first{root_name.empty()};

        if (!first) {
            part_name << root_name;
        }

        if (name.empty()) return *parent;

        std::string::size_type pos = std::string::npos;
        do {
            std::string::size_type oldpos = pos;
            if (oldpos == std::string::npos) {
                oldpos = 0;
            } else {
                // Skip the dot from previous part.
                ++oldpos;
            }

            pos = name.find('.', oldpos);

            std::string module{name.substr(
                    oldpos, (pos != std::string::npos)?(pos - oldpos):name.size() - oldpos)};

            if (!first) {
                part_name << ".";
            } else {
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

	static Logger &get(const std::string &name) {
		return get_root().get_child(name);		
	}

    template<typename Stream>
    void dump(Stream &s) {
        dump(s, 0);
    }

private:
    Logger(): parent(*this), name("")
	{}

    Logger(Logger &parent, const std::string &name): parent(parent), name(name)
	{}

protected:
    template<typename Stream>
    void dump(Stream &s, int level) {
        std::string indent(level * 4, ' ');
        if (name.empty()) {
            s << indent << "-root-logger-\n";
        } else {
            s << indent << name << "\n";
        }
        s << indent << " - handlers: ";
        for (auto &h: handlers) {
            s << (void *)(&h);
        }
        s << "\n";
        s << indent << " - childs: \n";
        for (auto &ch: childs) {
            ch.second.dump(s, level + 1);
        }
    }

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

inline Logger &get_root_logger() {
    static Logger root;
    return root;
}

} // namespace logging
} // namespace gcm
