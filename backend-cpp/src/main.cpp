#include "crow.h"
#include "crow/middlewares/cookie_parser.h"
#include "crow/middlewares/session.h"
#include <cpr/cpr.h>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <mutex>

// --- Credenciales (hardcodeadas por ahora, mover a hash + BD más adelante) ---
static const std::string ADMIN_USER = "admin";
static const std::string ADMIN_PASS = "admin";

// --- Estado en memoria de cada dispositivo ---
static std::unordered_map<std::string, bool> deviceState;
static std::mutex stateMutex;

using Session = crow::SessionMiddleware<crow::InMemoryStore>;

// Lee config.json y lo devuelve como crow::json::rvalue
crow::json::rvalue loadConfig()
{
    std::ifstream file("config.json");
    std::stringstream buffer;
    buffer << file.rdbuf();
    return crow::json::load(buffer.str());
}

// Añade las cabeceras CORS necesarias para peticiones con credenciales
void addCorsHeaders(const crow::request& req, crow::response& res)
{
    std::string origin = req.get_header_value("Origin");
    if (!origin.empty())
        res.add_header("Access-Control-Allow-Origin", origin);
    res.add_header("Access-Control-Allow-Credentials", "true");
    res.add_header("Access-Control-Allow-Headers", "Content-Type");
    res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
}

int main()
{
    crow::App<crow::CookieParser, Session> app{
        Session{
            crow::CookieParser::Cookie("session").path("/").max_age(60 * 60 * 8), // 8h
            20,
            crow::InMemoryStore{}}};

    // ---------- Preflight OPTIONS genérico ----------
    auto handleOptions = [](const crow::request& req, crow::response& res) {
        addCorsHeaders(req, res);
        res.code = 204;
        res.end();
    };

    CROW_ROUTE(app, "/api/login").methods("OPTIONS"_method)(handleOptions);
    CROW_ROUTE(app, "/api/logout").methods("OPTIONS"_method)(handleOptions);
    CROW_ROUTE(app, "/api/widgets").methods("OPTIONS"_method)(handleOptions);
    CROW_ROUTE(app, "/api/toggle/<string>").methods("OPTIONS"_method)(handleOptions);

    // ---------- POST /api/login ----------
    CROW_ROUTE(app, "/api/login").methods("POST"_method)(
        [&app](const crow::request& req, crow::response& res) {
            addCorsHeaders(req, res);
            auto body = crow::json::load(req.body);
            if (!body) { res.code = 400; res.write("{\"error\":\"JSON inválido\"}"); res.end(); return; }

            std::string username = body["username"].s();
            std::string password = body["password"].s();

            if (username == ADMIN_USER && password == ADMIN_PASS) {
                auto& session = app.get_context<Session>(req);
                session.set("user", username);
                res.code = 200;
                res.write("{\"ok\":true,\"user\":\"" + username + "\"}");
            } else {
                res.code = 401;
                res.write("{\"error\":\"Usuario o contraseña incorrectos\"}");
            }
            res.end();
        });

    // ---------- POST /api/logout ----------
    CROW_ROUTE(app, "/api/logout").methods("POST"_method)(
        [&app](const crow::request& req, crow::response& res) {
            addCorsHeaders(req, res);
            auto& session = app.get_context<Session>(req);
            session.remove("user");
            res.code = 200;
            res.write("{\"ok\":true}");
            res.end();
        });

    // ---------- GET /api/session ----------
    CROW_ROUTE(app, "/api/session").methods("GET"_method)(
        [&app](const crow::request& req, crow::response& res) {
            addCorsHeaders(req, res);
            auto& session = app.get_context<Session>(req);
            std::string user = session.get("user", std::string(""));
            if (user.empty())
                res.write("{\"user\":null}");
            else
                res.write("{\"user\":\"" + user + "\"}");
            res.end();
        });

    // ---------- GET /api/widgets ----------
    CROW_ROUTE(app, "/api/widgets").methods("GET"_method)(
        [&app](const crow::request& req, crow::response& res) {
            addCorsHeaders(req, res);
            auto& session = app.get_context<Session>(req);
            if (session.get("user", std::string("")).empty()) {
                res.code = 401;
                res.write("{\"error\":\"No autenticado\"}");
                res.end();
                return;
            }

            auto config = loadConfig();
            crow::json::wvalue result;
            std::vector<crow::json::wvalue> widgets;

            std::lock_guard<std::mutex> lock(stateMutex);
            for (const auto& d : config["devices"]) {
                crow::json::wvalue w;
                std::string id = d["id"].s();
                w["id"] = id;
                w["label"] = d["label"].s();
                w["state"] = deviceState.count(id) ? deviceState[id] : false;
                widgets.push_back(std::move(w));
            }
            result["widgets"] = std::move(widgets);
            res.write(result.dump());
            res.end();
        });

    // ---------- POST /api/toggle/<id> ----------
    CROW_ROUTE(app, "/api/toggle/<string>").methods("POST"_method)(
        [&app](const crow::request& req, crow::response& res, std::string id) {
            addCorsHeaders(req, res);
            auto& session = app.get_context<Session>(req);
            if (session.get("user", std::string("")).empty()) {
                res.code = 401;
                res.write("{\"error\":\"No autenticado\"}");
                res.end();
                return;
            }

            auto config = loadConfig();
            std::string ip, endpoint;
            bool found = false;
            for (const auto& d : config["devices"]) {
                if (d["id"].s() == id) {
                    ip = d["ip"].s();
                    endpoint = d["endpoint"].s();
                    found = true;
                    break;
                }
            }

            if (!found) {
                res.code = 404;
                res.write("{\"error\":\"Dispositivo '" + id + "' no existe en config.json\"}");
                res.end();
                return;
            }

            bool nextState;
            {
                std::lock_guard<std::mutex> lock(stateMutex);
                nextState = !(deviceState.count(id) ? deviceState[id] : false);
            }

            std::string url = "http://" + ip + endpoint;
            std::string payload = std::string("{\"state\":") + (nextState ? "true" : "false") + "}";

            auto r = cpr::Post(cpr::Url{url},
                                cpr::Body{payload},
                                cpr::Header{{"Content-Type", "application/json"}},
                                cpr::Timeout{3000});

            bool finalState = nextState;
            bool simulated = false;

            if (r.status_code == 200) {
                auto respJson = crow::json::load(r.text);
                if (respJson && respJson.has("state"))
                    finalState = respJson["state"].b();
            } else {
                // ESP32 no responde: modo simulado para poder probar sin hardware
                simulated = true;
            }

            {
                std::lock_guard<std::mutex> lock(stateMutex);
                deviceState[id] = finalState;
            }

            crow::json::wvalue out;
            out["id"] = id;
            out["state"] = finalState;
            if (simulated) out["simulated"] = true;
            res.write(out.dump());
            res.end();
        });

    app.port(3001).multithreaded().run();
}
