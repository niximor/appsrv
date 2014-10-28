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
#include <fstream>

#include "message.h"
#include "formatter.h"

namespace gcm {
namespace logging {

class Handler {
public:
    Handler(Formatter &&formatter): formatter(std::move(formatter)) {}
	Handler(Handler &&) = default;

    virtual void write(Message &msg) = 0;
    virtual ~Handler();

protected:
    Formatter formatter;
};

class FileHandler: public Handler {
public:
    FileHandler(const std::string &fileName, Formatter &&formatter):
		Handler(std::move(formatter)),
		stream(fileName, std::ios_base::app)
    {}
	FileHandler(FileHandler &&) = default;

    virtual void write(Message &msg) {
        formatter.write(msg, stream);
		stream << std::endl;
    }

protected:
    std::ofstream stream;
};

class StdErrHandler: public Handler {
public:
	StdErrHandler(Formatter &&formatter):
		Handler(std::move(formatter)),
		stream(std::cerr)
	{}
	StdErrHandler(StdErrHandler &&) = default;

	virtual void write(Message &msg) {
		formatter.write(msg, stream);
		stream << std::endl;
	}

protected:
	std::ostream &stream;
};

} // namespace logging
} // namespace gcm
