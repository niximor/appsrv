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
#include <vector>
#include <memory>

#include "field.h"

namespace gcm {
namespace logging {

class Message;

class Formatter {
public:
    template<typename... T>
    Formatter(T... params) {
        add_format(params...);
    }

	Formatter(const Formatter &) = delete;
	Formatter(Formatter &&) = default;

    template<typename... T>
    void set_format(T... params) {
        fields.clear();
        add_format(params...);
    }

    void write(Message &msg, std::ostream &stream) {
        for (auto &f: fields) {
            f->format(msg, stream);
        }
    }

protected:
    std::vector<std::unique_ptr<field::Field>> fields;

    void add_format() {}

    template<typename... T>
    void add_format(const std::string &literal, T... params) {
        fields.push_back(std::make_unique<field::LiteralField>(std::move(field::LiteralField(literal))));
        add_format(params...);
    }

	template<typename... T>
	void add_format(const char *literal, T... params) {
		fields.push_back(std::make_unique<field::LiteralField>(std::move(field::LiteralField(literal))));
		add_format(params...);
	}

    template<typename F, typename... T>
    void add_format(F f, T... params) {
        fields.push_back(std::make_unique<F>(std::move(f)));
		add_format(params...);
    }
};

} // namespace logging
} // namespace gcm
