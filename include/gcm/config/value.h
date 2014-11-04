#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace gcm {
namespace config {

class Value {
public:
    friend class Parser;

    using IntType = int64_t;
    using DoubleType = double;
    using BoolType = bool;
    using StringType = std::string;
    using ArrayType = std::vector<Value>;
    using StructType = std::vector<std::pair<std::string, Value>>;

    union UValue {
        IntType int_value;
        DoubleType double_value;
        BoolType bool_value;
        StringType string_value;
        ArrayType array_value;
        StructType struct_value;

        UValue() {}
        ~UValue() {}
    };

    enum class Type {
        Null, Int, Double, Bool, String, Array, Struct
    };

    Value(): type(Type::Null), parent(nullptr) {}
    Value(const Value &other): type(Type::Null), parent(other.parent) {
        this->operator=(other);
    }
    Value(Value &&other): type(Type::Null), parent(other.parent) {
        this->operator=(std::forward<Value>(other));
    }
    Value(IntType v): type(Type::Int), parent(nullptr) {
        value.int_value = v;
    }
    Value(DoubleType v): type(Type::Double), parent(nullptr) {
        value.double_value = v;
    }
    Value(BoolType v): type(Type::Bool), parent(nullptr) {
        value.bool_value = v;
    }
    Value(StringType v): type(Type::String), parent(nullptr) {
        new (&value.string_value) StringType;
        value.string_value = v;
    }
    Value(ArrayType v): type(Type::Array), parent(nullptr) {
        new (&value.array_value) ArrayType;
        value.array_value = v;
    }
    Value(StructType v): type(Type::Struct), parent(nullptr) {
        new (&value.struct_value) StructType;
        value.struct_value = v;
    }
    Value(nullptr_t): type(Type::Null), parent(nullptr)
    {}

    IntType asInt() {
        switch (type) {
            case Type::Int: return value.int_value;
            case Type::Double: return value.double_value;
            case Type::String: atoll(value.string_value.c_str());
            case Type::Null: return 0;
            case Type::Bool: return value.bool_value;
            default:
                test_type(Type::Int);
        }
    }

    DoubleType asDouble() {
        switch (type) {
            case Type::Int: return value.int_value;
            case Type::Double: return value.double_value;
            case Type::String: return atof(value.string_value.c_str());
            case Type::Null: return 0.0;
            case Type::Bool: return value.bool_value;
            default:
                test_type(Type::Double);
        }
    }

    BoolType asBool() {
        switch (type) {
            case Type::Int: return value.int_value != 0;
            case Type::Double: return value.double_value != 0.0;
            case Type::String: return !value.string_value.empty();
            case Type::Null: return false;
            case Type::Bool: return value.bool_value;
            default:
                test_type(Type::Bool);
        }
    }

    StringType asString() {
        switch (type) {
            case Type::Int: {
                std::stringstream ss;
                ss << value.int_value;
                return ss.str();
            }

            case Type::Double: {
                std::stringstream ss;
                ss << value.double_value;
                return ss.str();
            }

            case Type::Null: return "";

            case Type::String: return value.string_value;
            case Type::Bool: return (value.bool_value ? "true" : "false");
            default:
                test_type(Type::String);
        }
    }

    ArrayType &asArray() {
        test_type(Type::Array);
        return value.array_value;
    }

    StructType &asStruct() {
        test_type(Type::Struct);
        return value.struct_value;
    }

    bool isNull() { return type == Type::Null; }
    Type getType() { return type; }

    Value &operator[](size_t index) {
        auto &a = asArray();
        if (index < a.size()) {
            return a[index];
        } else {
            throw std::runtime_error(identifier + ": Array index out of bounds.");
        }
    }

    Value &operator[](const std::string &index) {
        auto &s = asStruct();

        for (auto &item: s) {
            if (item.first == index) {
                return item.second;
            }
        }

        throw std::runtime_error(identifier + ": No such option " + index + ".");
    }

    bool hasItem(const std::string &index) {
        //return val.asStruct().find(index) != val.asStruct().end();
        auto &s = asStruct();
        for (auto &item: s) {
            if (item.first == index) return true;
        }
        return false;
    }

    template<typename T>
    T &get(const std::string &index, T &def) {
        auto &s = asStruct();
        for (auto &item: s) {
            if (item.first == index) {
                return item.second;
            }
        }
        return def;
    }

    auto getAll(const std::string &index) {
        auto &s = asStruct();
        std::vector<Value *> out;
        for (auto &item: s) {
            if (item.first == index) {
                out.emplace_back(&item.second);
            }
        }
        return out;
    }

    Value &operator=(IntType v) {
        destruct();
        value.int_value = v;
        type = Type::Int;
        return *this;
    }

    Value &operator=(DoubleType v) {
        destruct();
        value.double_value = v;
        type = Type::Double;
        return *this;
    }

    Value &operator=(BoolType v) {
        destruct();
        value.bool_value = v;
        type = Type::Bool;
        return *this;
    }

    Value &operator=(StringType v) {
        destruct();
        new (&value.string_value) StringType;
        value.string_value = v;
        type = Type::String;
        return *this;
    }

    Value &operator=(ArrayType v) {
        destruct();
        new (&value.array_value) ArrayType;
        value.array_value = v;
        type = Type::Array;
        return *this;
    }

    Value &operator=(StructType v) {
        destruct();

        new (&value.struct_value) StructType;
        value.struct_value = v;
        type = Type::Struct;
        return *this;
    }

    Value &operator=(Value &&other) {
        destruct();

        type = other.type;
        identifier = other.identifier;
        parent = other.parent;

        switch (type) {
            case Type::Int: value.int_value = other.value.int_value; break;
            case Type::Double: value.double_value = other.value.double_value; break;
            case Type::Bool: value.bool_value = other.value.bool_value; break;
            case Type::Null: break;

            case Type::String: {
                new (&value.string_value) StringType;
                value.string_value = std::move(other.value.string_value);
                break;
            }
            
            case Type::Array: {
                new (&value.array_value) ArrayType;
                value.array_value = std::move(other.value.array_value);
                break;
            }

            case Type::Struct: {
                new (&value.struct_value) StructType;
                value.struct_value = std::move(other.value.struct_value);
                break;
            }
        }

        return *this;
    }

    Value &operator=(const Value &other) {
        destruct();

        type = other.type;
        identifier = other.identifier;
        parent = other.parent;

        switch (type) {
            case Type::Int: value.int_value = other.value.int_value; break;
            case Type::Double: value.double_value = other.value.double_value; break;
            case Type::Bool: value.bool_value = other.value.bool_value; break;
            case Type::Null: break;

            case Type::String: {
                new (&value.string_value) StringType;
                value.string_value = other.value.string_value;
                break;
            }
            
            case Type::Array: {
                new (&value.array_value) ArrayType;
                value.array_value = other.value.array_value;
                break;
            }

            case Type::Struct: {
                new (&value.struct_value) StructType;
                value.struct_value = other.value.struct_value;
                break;
            }
        }

        return *this;
    }

    ~Value() {
        destruct();
    }

protected:
    UValue value;
    Type type;
    Value *parent;
    std::string identifier;

    void destruct() {
        switch (type) {
            case Type::String: value.string_value.~StringType(); break;
            case Type::Array: value.array_value.~ArrayType(); break;
            case Type::Struct: value.struct_value.~StructType(); break;
            default: break;
        }
        type = Type::Null;
    }

    std::string strtype(Type type) {
        switch (type) {
            case Type::Int: return "int";
            case Type::Double: return "double";
            case Type::Bool: return "bool";
            case Type::String: return "string";
            case Type::Null: return "null";
            case Type::Array: return "array";
            case Type::Struct: return "struct";
        }
        throw std::runtime_error(identifier + ": Invalid type.");
    }

    void test_type(Type type) {
        if (getType() != type) {
            throw std::runtime_error(identifier + ": Invalid conversion from " + strtype(getType()) + " to " + strtype(type) + ".");
        }
    }
};


} // namespace config
} // namespace gcm
