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

    virtual ValueType get_type() {
        return Value::type;
    }

    bool is_null() {
        return get_type() == ValueType::Null;
    }

    virtual std::string to_string() {
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

    ValueType get_type() {
        return Object::type;
    }

    bool has_key(const std::string &index) {
        return find(index) != end();
    }

    std::string to_string() {
        std::stringstream ss;

        ss << "{";

        for (auto it = begin(); it != end(); ++it) {
            if (it != begin()) {
                ss << ",";
            }

            ss << "\"" << it->first << "\":" << it->second->to_string();
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

    ValueType get_type() {
        return Array::type;
    }

    std::string to_string() {
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

    ValueType get_type() {
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

    template<typename I>
    PlainValue &operator=(I &&v) {
        value = std::forward<std::decay_t<I>>(v);
        return *this;
    }

    PlainValue &operator=(const PlainValue &other) = default;
    PlainValue &operator=(PlainValue &&other) = default;

    std::string to_string() {
        return get_string<T>();
    }

protected:
    template<typename I>
    std::enable_if_t<std::is_arithmetic<I>::value && !std::is_same<I, bool>::value, std::string>
    get_string() {
        return std::to_string(value);
    }

    template<typename I>
    std::enable_if_t<std::is_same<I, bool>::value, std::string>
    get_string() {
        if (value) {
            return "true";
        } else {
            return "false";
        }
    }

    template<typename I>
    std::enable_if_t<std::is_same<I, std::string>::value, std::string>
    get_string() {
        return "\"" + value + "\"";
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

inline std::shared_ptr<Value> make_object() {
    return std::make_shared<Object>();
}

inline std::shared_ptr<Value> make_array() {
    return std::make_shared<Array>();
}

inline std::shared_ptr<Value> make_array(std::initializer_list<std::shared_ptr<Value>> values) {
    return std::make_shared<Array>(values);
}

inline std::shared_ptr<Value> make_int(int value) {
    return std::make_shared<Int>(value);
}

inline std::shared_ptr<Value> make_double(double value) {
    return std::make_shared<Double>(value);
}

inline std::shared_ptr<Value> make_string(std::string &&value) {
    return std::make_shared<String>(std::forward<std::string>(value));
}

inline std::shared_ptr<Value> make_string(const std::string &value) {
    return std::make_shared<String>(value);
}

inline std::shared_ptr<Value> make_bool(bool value) {
    return std::make_shared<Bool>(value);
}

} // namespace json
} // namespace gcm
