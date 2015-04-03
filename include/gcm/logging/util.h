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

#include <initializer_list>

#include <gcm/config/config.h>

namespace gcm {
namespace logging {
namespace util {

class Levels {
public:
    bool critical;
    bool error;
    bool warning;
    bool info;
    bool debug;

    Levels():
        critical(false),
        error(false),
        warning(false),
        info(false),
        debug(false)
    {}

    Levels(const Levels &) = default;
    Levels(Levels &&) = default;

    Levels(bool c, bool e, bool w, bool i, bool d):
        critical(c), error(e), warning(w), info(i), debug(d)
    {}

    Levels(config::Value &&l):
        critical(l.hasItem("critical") ? l["critical"].asBool() : false),
        error(l.hasItem("error") ? l["error"].asBool() : false),
        warning(l.hasItem("warning") ? l["warning"].asBool() : false),
        info(l.hasItem("info") ? l["info"].asBool() : false),
        debug(l.hasItem("debug") ? l["debug"].asBool() : false)
    {}

    void update(config::Value &l) {
        if (l.hasItem("critical")) {
            critical = l["critical"].asBool();
        }

        if (l.hasItem("error")) {
            error = l["error"].asBool();
        }

        if (l.hasItem("warning")) {
            warning = l["warning"].asBool();
        }

        if (l.hasItem("info")) {
            info = l["info"].asBool();
        }

        if (l.hasItem("debug")) {
            debug = l["debug"].asBool();
        }
    }

    void apply(Handler &handler) {
        handler.set_type(MessageType::Critical, critical);
        handler.set_type(MessageType::Error, error);
        handler.set_type(MessageType::Warning, warning);
        handler.set_type(MessageType::Info, info);
        handler.set_type(MessageType::Debug, debug);
    }
};

class LoggingSettings {
public:
    bool stderr;
    std::string file;
    Levels levels;
    const std::string appname;

    LoggingSettings(): stderr(false) {}
    LoggingSettings(const std::string &appname): stderr(false), appname(appname) {}
    LoggingSettings(const LoggingSettings &) = default;
    LoggingSettings(LoggingSettings &&) = default;

    LoggingSettings(const std::string &appname, config::Value &s):
        stderr{s.hasItem("stderr") && s["stderr"].asBool()},
        file{(s.hasItem("file") && !s["file"].isNull() && s["file"].asBool())
            ?s["file"].asString()
            :""},
        levels(s.hasItem("levels") && s.isStruct() ? s["levels"] : config::Value{config::StructType{}}),
        appname(appname)
    {}

    LoggingSettings(const LoggingSettings &other, config::Value &s):
        LoggingSettings(other)
    {
        if (s.hasItem("stderr")) {
            stderr = s["stderr"].asBool();
        }

        if (s.hasItem("file")) {
            file = s["file"].asString();
        }

        if (s.hasItem("levels") && s["levels"].isStruct()) {
            levels.update(s["levels"]);
        }
    }

    void apply(Logger &logger) {
        auto formatter = Formatter(
            field::Date(), " ",
            "[", field::Pid(), "] ",
            appname, ".",
            field::Name(), " ",
            field::Severenity(), ": ",
            field::Message(),
            " {", field::File(), ":", field::Line(), "}"
        );

        // Clear existing handlers.
        logger.get_handlers(true).clear();

        if (stderr) {
            logger.add_handler(StdErrHandler(formatter));
        }

        if (!file.empty()) {
            logger.add_handler(FileHandler(file, formatter));
        }

        for (auto &handler: logger.get_handlers(true)) {
            levels.apply(*handler);
        }
    }
};

/**
 * Default logging not related to configuration.
 * Can get optional list of MessageType severenity levels to log.
 */
void setup_logging(const std::string &appname, std::initializer_list<MessageType> levels) {
    auto &log = getLogger("");
    log.add_handler(StdErrHandler(Formatter(
            field::Date(), " [", field::Pid(), "] ", std::string(appname), ".", field::Name(), " ", 
            field::Severenity(), ": ", field::Message(), " {",
            field::File(), ":", field::Line(), "}"
    )));

    for (auto &handler: log.get_handlers(true)) {
        for (auto level: levels) {
            handler->set_type(level, true);
        }
    }
}

void setup_logging(const std::string &appname) {
    setup_logging(appname, std::initializer_list<MessageType>{});
}

/**
 * Read and apply configuration from config file.
 */
void setup_logging(const std::string &appname, config::Config &cfg) {
    auto &root = getLogger("");

    if (cfg.hasItem("logging") && cfg["logging"].isStruct()) {
        auto &logcfg = cfg["logging"];
        LoggingSettings default_settings(appname, logcfg);
        default_settings.apply(root);

        if (logcfg.hasItem("facilities") && logcfg["facilities"].isArray()) {
            for (auto &f: logcfg["facilities"].asArray()) {
                if (f.isStruct() && f.hasItem("identifier")) {
                    LoggingSettings facility_settings(default_settings, f);

                    auto &log = getLogger(f["identifier"].asString());
                    facility_settings.apply(log);
                }
            }
        }
    }
}

} // namespace util
} // namespace logging
} // namespace gcm
