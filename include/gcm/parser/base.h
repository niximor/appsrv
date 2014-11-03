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
 * @date 2014-11-02
 *
 */

#pragma once

//#define PARSER_DEBUG

#include <type_traits>

#ifdef PARSER_DEBUG
#   include <gcm/logging/logging.h>
#endif

namespace gcm {
namespace parser {

class rule_base {
public:
#ifdef PARSER_DEBUG
    rule_base(): log(gcm::logging::getLogger("GCM.Parser"))
    {}

    gcm::logging::Logger &log;
#else
    rule_base() {}
#endif
};

template<typename T>
struct is_rule: std::integral_constant<bool, std::is_base_of<rule_base, std::decay_t<T>>::value>
{};

} // namespace parser
} // namespace gcm
