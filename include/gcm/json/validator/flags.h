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

#include "detail/flags.h"

namespace gcm {
namespace json {
namespace validator {

template<typename T>
auto Nullable(T param_def) {
    return detail::Nullable_t<T>(param_def);
}

template<typename T>
auto Optional(T param_def) {
    return detail::Optional_t<T>(param_def);
}

template<typename T, typename V>
auto Optional(T param_def, V default_value) {
    return detail::OptionalWithDefault_t<T, V>(param_def, default_value);
}

} // namespace validator
} // namespace json
} // namespace gcm
