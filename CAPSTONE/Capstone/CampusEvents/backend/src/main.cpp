#include "httplib.h"
#include "database.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <mutex>

// Simple in-memory session tokens (userId -> token) for demo; production uses JWT
static std::map<std::string, int> g_tokens; // token -> userId
static std::mutex g_token_mtx;

static Config g_cfg;
static Database* g_db = nullptr;

// Helpers
static std::string getBearer(const httplib::Request& req) {
    auto it = req.headers.find("Authorization");
    if (it == req.headers.end()) return "";
    const std::string& v = it->second;
    if (v.rfind("Bearer ", 0) == 0) return v.substr(7);
    return "";
}

static std::optional<User> requireAuth(const httplib::Request& req, httplib::Response& res) {
    std::string token = getBearer(req);
    if (token.empty()) {
        res.status = 401;
        res.set_content(jsonError("UNAUTHORIZED", "Missing or invalid token"), "application/json");
        return std::nullopt;
    }
    std::lock_guard<std::mutex> lock(g_token_mtx);
    auto it = g_tokens.find(token);
    if (it == g_tokens.end()) {
        res.status = 401;
        res.set_content(jsonError("UNAUTHORIZED", "Invalid token"), "application/json");
        return std::nullopt;
    }
    auto user = g_db->findUserById(it->second);
    if (!user) {
        res.status = 401;
        res.set_content(jsonError("UNAUTHORIZED", "User not found"), "application/json");
        return std::nullopt;
    }
    return user;
}

static bool requireRole(const User& user, const std::string& role, httplib::Response& res) {
    if (user.role != role && !(role == "ORGANIZER" && user.role == "SUPER_ADMIN")) {
        res.status = 403;
        res.set_content(jsonError("FORBIDDEN", "Insufficient permissions"), "application/json");
        return false;
    }
    return true;
}

static std::string eventToJson(const Event& e) {
    std::ostringstream os;
    os << "{\"id\":" << e.id
       << ",\"title\":\"" << escapeJson(e.title) << "\""
       << ",\"type\":\"" << escapeJson(e.type) << "\""
       << ",\"theme\":\"" << escapeJson(e.theme) << "\""
       << ",\"description\":\"" << escapeJson(e.description) << "\""
       << ",\"venue\":\"" << escapeJson(e.venue) << "\""
       << ",\"start_at\":\"" << escapeJson(e.start_at) << "\""
       << ",\"end_at\":\"" << escapeJson(e.end_at) << "\""
       << ",\"seat_limit\":" << e.seat_limit
       << ",\"registered_count\":" << e.registered_count
       << ",\"status\":\"" << escapeJson(e.status) << "\""
       << ",\"review_reason\":\"" << escapeJson(e.review_reason) << "\""
       << ",\"created_by\":" << e.created_by
       << ",\"created_at\":\"" << escapeJson(e.created_at) << "\""
       << ",\"updated_at\":\"" << escapeJson(e.updated_at) << "\"}";
    return os.str();
}

static std::string userToJson(const User& u) {
    std::ostringstream os;
    os << "{\"id\":" << u.id
       << ",\"name\":\"" << escapeJson(u.name) << "\""
       << ",\"email\":\"" << escapeJson(u.email) << "\""
       << ",\"role\":\"" << escapeJson(u.role) << "\""
       << ",\"department\":\"" << escapeJson(u.department) << "\"}";
    return os.str();
}

int main() {
    g_cfg = loadConfig();
    Database db(g_cfg.db_path);
    if (!db.open()) {
        std::cerr << "Failed to open database at " << g_cfg.db_path << std::endl;
        return 1;
    }
    g_db = &db;

    httplib::Server svr;

    // CORS
    svr.set_default_headers({
        {"Access-Control-Allow-Origin", g_cfg.cors_origin},
        {"Access-Control-Allow-Methods", "GET, POST, PATCH, PUT, DELETE, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type, Authorization"}
    });
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
    });

    // ---------- Auth ----------
    svr.Post("/api/auth/register", [](const httplib::Request& req, httplib::Response& res) {
        // Minimal body parse: expect {"name":"...","email":"...","password":"...","role":"STUDENT","department":"..."}
        // For simplicity use query-like or fixed; production use proper JSON parser.
        // We accept form or simple key extraction.
        auto name = req.get_param_value("name");
        auto email = req.get_param_value("email");
        auto password = req.get_param_value("password");
        auto role = req.get_param_value("role");
        auto department = req.get_param_value("department");
        if (name.empty() || email.empty() || password.empty()) {
            // Try body as raw for JSON-like
            // Simple extraction
            auto body = req.body;
            auto extract = [&](const std::string& key) -> std::string {
                auto pos = body.find("\"" + key + "\"");
                if (pos == std::string::npos) return "";
                pos = body.find(':', pos);
                if (pos == std::string::npos) return "";
                pos = body.find('"', pos);
                if (pos == std::string::npos) return "";
                auto end = body.find('"', pos + 1);
                if (end == std::string::npos) return "";
                return body.substr(pos + 1, end - pos - 1);
            };
            name = extract("name");
            email = extract("email");
            password = extract("password");
            role = extract("role");
            department = extract("department");
        }
        if (name.empty() || email.empty() || password.empty()) {
            res.status = 400;
            res.set_content(jsonError("VALIDATION", "name, email, password required"), "application/json");
            return;
        }
        if (role.empty()) role = "STUDENT";
        if (g_db->findUserByEmail(email)) {
            res.status = 409;
            res.set_content(jsonError("CONFLICT", "Email already registered"), "application/json");
            return;
        }
        std::string hash = sha256(password);
        int id = g_db->createUser(name, email, hash, role, department);
        if (id < 0) {
            res.status = 500;
            res.set_content(jsonError("SERVER", "Failed to create user"), "application/json");
            return;
        }
        res.set_content(jsonOk(userToJson(*g_db->findUserById(id))), "application/json");
    });

    svr.Post("/api/auth/login", [](const httplib::Request& req, httplib::Response& res) {
        auto email = req.get_param_value("email");
        auto password = req.get_param_value("password");
        if (email.empty() || password.empty()) {
            auto body = req.body;
            auto extract = [&](const std::string& key) -> std::string {
                auto pos = body.find("\"" + key + "\"");
                if (pos == std::string::npos) return "";
                pos = body.find(':', pos);
                if (pos == std::string::npos) return "";
                pos = body.find('"', pos);
                if (pos == std::string::npos) return "";
                auto end = body.find('"', pos + 1);
                if (end == std::string::npos) return "";
                return body.substr(pos + 1, end - pos - 1);
            };
            email = extract("email");
            password = extract("password");
        }
        if (email.empty() || password.empty()) {
            res.status = 400;
            res.set_content(jsonError("VALIDATION", "email and password required"), "application/json");
            return;
        }
        auto user = g_db->findUserByEmail(email);
        if (!user || user->password_hash != sha256(password)) {
            res.status = 401;
            res.set_content(jsonError("AUTH", "Invalid credentials"), "application/json");
            return;
        }
        std::string token = generateToken();
        {
            std::lock_guard<std::mutex> lock(g_token_mtx);
            g_tokens[token] = user->id;
        }
        std::ostringstream os;
        os << "{\"token\":\"" << token << "\",\"user\":" << userToJson(*user) << "}";
        res.set_content(jsonOk(os.str()), "application/json");
    });

    svr.Get("/api/me", [](const httplib::Request& req, httplib::Response& res) {
        auto user = requireAuth(req, res);
        if (!user) return;
        res.set_content(jsonOk(userToJson(*user)), "application/json");
    });

    // ---------- Events ----------
    svr.Get("/api/events", [](const httplib::Request& req, httplib::Response& res) {
        std::string status = req.get_param_value("status");
        std::string type = req.get_param_value("type");
        int created_by = -1;
        if (req.has_param("mine")) {
            auto user = requireAuth(req, res);
            if (!user) return;
            created_by = user->id;
        } else if (status.empty()) {
            // Students see only APPROVED by default
            status = "APPROVED";
        }
        auto events = g_db->listEvents(status, type, created_by);
        std::ostringstream os;
        os << "[";
        for (size_t i = 0; i < events.size(); ++i) {
            if (i) os << ",";
            os << eventToJson(events[i]);
        }
        os << "]";
        res.set_content(jsonOk(os.str()), "application/json");
    });

    svr.Get(R"(/api/events/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        auto e = g_db->getEvent(id);
        if (!e) {
            res.status = 404;
            res.set_content(jsonError("NOT_FOUND", "Event not found"), "application/json");
            return;
        }
        res.set_content(jsonOk(eventToJson(*e)), "application/json");
    });

    svr.Post("/api/events", [](const httplib::Request& req, httplib::Response& res) {
        auto user = requireAuth(req, res);
        if (!user) return;
        if (!requireRole(*user, "ORGANIZER", res)) return;

        auto extract = [&](const std::string& key) -> std::string {
            auto body = req.body;
            auto pos = body.find("\"" + key + "\"");
            if (pos == std::string::npos) return "";
            pos = body.find(':', pos);
            if (pos == std::string::npos) return "";
            // number or string
            while (pos < body.size() && (body[pos] == ':' || body[pos] == ' ')) ++pos;
            if (pos >= body.size()) return "";
            if (body[pos] == '"') {
                auto end = body.find('"', pos + 1);
                return body.substr(pos + 1, end - pos - 1);
            }
            // number
            size_t end = pos;
            while (end < body.size() && (isdigit(body[end]) || body[end] == '-')) ++end;
            return body.substr(pos, end - pos);
        };

        Event e;
        e.title = extract("title");
        e.type = extract("type");
        e.theme = extract("theme");
        e.description = extract("description");
        e.venue = extract("venue");
        e.start_at = extract("start_at");
        e.end_at = extract("end_at");
        e.seat_limit = std::atoi(extract("seat_limit").c_str());
        e.status = "PENDING";
        e.created_by = user->id;

        if (e.title.empty() || e.venue.empty() || e.start_at.empty() || e.seat_limit <= 0) {
            res.status = 400;
            res.set_content(jsonError("VALIDATION", "title, venue, start_at, seat_limit required"), "application/json");
            return;
        }
        if (g_db->hasVenueConflict(e.venue, e.start_at, e.end_at.empty() ? e.start_at : e.end_at)) {
            res.status = 409;
            res.set_content(jsonError("CONFLICT", "Venue already booked for this time slot"), "application/json");
            return;
        }
        int id = g_db->createEvent(e);
        if (id < 0) {
            res.status = 500;
            res.set_content(jsonError("SERVER", "Failed to create event"), "application/json");
            return;
        }
        g_db->addNotification(user->id, "Event \"" + e.title + "\" submitted and is pending review.");
        auto created = g_db->getEvent(id);
        res.status = 201;
        res.set_content(jsonOk(eventToJson(*created)), "application/json");
    });

    svr.Patch(R"(/api/events/(\d+)/review)", [](const httplib::Request& req, httplib::Response& res) {
        auto user = requireAuth(req, res);
        if (!user) return;
        if (!requireRole(*user, "SUPER_ADMIN", res)) return;

        int id = std::stoi(req.matches[1]);
        auto extract = [&](const std::string& key) -> std::string {
            auto body = req.body;
            auto pos = body.find("\"" + key + "\"");
            if (pos == std::string::npos) return "";
            pos = body.find(':', pos);
            if (pos == std::string::npos) return "";
            pos = body.find('"', pos);
            if (pos == std::string::npos) return "";
            auto end = body.find('"', pos + 1);
            return body.substr(pos + 1, end - pos - 1);
        };
        std::string status = extract("status");
        std::string reason = extract("reason");
        if (status != "APPROVED" && status != "REJECTED" && status != "CHANGES_REQUESTED") {
            res.status = 400;
            res.set_content(jsonError("VALIDATION", "status must be APPROVED, REJECTED or CHANGES_REQUESTED"), "application/json");
            return;
        }
        if (reason.empty() && status != "APPROVED") {
            res.status = 400;
            res.set_content(jsonError("VALIDATION", "reason is required for reject / changes"), "application/json");
            return;
        }
        auto ev = g_db->getEvent(id);
        if (!ev) {
            res.status = 404;
            res.set_content(jsonError("NOT_FOUND", "Event not found"), "application/json");
            return;
        }
        if (!g_db->updateEventStatus(id, status, user->id, reason)) {
            res.status = 500;
            res.set_content(jsonError("SERVER", "Update failed"), "application/json");
            return;
        }
        std::string msg = "Your event \"" + ev->title + "\" was " + status;
        if (!reason.empty()) msg += ": " + reason;
        g_db->addNotification(ev->created_by, msg);
        auto updated = g_db->getEvent(id);
        res.set_content(jsonOk(eventToJson(*updated)), "application/json");
    });

    // ---------- Registration ----------
    svr.Post(R"(/api/events/(\d+)/register)", [](const httplib::Request& req, httplib::Response& res) {
        auto user = requireAuth(req, res);
        if (!user) return;
        if (user->role != "STUDENT") {
            res.status = 403;
            res.set_content(jsonError("FORBIDDEN", "Only students can register"), "application/json");
            return;
        }
        int event_id = std::stoi(req.matches[1]);
        auto ev = g_db->getEvent(event_id);
        if (!ev || ev->status != "APPROVED") {
            res.status = 404;
            res.set_content(jsonError("NOT_FOUND", "Event not available for registration"), "application/json");
            return;
        }
        std::string token = generateToken() + "." + hmacSign(std::to_string(event_id) + ":" + std::to_string(user->id), g_cfg.qr_hmac_key);
        int reg_id = g_db->registerUser(event_id, user->id, token);
        if (reg_id == -2) {
            res.status = 409;
            res.set_content(jsonError("FULL", "Event is full"), "application/json");
            return;
        }
        if (reg_id == -3 || reg_id < 0) {
            res.status = 409;
            res.set_content(jsonError("CONFLICT", "Already registered or error"), "application/json");
            return;
        }
        g_db->addNotification(user->id, "Registered for \"" + ev->title + "\". Show QR at venue.");
        auto reg = g_db->getRegistration(reg_id);
        std::ostringstream os;
        os << "{\"id\":" << reg->id << ",\"event_id\":" << reg->event_id
           << ",\"token\":\"" << escapeJson(reg->token) << "\",\"attended\":" << reg->attended << "}";
        res.status = 201;
        res.set_content(jsonOk(os.str()), "application/json");
    });

    svr.Get("/api/registrations/mine", [](const httplib::Request& req, httplib::Response& res) {
        auto user = requireAuth(req, res);
        if (!user) return;
        auto regs = g_db->getRegistrationsForUser(user->id);
        std::ostringstream os;
        os << "[";
        for (size_t i = 0; i < regs.size(); ++i) {
            if (i) os << ",";
            os << "{\"id\":" << regs[i].id << ",\"event_id\":" << regs[i].event_id
               << ",\"token\":\"" << escapeJson(regs[i].token) << "\""
               << ",\"attended\":" << regs[i].attended
               << ",\"event_title\":\"" << escapeJson(regs[i].event_title) << "\""
               << ",\"created_at\":\"" << escapeJson(regs[i].created_at) << "\"}";
        }
        os << "]";
        res.set_content(jsonOk(os.str()), "application/json");
    });

    svr.Get(R"(/api/events/(\d+)/registrations)", [](const httplib::Request& req, httplib::Response& res) {
        auto user = requireAuth(req, res);
        if (!user) return;
        if (!requireRole(*user, "ORGANIZER", res)) return;
        int event_id = std::stoi(req.matches[1]);
        auto regs = g_db->getRegistrationsForEvent(event_id);
        std::ostringstream os;
        os << "[";
        for (size_t i = 0; i < regs.size(); ++i) {
            if (i) os << ",";
            os << "{\"id\":" << regs[i].id << ",\"user_id\":" << regs[i].user_id
               << ",\"student_name\":\"" << escapeJson(regs[i].student_name) << "\""
               << ",\"attended\":" << regs[i].attended
               << ",\"attended_at\":\"" << escapeJson(regs[i].attended_at) << "\"}";
        }
        os << "]";
        res.set_content(jsonOk(os.str()), "application/json");
    });

    // QR: return the token as text/plain or simple SVG QR placeholder
    svr.Get(R"(/api/registrations/(\d+)/qr)", [](const httplib::Request& req, httplib::Response& res) {
        auto user = requireAuth(req, res);
        if (!user) return;
        int id = std::stoi(req.matches[1]);
        auto reg = g_db->getRegistration(id);
        if (!reg || reg->user_id != user->id) {
            res.status = 404;
            res.set_content(jsonError("NOT_FOUND", "Registration not found"), "application/json");
            return;
        }
        // Return token; frontend can render QR with a library
        res.set_content(jsonOk("\"" + escapeJson(reg->token) + "\""), "application/json");
    });

    // Attendance scan
    svr.Post("/api/attendance/scan", [](const httplib::Request& req, httplib::Response& res) {
        auto user = requireAuth(req, res);
        if (!user) return;
        if (!requireRole(*user, "ORGANIZER", res)) return;

        auto extract = [&](const std::string& key) -> std::string {
            auto body = req.body;
            auto pos = body.find("\"" + key + "\"");
            if (pos == std::string::npos) return "";
            pos = body.find(':', pos);
            if (pos == std::string::npos) return "";
            pos = body.find('"', pos);
            if (pos == std::string::npos) return "";
            auto end = body.find('"', pos + 1);
            return body.substr(pos + 1, end - pos - 1);
        };
        std::string token = extract("token");
        if (token.empty()) token = req.get_param_value("token");
        if (token.empty()) {
            res.status = 400;
            res.set_content(jsonError("VALIDATION", "token required"), "application/json");
            return;
        }
        auto reg = g_db->getRegistrationByToken(token);
        if (!reg) {
            res.status = 404;
            res.set_content(jsonError("INVALID_TOKEN", "Token not found or forged"), "application/json");
            return;
        }
        if (reg->attended) {
            res.status = 409;
            res.set_content(jsonError("ALREADY_SCANNED", "Attendance already marked"), "application/json");
            return;
        }
        if (!g_db->markAttended(reg->id)) {
            res.status = 500;
            res.set_content(jsonError("SERVER", "Failed to mark attendance"), "application/json");
            return;
        }
        // Issue certificate
        std::string code = generateCertCode();
        std::string path = "certs/" + code + ".txt"; // simple text cert for demo
        g_db->createCertificate(reg->id, code, path);
        // Write a simple cert file
        std::ofstream ofs(path);
        if (ofs) {
            ofs << "CERTIFICATE OF PARTICIPATION\n"
                << "This is to certify that " << reg->student_name
                << " attended \"" << reg->event_title << "\".\n"
                << "Certificate Code: " << code << "\n";
        }
        g_db->addNotification(reg->user_id, "Attendance marked for \"" + reg->event_title + "\". Certificate available.");
        std::ostringstream os;
        os << "{\"registration_id\":" << reg->id << ",\"student_name\":\"" << escapeJson(reg->student_name)
           << "\",\"certificate_code\":\"" << code << "\"}";
        res.set_content(jsonOk(os.str()), "application/json");
    });

    // Certificates
    svr.Get("/api/certificates/mine", [](const httplib::Request& req, httplib::Response& res) {
        auto user = requireAuth(req, res);
        if (!user) return;
        auto certs = g_db->getCertificatesForUser(user->id);
        std::ostringstream os;
        os << "[";
        for (size_t i = 0; i < certs.size(); ++i) {
            if (i) os << ",";
            os << "{\"id\":" << certs[i].id << ",\"code\":\"" << escapeJson(certs[i].code)
               << "\",\"event_title\":\"" << escapeJson(certs[i].event_title)
               << "\",\"issued_at\":\"" << escapeJson(certs[i].issued_at) << "\"}";
        }
        os << "]";
        res.set_content(jsonOk(os.str()), "application/json");
    });

    svr.Get(R"(/api/certificates/verify/([A-Za-z0-9\-]+))", [](const httplib::Request& req, httplib::Response& res) {
        std::string code = req.matches[1];
        auto cert = g_db->getCertificateByCode(code);
        if (!cert) {
            res.status = 404;
            res.set_content(jsonError("NOT_FOUND", "Invalid certificate code"), "application/json");
            return;
        }
        std::ostringstream os;
        os << "{\"valid\":true,\"student_name\":\"" << escapeJson(cert->student_name)
           << "\",\"event_title\":\"" << escapeJson(cert->event_title)
           << "\",\"event_date\":\"" << escapeJson(cert->event_date)
           << "\",\"code\":\"" << escapeJson(cert->code) << "\"}";
        res.set_content(jsonOk(os.str()), "application/json");
    });

    svr.Get(R"(/api/certificates/(\d+)/download)", [](const httplib::Request& req, httplib::Response& res) {
        auto user = requireAuth(req, res);
        if (!user) return;
        int id = std::stoi(req.matches[1]);
        // Find by id via code or path - simplified: list and match
        auto certs = g_db->getCertificatesForUser(user->id);
        for (const auto& c : certs) {
            if (c.id == id) {
                std::ifstream ifs(c.pdf_path);
                if (ifs) {
                    std::stringstream buf;
                    buf << ifs.rdbuf();
                    res.set_content(buf.str(), "text/plain");
                    res.set_header("Content-Disposition", "attachment; filename=\"certificate.txt\"");
                    return;
                }
            }
        }
        res.status = 404;
        res.set_content(jsonError("NOT_FOUND", "Certificate not found"), "application/json");
    });

    // Notifications
    svr.Get("/api/notifications", [](const httplib::Request& req, httplib::Response& res) {
        auto user = requireAuth(req, res);
        if (!user) return;
        auto notes = g_db->getNotifications(user->id);
        std::ostringstream os;
        os << "[";
        for (size_t i = 0; i < notes.size(); ++i) {
            if (i) os << ",";
            os << "{\"id\":" << notes[i].id << ",\"message\":\"" << escapeJson(notes[i].message)
               << "\",\"is_read\":" << notes[i].is_read
               << ",\"created_at\":\"" << escapeJson(notes[i].created_at) << "\"}";
        }
        os << "]";
        res.set_content(jsonOk(os.str()), "application/json");
    });

    svr.Patch(R"(/api/notifications/(\d+)/read)", [](const httplib::Request& req, httplib::Response& res) {
        auto user = requireAuth(req, res);
        if (!user) return;
        int id = std::stoi(req.matches[1]);
        g_db->markNotificationRead(id, user->id);
        res.set_content(jsonOk("true"), "application/json");
    });

    // CSV export
    svr.Get(R"(/api/events/(\d+)/attendance.csv)", [](const httplib::Request& req, httplib::Response& res) {
        auto user = requireAuth(req, res);
        if (!user) return;
        if (!requireRole(*user, "ORGANIZER", res)) return;
        int event_id = std::stoi(req.matches[1]);
        auto regs = g_db->getRegistrationsForEvent(event_id);
        std::ostringstream os;
        os << "id,student_name,attended,attended_at\n";
        for (const auto& r : regs) {
            os << r.id << ",\"" << r.student_name << "\"," << r.attended << ",\"" << r.attended_at << "\"\n";
        }
        res.set_content(os.str(), "text/csv");
        res.set_header("Content-Disposition", "attachment; filename=\"attendance.csv\"");
    });

    std::cout << "CampusEvents server listening on http://0.0.0.0:" << g_cfg.port << std::endl;
    std::cout << "DB: " << g_cfg.db_path << std::endl;
    if (!svr.listen("0.0.0.0", g_cfg.port)) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    return 0;
}
