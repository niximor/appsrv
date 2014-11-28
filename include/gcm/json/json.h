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
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include <map>
#include <memory>
#include <iomanip>

namespace gcm {
namespace json {

class Exception: public std::runtime_error {
public:
    Exception(std::string message): std::runtime_error(message)
    {}
};

class TypeException: public Exception {
public:
    TypeException(std::string message): Exception(message)
    {}
};

enum class ValueType {
    Null, Object, Array, Int, Double, String, Bool
};

inline std::string str_type(ValueType val) {
    switch (val) {
        case ValueType::Null: return "json::Null";
        case ValueType::Object: return "json::Object";
        case ValueType::Array: return "json::Array";
        case ValueType::Int: return "json::Int";
        case ValueType::Double: return "json::Double";
        case ValueType::String: return "json::String";
        case ValueType::Bool: return "json::Bool";
        default: return "<unknown type>";
    }
}

class Value {
public:
    virtual ~Value();

    static constexpr const ValueType type = ValueType::Null;

    virtual ValueType get_type() const {
        return Value::type;
    }

    bool is_null() const {
        return get_type() == ValueType::Null;
    }

    virtual std::string to_string() const {
        return "null";
    }
};

class Object: public Value, public std::map<std::string, std::shared_ptr<Value>> {
public:
    Object(): Value(), std::map<std::string, std::shared_ptr<Value>>()
    {}

    Object(const Object &) = default;
    Object(Object &&) = default;

    static constexpr const ValueType type = ValueType::Object;

    ValueType get_type() const {
        return Object::type;
    }

    bool has_key(const std::string &index) const {
        return find(index) != end();
    }

    std::string to_string() const {
        std::stringstream ss;

        ss << "{";

        for (auto it = begin(); it != end(); ++it) {
            if (it != begin()) {
                ss << ",";
            }

            if (it->second) {
                ss << "\"" << it->first << "\":" << it->second->to_string();
            } else {
                ss << "\"" << it->first << "\":null";
            }
        }

        ss << "}";

        return ss.str();
    }
};

class Array: public Value, public std::vector<std::shared_ptr<Value>> {
public:
    Array(): Value(), std::vector<std::shared_ptr<Value>>()
    {}

    Array(std::initializer_list<std::shared_ptr<Value>> values): Value(), std::vector<std::shared_ptr<Value>>(values)
    {}

    Array(const Array &) = default;
    Array(Array &&) = default;

    static constexpr const ValueType type = ValueType::Array;

    ValueType get_type() const {
        return Array::type;
    }

    std::string to_string() const {
        std::stringstream ss;

        ss << "[";

        for (auto it = begin(); it != end(); ++it) {
            if (it != begin()) {
                ss << ",";
            }

            ss << (*it)->to_string();
        }

        ss << "]";

        return ss.str();
    }
};

template<typename T, ValueType VT>
class PlainValue: public Value {
public:
    template<typename I>
    PlainValue(I &&value): Value(), value(std::forward<std::decay_t<I>>(value))
    {}

    template<typename I>
    PlainValue(const I &value): Value(), value(value)
    {}

    static constexpr const ValueType type = VT;

    ValueType get_type() const {
        return VT;
    }

    PlainValue(const PlainValue &) = default;
    PlainValue(PlainValue &&) = default;

    operator const T() const {
        return value;
    }

    operator T&() {
        return value;
    }

    T& get_value() {
        return value;
    }

    const T &get_value() const {
        return value;
    }

    template<typename I>
    PlainValue &operator=(I &&v) {
        value = std::forward<std::decay_t<I>>(v);
        return *this;
    }

    PlainValue &operator=(const PlainValue &other) = default;
    PlainValue &operator=(PlainValue &&other) = default;

    std::string to_string() const {
        return get_string<T>();
    }

protected:
    template<typename I>
    std::enable_if_t<std::is_arithmetic<I>::value && !std::is_same<I, bool>::value, std::string>
    get_string() const {
        return std::to_string(value);
    }

    template<typename I>
    std::enable_if_t<std::is_same<I, bool>::value, std::string>
    get_string() const {
        if (value) {
            return "true";
        } else {
            return "false";
        }
    }

    template<typename I>
    std::enable_if_t<std::is_same<I, std::string>::value, std::string>
    get_string() const {
        std::stringstream ss;
        ss << '"';

        bool is_unicode = false;
        for (char ch: value) {
            if (is_unicode) {
                ss << "\\u" << std::setw(2) << std::setfill('0') << std::hex << (int)ch;
                continue;
            }

            if (std::isprint(ch)) {
                ss << ch;
            } else if (ch == '\"' || ch == '\\' || ch == '/') {
                ss << '\\' << ch;
            } else if (ch == '\b') {
                ss << "\\b";
            } else if (ch == '\f') {
                ss << "\\f";
            } else if (ch == '\n') {
                ss << "\\n";
            } else if (ch == '\r') {
                ss << "\\r";
            } else if (ch == '\t') {
                ss << "\\t";
            } else {
                ss << "\\u" << std::setw(2) << std::setfill('0') << std::hex << (int)ch;
                is_unicode = true;
            }
        }

        ss << '"';

        return ss.str();
    }

    T value;
};

using Int = PlainValue<int64_t, ValueType::Int>;
using Double = PlainValue<double, ValueType::Double>;
using String = PlainValue<std::string, ValueType::String>;
using Bool = PlainValue<bool, ValueType::Bool>;

template<typename T>
T &to(Value &value) {
    T *val = dynamic_cast<T *>(&value);
    if (!val) {
        throw TypeException("Conversion from " + str_type(value.get_type()) + " to " + str_type(T::type) + " requested.");
    }
    return *val;
}

template<typename T>
T &to(std::shared_ptr<Value> &value) {
    return to<T>(*value);
}

inline std::shared_ptr<Value> make_null() {
    return std::make_shared<Value>();
}

template<typename Head, typename... Tail>
inline std::shared_ptr<Value> make_object(Head head, Tail... tail) {
    std::shared_ptr<Value> obj = make_object(tail...);
    to<Object>(obj)[head.first] = head.second;
    return obj;
}

template<typename Head>
inline std::shared_ptr<Value> make_object(Head head) {
    std::shared_ptr<Value> obj = std::make_shared<Object>();
    to<Object>(obj)[head.first] = head.second;
    return obj;
}

using JsonValue = std::shared_ptr<Value>;

inline JsonValue make_object() {
    return std::make_shared<Object>();
}

inline JsonValue make_array() {
    return std::make_shared<Array>();
}

inline JsonValue make_array(std::initializer_list<std::shared_ptr<Value>> values) {
    return std::make_shared<Array>(values);
}

template<typename T>
std::enable_if_t<std::is_convertible<T, int>::value, JsonValue>
make_int(T value) {
    return std::make_shared<Int>(static_cast<int>(value));
}

template<typename T>
std::enable_if_t<std::is_enum<T>::value, JsonValue>
make_int(T value) {
    return std::make_shared<Int>(static_cast<int>(value));
}

template<typename T>
std::enable_if_t<std::is_convertible<T, double>::value, JsonValue>
make_double(T value) {
    return std::make_shared<Double>(static_cast<double>(value));
}

inline JsonValue make_string(std::string &&value) {
    return std::make_shared<String>(std::forward<std::string>(value));
}

inline JsonValue make_string(const std::string &value) {
    return std::make_shared<String>(value);
}

inline JsonValue make_bool(bool value) {
    return std::make_shared<Bool>(value);
}

} // namespace json
} // namespace gcm
