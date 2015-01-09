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

#include <cctype>
#include <utility>
#include <iterator>

#include <netinet/in.h>

namespace gcm {
namespace socket {
namespace util {

/*
IPs can have following formats:

<port>
[<IPv6>]:<port>
<IPv4>:<port>

*/

template<typename I, typename T>
I find(I begin, I end, T item) {
    I old_begin = begin;
    for (; begin < end; ++begin) {
        if (*begin == item) return begin;
    }

    return old_begin;
}

/**
 * Return port from any of formats above.
 * @returns Port number.
 */
template<typename I>
in_port_t get_port(I begin, I end) {
    std::reverse_iterator<I> rbegin(end);
    std::reverse_iterator<I> rend(begin);

    auto pos = find(rbegin, rend, ':');
    if (pos == rbegin) {
        pos = rend;
    }

    auto s = std::basic_string<std::decay_t<decltype(*begin)>>(pos.base(), end);
    return atoi(s.c_str());
}

/**
 * Get IPv4 address from any of formats above.
 * @returns Pair of iterators pointing to begining and end of IPv4 address in
 *   the struct.
 *   If IPv4 is not found, it returns both iterators set to the value of end iterator.
 */
template<typename I>
std::pair<I, I> get_ipv4(I begin, I end) {
    while (std::isspace(*begin)) {
        ++begin;
    }

    // Find port part.
    auto pos = find(begin, end, ':');

    // If there is no port in the string...
    if (begin == pos) {
        pos = end;
    }

    // All between begin and pos should be IPv4. Validate it.
    char buf[sizeof(in_addr)];
    if (inet_pton(AF_INET, std::basic_string<std::decay_t<decltype(*begin)>>(begin, pos).c_str(), buf) == 1) {
        return std::make_pair(begin, pos);
    } else {
        return std::make_pair(end, end);
    }
}

/**
 * Get IPv6 address from any of formats above.
 */
template<typename I>
std::pair<I, I> get_ipv6(I begin, I end) {
    while (std::isspace(*begin)) {
        ++begin;
    }

    std::reverse_iterator<I> rbegin(end);
    std::reverse_iterator<I> rend(begin);

    I endpos = find(rbegin, rend, ':').base();

    if (*begin == '[') {
        endpos = find(begin, end, ']');
        if (endpos == begin) {
            return std::make_pair(end, end);
        }
        ++begin;
    } else {
        if (endpos == rbegin.base()) {
            return std::make_pair(end, end);
        }
    }

    char buf[sizeof(in6_addr)];
    if (inet_pton(AF_INET6, std::basic_string<std::decay_t<decltype(*begin)>>(begin, endpos).c_str(), buf) == 1) {
        return std::make_pair(begin, endpos);
    } else {
        return std::make_pair(end, end);
    }
}

} // namespace util
} // namespace socket
} // namespace gcm
