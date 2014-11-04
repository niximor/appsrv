#pragma once

#include <iostream>
#include <fstream>

#include "value.h"
#include "parser.h"

#include <gcm/logging/logging.h>

namespace gcm {
namespace config {

class Config {
public:
    Config(const std::string &file): val(Value::StructType()), log(gcm::logging::getLogger("GCM.ConfigParser")) {
        std::ifstream f(file, std::ios_base::in);
        std::string contents((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

        Parser p(val);
        try {
            p.parse(contents.begin(), contents.end());
        } catch (parse_error &e) {
            ERROR(log) << file << " " << e.what();
        }
    }

    Value &operator[](const std::string &index) {
        return val[index];
    }

    bool hasItem(const std::string &index) {
        return val.asStruct().find(index) != val.asStruct().end();
    }

    template<typename T>
    T &get(const std::string &index, T &def) {
        auto &s = val.asStruct();
        auto it = s.find(index);
        if (it != s.end()) {
            return it->second;
        } else {
            return def;
        }
    }

private:
    Value val;
    gcm::logging::Logger &log;
};

}
}
