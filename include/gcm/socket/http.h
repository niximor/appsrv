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

#include <utility>
#include <string>
#include <sstream>
#include <exception>

#include <gcm/parser/parser.h>

namespace gcm {
namespace socket {
namespace http {

using HttpHeader = std::pair<std::string, std::string>;

constexpr const char *CrLf = "\r\n";

inline const char *get_default_status_message(int status) {
    switch (status) {
        case 100: return "Continue";
        case 101: return "Switching Protocols";
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 203: return "Non-Authoritative Information";
        case 204: return "No Content";
        case 205: return "Reset Content";
        case 206: return "Partial Content";
        case 300: return "Multiple Choices";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 305: return "Use Proxy";
        case 306: return "(Unused)";
        case 307: return "Temporary Redirect";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 402: return "Payment Required";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 406: return "Not Acceptable";
        case 407: return "Proxy Authentication Required";
        case 408: return "Request Timeout";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 411: return "Length Required";
        case 412: return "Precondition Failed";
        case 413: return "Request Entity Too Large";
        case 414: return "Request-URI Too Long";
        case 415: return "Unsupported Media Type";
        case 416: return "Requested Range Not Satisfiable";
        case 417: return "Expectation Failed";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";
        default: return "Unknown";
    }
}

class HttpException: public std::runtime_error {
public:
    HttpException(int status):
        std::runtime_error(get_default_status_message(status)),
        status(status)
    {}

    HttpException(int status, const std::string &status_message):
        std::runtime_error(status_message),
        status(status)
    {}

    template<typename T>
    void write(T &&stream);

    int get_status() {
        return status;
    }

protected:
    int status;
};

class HeaderSet: public std::vector<HttpHeader> {
public:
    template<typename T>
    void write(T &&stream) const {
        stream << (std::string)(*this);
    }

    const std::string operator[](const std::string &index) const {
        for (auto &header: (*this)) {
            if (header.first == index) {
                return header.second;
            }
        }

        return "";
    }

    bool has_header(const std::string &index) const {
        for (auto &header: (*this)) {
            if (header.first == index) {
                return true;
            }
        }

        return false;
    }

    operator std::string() const {
        std::stringstream ss;

        for (auto &header: (*this)) {
            ss << header.first << ": " << header.second << CrLf;
        }
        ss << CrLf;

        return ss.str();
    }

    std::string to_string() const {
        return std::string(*this);
    }
};

class HttpVersion {
public:
    HttpVersion(): maj(1), min(0)
    {}

    HttpVersion(int maj, int min): maj(maj), min(min)
    {}

    int get_major() {
        return maj;
    }

    int get_minor() {
        return min;
    }

    void set_version(int new_major, int new_minor) {
        maj = new_major;
        min = new_minor;
    }

    void set_major(int new_major) {
        maj = new_major;
    }

    void set_minor(int new_minor) {
        min = new_minor;
    }

    int compare_to(int o_maj, int o_min) {
        if (maj == o_maj && min == o_min) return 0;
        else if (maj < o_maj || (maj == o_maj && min < o_min)) return -1;
        else return 1;
    }

    bool operator==(const HttpVersion &other) {
        return maj == other.maj && min == other.min;
    }

    bool operator!=(const HttpVersion &other) {
        return maj != other.maj || min != other.min;
    }

    bool operator>(const HttpVersion &other) {
        return maj > other.maj || min > other.min;
    }

    bool operator<(const HttpVersion &other) {
        return maj < other.maj || min < other.min;
    }

protected:
    int maj;
    int min;
};

class BaseHttpResponse {
public:
    BaseHttpResponse():
        status(200),
        version(1, 0),
        headers_written(false)
    {}

    BaseHttpResponse(const HttpVersion &version):
        status(200),
        version(version),
        headers_written(false)
    {}

    BaseHttpResponse(int status, const HttpVersion &version):
        status(status),
        version(version),
        headers_written(false)
    {}

    BaseHttpResponse(int status, const std::string &status_message):
        status(status),
        status_message(status_message),
        version(1, 0),
        headers_written(false)
    {}

    BaseHttpResponse(int status, const std::string &status_message, const HttpVersion &version):
        status(status),
        status_message(status_message),
        version(version),
        headers_written(false)
    {}

    BaseHttpResponse(BaseHttpResponse &&) = default;
    BaseHttpResponse(const BaseHttpResponse &) = default;

    template<typename T>
    void write_headers(T &stream) {
        if (!headers_written) {
            if (version.compare_to(1, 1) >= 0) {
                // Connection header.
                if (!has_header("Content-Length")) {
                    set_header("Connection", "close");
                }
            }

            stream << "HTTP/" << version.get_major() << '.' << version.get_minor() << ' ' << status << ' ' << get_status_message() << CrLf;
            headers.write(stream);
            headers_written = true;
        } else {
            throw HttpException(500, "Headers already written.");
        }
    }

    const HeaderSet &get_headers() const {
        return headers;
    }

    void add_header(const std::string &name, const std::string &value) {
        if (headers_written) {
            throw HttpException(500, "Headers already written.");
        }

        headers.push_back(std::make_pair(name, value));
    }

    void set_header(const std::string &name, const std::string &value) {
        if (headers_written) {
            throw HttpException(500, "Headers already written.");
        }

        for (auto &it: headers) {
            if (it.first == name) {
                it.second = value;
                return;
            }
        }

        add_header(name, value);
    }

    void set_status(int new_status) {
        if (headers_written) {
            throw HttpException(500, "Headers already written.");
        }

        status = new_status;
    }

    void set_status(int new_status, const std::string &new_msg) {
        if (headers_written) {
            throw HttpException(500, "Headers already written.");
        }

        set_status(new_status);
        set_status_message(new_msg);
    }

    int get_status() const {
        return status;
    }

    void set_status_message(const std::string &new_msg) {
        if (headers_written) {
            throw HttpException(500, "Headers already written.");
        }

        status_message = new_msg;
    }

    std::string get_status_message() const {
        if (status_message.empty()) {
            return get_default_status_message(status);
        } else {
            return status_message;
        }
    }

    void set_version(HttpVersion new_version) {
        if (headers_written) {
            throw HttpException(500, "Headers already written.");
        }

        version = new_version;
    }

    const HttpVersion &get_version() const {
        return version;
    }

    const std::string operator[](const std::string &index) const {
        return get_headers()[index];
    }

    bool has_header(const std::string &index) const {
        return get_headers().has_header(index);
    }

protected:
    HeaderSet headers;
    int status;
    std::string status_message;
    HttpVersion version;
    bool headers_written;
};

template<typename T>
class HttpResponse;

class HttpRequest {
public:
    HttpRequest() = default;
    HttpRequest(HttpRequest &&) = default;
    HttpRequest(const HttpRequest &) = default;

    template<typename T>
    void parse(T &&stream) {
        std::string head;
        head.reserve(65535);

        char ch = '\0';
        size_t len = 0;

        while (true) {
            stream >> ch;

            if (stream.eof()) {
                break;
            }

            head += ch;
            ++len;

            if (len >= 4 && head.substr(len - 4, 4) == "\r\n\r\n") {
                break;
            }
        }

        if (!parse(head.begin(), head.end() - 2)) {
            // TODO: Throw exception when header parsing failed.
        }

        // Here we have parsed head, and in the stream remains rest of body. Everything is OK.
    }

    const HeaderSet &get_headers() const {
        return headers;
    }

    HeaderSet &get_headers() {
        return headers;
    }

    const std::string &get_method() const {
        return method;
    }

    const std::string &get_uri() const {
        return uri;
    }

    HttpVersion get_version() {
        return version;
    }

    template<typename T>
    HttpResponse<T> get_response(T &&stream) {
        auto resp = HttpResponse<T>{*this, std::forward<T>(stream)};

        resp.set_header("Server", "GCM::HTTP");

        // HTTP/1.1 supports keep-alive connections.
        if (get_version().compare_to(1, 1) >= 0) {
            if (!has_header("Content-Length") || (*this)["Connection"] != "keep-alive") {
                resp.set_header("Connection", "close");
            } else {
                resp.set_header("Connection", "keep-alive");
            }
        }

        return resp;
    }

    std::string operator[](const std::string &index) const {
        return get_headers()[index];
    }

    bool has_header(const std::string &index) const {
        return get_headers().has_header(index);
    }

protected:
    std::string method;
    std::string uri;
    HttpVersion version;
    HeaderSet headers;

    template<typename I>
    bool parse(I begin, I end) {
        /* The parsing must be done in two steps. First, read everything until first empty line (\r\n\r\n),
           pass it to the parser. Then, the handler itself should read the body. */

        using namespace gcm::parser;
        using namespace std::placeholders;

        auto SP = ' '_r;
        auto HT = '\t'_r;
        auto CTLS = cntrl();
        auto CRLF = literal_rule(CrLf);

        auto LWS = CrLf & +(SP | HT);

        auto separators = '('_r | ')'_r | '<'_r | '>'_r | '@'_r
                        | ','_r | ';'_r | ':'_r | '\\'_r | '"'_r
                        | '/'_r | '['_r | ']'_r | '?'_r | '='_r
                        | '{'_r | '}'_r | SP | HT;

        auto token = +(any_rule() - CTLS - separators);
        auto method = token;

        auto http_version = "HTTP/"_r
            & +digit() >> std::bind(&HttpRequest::set_major_version<I>, this, _1, _2)
            & '.'
            & +digit() >> std::bind(&HttpRequest::set_minor_version<I>, this, _1, _2);

        auto request_uri = *(any_rule() - space());

        auto request_line =
            method >> std::bind(&HttpRequest::set_method<I>, this, _1, _2)
            & SP
            & request_uri >> std::bind(&HttpRequest::set_uri<I>, this, _1, _2)
            & SP
            & http_version
            & CrLf;

        HttpHeader current_header;

        auto field_name = *(print() - ':');
        auto field_body =
            *space()
            & *(any_rule() - CrLf) >> std::bind(&HttpRequest::set_field_body<I>, this, &current_header, _1, _2)
            & CrLf
            & *(
                +space()
                & *(any_rule() - CrLf) >> std::bind(&HttpRequest::set_field_body<I>, this, &current_header, _1, _2)
                & CrLf
            );

        auto field =
            field_name >> std::bind(&HttpRequest::set_field_name<I>, this, &current_header, _1, _2)
            & ":"
            & field_body >> std::bind(&HttpRequest::add_header<I>, this, &current_header, _1, _2);

        auto request = request_line & *field;

        request(begin, end);

        if (begin == end) {
            return true;
        } else {
            return false;
        }
    }

    template<typename I>
    void set_major_version(I begin, I end) {
        version.set_major(atoi(std::string(begin, end).c_str()));
    }

    template<typename I>
    void set_minor_version(I begin, I end) {
        version.set_minor(atoi(std::string(begin, end).c_str()));
    }

    template<typename I>
    void set_method(I begin, I end) {
        method = std::string(begin, end);
    }

    template<typename I>
    void set_uri(I begin, I end) {
        uri = std::string(begin, end);
    }

    template<typename I>
    void set_field_name(HttpHeader *current_header, I begin, I end) {
        current_header->first = std::string(begin, end);
        current_header->second = "";
    }

    template<typename I>
    void set_field_body(HttpHeader *current_header, I begin, I end) {
        if (!current_header->second.empty()) {
            current_header->second.append(" ");
            
        }
        current_header->second.append(begin, end);
    }

    template<typename I>
    void add_header(HttpHeader *current_header, I, I) {
        headers.push_back(*current_header);
    }
};

template<typename T>
class HttpResponse: public BaseHttpResponse {
public:
    template<typename S>
    HttpResponse(HttpRequest &req, S &&stream):
        BaseHttpResponse(req.get_version()),
        request(req),
        stream(std::forward<S>(stream))
    {}

    HttpRequest &get_request() {
        return request;
    }

    template<typename V>
    HttpResponse &operator<<(V &&s) {
        if (!this->headers_written) {
            this->write_headers(stream);
        }

        stream << s;
        return *this;
    }

protected:
    HttpRequest &request;
    T &stream;
};

template<typename T>
void HttpException::write(T &&stream){
    BaseHttpResponse resp(status, std::string(this->what()));
    resp.write_headers(stream);

    stream << "<html><body><h1>" << status << ": " << this->what() << "</h1></body></html>";
}

} // namespace http
} // namespace socket
} // namespace gcm
