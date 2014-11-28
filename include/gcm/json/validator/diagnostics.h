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
#include <vector>
#include <memory>

#include "../json.h"

namespace gcm {
namespace json {
namespace validator {

enum class ProblemCode {
    WrongType = 401,
    CannotBeNull = 402,
    MustBePresent = 403,
    UnexpectedParam = 404
};

class Problem {
public:
    Problem(const std::string &item, ProblemCode code, const std::string &description):
        item(item),
        code(code),
        description(description)
    {}

    JsonValue to_json() const {
        auto obj = Object();

        obj["item"] = make_string(item);
        obj["code"] = make_int(static_cast<int>(code));
        obj["description"] = make_string(description);

        return std::make_shared<Object>(std::move(obj));
    }

protected:
    const std::string item;
    ProblemCode code;
    const std::string description;
};

class Diagnostics: public json::Exception {
public:
    Diagnostics(): json::Exception("There were errors processing your request.")
    {}

    Diagnostics(Diagnostics &&) = default;
    Diagnostics(const Diagnostics &) = default;

    void add_problem(Problem &&problem) {
        problems.emplace_back(std::forward<Problem>(problem));
    }

    void add_problem(const std::string &item, ProblemCode code, const std::string &description) {
        problems.emplace_back(
            item,
            code,
            description
        );
    }

    const std::vector<Problem> &get_problems() {
        return problems;
    }

protected:
    std::vector<Problem> problems;
};

} // namespace gcm
} // namespace json
} // namespace validator
