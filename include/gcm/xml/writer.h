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
 * @date 2014-12-16
 *
 */

#pragma once

#include <string>
#include <stdexcept>
#include <sstream>

namespace gcm {
namespace xml {

class SerializeException: public std::runtime_error {
public:
    SerializeException(const char *what): std::runtime_error(what)
    {}
};

template<typename Stream>
class Element;

template<typename Stream>
class Writer {
friend class Element<Stream>;
public:
    Writer(Stream &stream, bool declaration=true, bool format=false, std::size_t indent=1, const std::string &indent_str=" "):
        stream(stream),
        format(format),
        indent(indent),
        indent_str(indent_str)
    {
        if (declaration) {
            stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        }
    }

    Element<Stream> element(const std::string &name) {
        return Element<Stream>(*this, name, 0);
    }

protected:
    Stream &stream;
    bool format;
    std::size_t indent;
    std::string indent_str;
};

template<typename Stream>
class Element {
friend class Writer<Stream>;
public:
    ~Element() {
        if (is_in_start_tag) {
            writer.stream << "/>";
        } else {
            if (has_long_content && writer.format) {
                fill(writer.indent_str, indent);
            }

            writer.stream << "</" << name << ">";
        }

        if (writer.format) {
            writer.stream << "\n";
        }
    }

    Element<Stream> element(const std::string &name) {
        end_start_tag(true);

        return Element<Stream>(writer, name, indent + writer.indent);
    }

    template<typename T>
    void text(T data) {
        end_start_tag();

        if (has_long_content && writer.format) {
            fill(writer.indent_str, indent + writer.indent);
        }

        writer.stream << escape(data);

        if (has_long_content && writer.format) {
            writer.stream << "\n";
        }
    }

    void comment(const std::string &data) {
        end_start_tag(true);

        if (writer.format) {
            fill(writer.indent_str, indent + writer.indent);
        }

        writer.stream << "<!-- " << data << " -->";

        if (writer.format) {
            writer.stream << "\n";
        }
    }

    template<typename T>
    void attribute(const std::string &name, T &value) {
        if (!is_in_start_tag) {
            throw SerializeException("Element content already set, cannot set attribute.");
        }

        writer.stream << " " << name << "=\"" << escape(value) << "\"";
    }

protected:
    Element(Writer<Stream> &writer, const std::string &name, std::size_t indent):
        writer(writer),
        name(name),
        indent(indent),
        has_long_content(false),
        is_in_start_tag(true)
    {
        if (writer.format) {
            fill(writer.indent_str, indent);
        }

        writer.stream << "<" << name;
    }

    void end_start_tag(bool long_content=false) {
        if (is_in_start_tag) {
            writer.stream << ">";
            is_in_start_tag = false;

            if ((long_content || has_long_content) && writer.format) {
                writer.stream << "\n";
            }
        }

        has_long_content = long_content;
    }

    void inline_replace(std::string &str, const std::string &from, const std::string &to) {
        std::string::size_type pos = 0;
        while ((pos = str.find(from, pos)) != std::string::npos) {
            str.replace(pos, from.length(), to);
            pos += to.length();
        }
    }

    std::string escape(const std::string &src) {
        std::string out = src;

        inline_replace(out, "&", "&amp;");
        inline_replace(out, "\"", "&quot;");
        inline_replace(out, "<", "&lt;");
        inline_replace(out, ">", "&gt;");

        return out;
    }

    template<typename T>
    std::string escape(T other) {
        std::stringstream ss;
        ss << other;
        return ss.str();
    }

    template<typename T>
    void fill(T fill, int num) {
        for (; num > 0; --num) {
            writer.stream << fill;
        }
    }

    Writer<Stream> &writer;
    std::string name;
    std::size_t indent;
    bool has_long_content;
    bool is_in_start_tag;
};

template<typename Stream>
Writer<Stream> make_writer(Stream &stream, bool declaration=true, bool format=false, std::size_t indent=4, const std::string &indent_str=" ") {
    return Writer<Stream>(stream, declaration, format, indent, indent_str);
}

}
}