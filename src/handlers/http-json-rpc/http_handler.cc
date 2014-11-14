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

#include <gcm/appsrv/interface.h>
#include <gcm/socket/socket.h>
#include <gcm/socket/http.h>
#include <gcm/logging/logging.h>
#include <gcm/thread/pool.h>
#include <gcm/json/json.h>
#include <gcm/json/rpc.h>
#include <gcm/json/parser.h>

#include <vector>
#include <memory>
#include <stdexcept>

namespace s = gcm::socket;
namespace l = gcm::logging;

using namespace gcm::socket::http;

class JsonHttpHandler: public gcm::appsrv::Handler {
protected:
    gcm::appsrv::ServerApi &api;
    gcm::json::rpc::Rpc json;
    gcm::logging::Logger &log;

public:
    JsonHttpHandler(gcm::appsrv::ServerApi &api):
        api(api),
        log(l::getLogger(api.handler_name))
    {}

    template<typename R, typename I>
    void process_body(R &response, I begin, I end) {
        try {
            try {
                std::vector<std::shared_ptr<gcm::json::rpc::Promise>> promises;

                std::shared_ptr<gcm::json::Value> call;
                I begin1 = begin;
                I end1 = end;
                while ((call = gcm::json::parse(begin1, end1)) != nullptr) {
                    end1 = end;

                    auto &obj = gcm::json::to<gcm::json::Object>(call);
                    auto &id = obj["id"];
                    auto &method = obj["method"];
                    auto &params = obj["params"];

                    if (method->get_type() == gcm::json::ValueType::String) {
                        promises.push_back(json.add_work(
                            id,
                            std::string(gcm::json::to<gcm::json::String>(method)),
                            (params->get_type() == gcm::json::ValueType::Array)
                                ? gcm::json::to<gcm::json::Array>(params)
                                : gcm::json::Array()
                        ));
                    }
                }

                if (begin1 != end) {
                    auto pos = gcm::parser::calc_line_column(begin, begin1);
                    ERROR(log) << "Bad request. Parse error at " << pos.first << " column " << pos.second;
                    DEBUG(log) << "Request was: " << std::string(begin, end);

                    std::string spaces(pos.second - 1, ' ');
                    DEBUG(log) << "             " << spaces << "^";
                }

                bool know_length = promises.size() < 2;

                gcm::json::rpc::wait_all(promises, [&](gcm::json::rpc::Promise &p){
                    auto resp = p.get();
                    std::string out{resp->to_string()};

                    if (know_length) {
                        response.set_header("Content-Length", std::to_string(out.size()));
                    }

                    response << out;
                    return true;
                });
            } catch (gcm::json::rpc::RpcException &e) {
                throw;
            } catch (gcm::json::Exception &e) {
                throw gcm::json::rpc::RpcException(-32603, e.what());
            }
        } catch (gcm::json::rpc::RpcException &e) {
            response << e.to_json()->to_string();

            throw;
        } catch (std::exception &e) {
            auto obj = gcm::json::Object();
            obj["id"] = gcm::json::make_null();
            auto err = gcm::json::to<gcm::json::Object>(obj["error"] = gcm::json::make_object());
            err["code"] = gcm::json::make_int(-32603);
            err["message"] = gcm::json::make_string(e.what());

            response << obj.to_string();

            throw;
        }
    }

    void handle(s::ConnectedSocket<s::AnyIpAddress> &&client) {
        auto &addr = client.get_client_address();

        DEBUG(log) << "Handle client " << addr.get_ip() << ":" << addr.get_port() << ".";

        try {
            client << s::ascii;

            HttpRequest req;
            req.parse(client);

            auto response = req.get_response(client);
            response.set_header("Content-Type", "application/json");

            if (!req.has_header("Content-Length")) {
                throw HttpException(400);
            }

            std::string body;
            body.reserve(std::stoi(req["Content-Length"]));

            client >> body;

            process_body(response, body.begin(), body.end());

            /*response << "Hello world!<br />";
            response << "Interface " << api.handler_name << " statistics: <br />";
            response << "Total requests: " << api.handler_stats.req_received << "<br />";
            response << "Handled: " << api.handler_stats.req_handled << "<br />";
            response << "Error: " << api.handler_stats.req_error << "<br />";*/
        } catch (HttpException &e) {
            e.write(client);
            ERROR(log) << addr.get_ip() << ":" << addr.get_port() << " " << e.get_status() << " " << e.what();
        } catch (std::exception &e) {
            ERROR(log) << addr.get_ip() << ":" << addr.get_port() << " " << e.what();
        }

        DEBUG(log) << "Client " << addr.get_ip() << ":" << addr.get_port() << " handled.";
    }
};

extern "C" {

void *init(void *ptr_api) {
    gcm::appsrv::ServerApi *api = (gcm::appsrv::ServerApi *)ptr_api;
    return new JsonHttpHandler(*api);
}

void stop(void *handler) {
    delete (JsonHttpHandler *)handler;
}

} // enum C
