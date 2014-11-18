#pragma once

#include <stdexcept>

#include "json.h"

namespace gcm {
namespace json {
namespace validate {

class str_const {
public:
    template<std::size_t N>
    constexpr str_const(const char (&a)[N]): str(a), size(N)
    {}

    constexpr char operator[](std::size_t n) {
        return n < sz ? str[n] : throw std::out_of_range("");
    }

    constexpr std::size_t size() {
        return sz;
    }

protected:
    const char *const str;
    const std::size_t sz;
};

template<typename Params, typename Result>
class MethodDefinition {
protected:
    bool validateParams(Array &params) {
        return params.validate(params);
    }

    bool validateResult(std::shared_ptr<Value> &result) {
        return result.validate(result);
    }

public:
    Params params;
    Result result;
};


template<ValueType Type>
class Value {
public:
    template<std::size_t N>
    constexpr Value(const char (&a)[N]): name(a), sz(N)
    {}

    bool validate(std::shared_ptr<Value> value, const std::string &myname = nullptr) {
        if (value.get_type() != Type) {
            if (!myname.empty()) {
                throw TypeException("Param " + name + "." + myname + " must be " + str_type(Type) + ", instead got " + str_type(value.get_type()));
            } else {
                throw TypeException("Param " + name + " must be " + str_type(Type) + ", instead got " + str_type(value.get_type()));
            }
        }
        return true;
    }

protected:
    const char const *name;
    const std::size_t sz;
};

class Int: public Value<ValueType::Int>
{};

class Double: public Value<ValueType::Double>
{};

class String: public Value<ValueType::String>
{};

class Bool: public Value<ValueType::Bool>
{};

class Null: public Value<ValueType::Null>
{};

template<typename T>
class Array: public Value<ValueType::Array> {
public:
    bool validate(std::shared_ptr<Value> value) {
        auto res = Value<ValueType::Array>.validate(value);
        if (res) {
            auto &a = to<json::Array>(value);
            int index;
            foreach (auto &i: a) {
                res &= arrayItem.validate(i, std::to_string(index));
                ++index;
            }
        }

        return res;
    }

protected:
    T arrayItem;
};


MethodDefinition<
    ParamDefinition<
        Optional<Int>>("count"),
        Optional<Array<String>>>("items"),
        Null<Object<Int("limit"), Int("offset")>>("filter")
    >,
    ResultDefinition<
        Int<"status">,
        String<"statusMessage">
    >
>();

}
}
}
