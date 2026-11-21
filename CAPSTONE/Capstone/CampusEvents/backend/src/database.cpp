#include "database.h"
#include <iostream>
#include <cstring>

Database::Database(const std::string& path) : path_(path) {}

Database::~Database() { close(); }

bool Database::open() {
    if (sqlite3_open(path_.c_str(), &db_) != SQLITE_OK) {
        std::cerr << "Cannot open DB: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    return true;
}

void Database::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool Database::exec(const std::string& sql) {
    char* err = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << (err ? err : "") << std::endl;
        sqlite3_free(err);
        return false;
    }
    return true;
}

std::optional<User> Database::findUserByEmail(const std::string& email) {
    std::lock_guard<std::mutex> lock(mtx_);
    const char* sql = "SELECT id,name,email,password_hash,role,department FROM users WHERE email=?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;
    sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_TRANSIENT);
    std::optional<User> u;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        User user;
        user.id = sqlite3_column_int(stmt, 0);
        user.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        user.role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        if (sqlite3_column_text(stmt, 5))
            user.department = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        u = user;
    }
    sqlite3_finalize(stmt);
    return u;
}

std::optional<User> Database::findUserById(int id) {
    std::lock_guard<std::mutex> lock(mtx_);
    const char* sql = "SELECT id,name,email,password_hash,role,department FROM users WHERE id=?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;
    sqlite3_bind_int(stmt, 1, id);
    std::optional<User> u;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        User user;
        user.id = sqlite3_column_int(stmt, 0);
        user.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        user.role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        if (sqlite3_column_text(stmt, 5))
            user.department = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        u = user;
    }
    sqlite3_finalize(stmt);
    return u;
}

int Database::createUser(const std::string& name, const std::string& email,
                         const std::string& password_hash, const std::string& role,
                         const std::string& department) {
    std::lock_guard<std::mutex> lock(mtx_);
    const char* sql = "INSERT INTO users(name,email,password_hash,role,department) VALUES(?,?,?,?,?)";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, password_hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, role.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, department.c_str(), -1, SQLITE_TRANSIENT);
    int id = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) id = (int)sqlite3_last_insert_rowid(db_);
    sqlite3_finalize(stmt);
    return id;
}

int Database::createEvent(const Event& e) {
    std::lock_guard<std::mutex> lock(mtx_);
    const char* sql = R"(INSERT INTO events(title,type,theme,description,venue,start_at,end_at,seat_limit,status,created_by)
                         VALUES(?,?,?,?,?,?,?,?,?,?))";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, e.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, e.type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, e.theme.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, e.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, e.venue.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, e.start_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, e.end_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 8, e.seat_limit);
    sqlite3_bind_text(stmt, 9, e.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 10, e.created_by);
    int id = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) id = (int)sqlite3_last_insert_rowid(db_);
    sqlite3_finalize(stmt);
    return id;
}

std::optional<Event> Database::getEvent(int id) {
    std::lock_guard<std::mutex> lock(mtx_);
    const char* sql = R"(SELECT e.id,e.title,e.type,e.theme,e.description,e.venue,e.start_at,e.end_at,
                         e.seat_limit,e.status,e.reviewer_id,e.review_reason,e.created_by,e.created_at,e.updated_at,
                         (SELECT COUNT(*) FROM registrations r WHERE r.event_id=e.id) as reg_count
                         FROM events e WHERE e.id=?)";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;
    sqlite3_bind_int(stmt, 1, id);
    std::optional<Event> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Event e;
        e.id = sqlite3_column_int(stmt, 0);
        e.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        e.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        if (sqlite3_column_text(stmt, 3)) e.theme = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        if (sqlite3_column_text(stmt, 4)) e.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        e.venue = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        e.start_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        e.end_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        e.seat_limit = sqlite3_column_int(stmt, 8);
        e.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        e.reviewer_id = sqlite3_column_int(stmt, 10);
        if (sqlite3_column_text(stmt, 11)) e.review_reason = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        e.created_by = sqlite3_column_int(stmt, 12);
        e.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));
        e.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14));
        e.registered_count = sqlite3_column_int(stmt, 15);
        result = e;
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<Event> Database::listEvents(const std::string& status_filter,
                                        const std::string& type_filter,
                                        int created_by) {
    std::lock_guard<std::mutex> lock(mtx_);
    std::string sql = R"(SELECT e.id,e.title,e.type,e.theme,e.description,e.venue,e.start_at,e.end_at,
                         e.seat_limit,e.status,e.reviewer_id,e.review_reason,e.created_by,e.created_at,e.updated_at,
                         (SELECT COUNT(*) FROM registrations r WHERE r.event_id=e.id) as reg_count
                         FROM events e WHERE 1=1)";
    if (!status_filter.empty()) sql += " AND e.status='" + status_filter + "'";
    if (!type_filter.empty()) sql += " AND e.type='" + type_filter + "'";
    if (created_by > 0) sql += " AND e.created_by=" + std::to_string(created_by);
    sql += " ORDER BY e.start_at ASC";

    sqlite3_stmt* stmt = nullptr;
    std::vector<Event> list;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return list;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Event e;
        e.id = sqlite3_column_int(stmt, 0);
        e.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        e.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        if (sqlite3_column_text(stmt, 3)) e.theme = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        if (sqlite3_column_text(stmt, 4)) e.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        e.venue = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        e.start_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        e.end_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        e.seat_limit = sqlite3_column_int(stmt, 8);
        e.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        e.reviewer_id = sqlite3_column_int(stmt, 10);
        if (sqlite3_column_text(stmt, 11)) e.review_reason = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        e.created_by = sqlite3_column_int(stmt, 12);
        e.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));
        e.updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14));
        e.registered_count = sqlite3_column_int(stmt, 15);
        list.push_back(e);
    }
    sqlite3_finalize(stmt);
    return list;
}

bool Database::updateEventStatus(int id, const std::string& status, int reviewer_id,
                                 const std::string& reason) {
    std::lock_guard<std::mutex> lock(mtx_);
    const char* sql = "UPDATE events SET status=?, reviewer_id=?, review_reason=?, updated_at=datetime('now') WHERE id=?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, reviewer_id);
    sqlite3_bind_text(stmt, 3, reason.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, id);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::hasVenueConflict(const std::string& venue, const std::string& start_at,
                                const std::string& end_at, int exclude_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    const char* sql = R"(SELECT COUNT(*) FROM events
                         WHERE venue=? AND status IN ('PENDING','APPROVED')
                         AND id != ? AND start_at < ? AND end_at > ?)";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, venue.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, exclude_id);
    sqlite3_bind_text(stmt, 3, end_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, start_at.c_str(), -1, SQLITE_TRANSIENT);
    bool conflict = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) conflict = sqlite3_column_int(stmt, 0) > 0;
    sqlite3_finalize(stmt);
    return conflict;
}

int Database::registerUser(int event_id, int user_id, const std::string& token) {
    std::lock_guard<std::mutex> lock(mtx_);
    // Transaction for seat limit
    sqlite3_exec(db_, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr);

    // Check current count
    const char* countSql = "SELECT COUNT(*) FROM registrations WHERE event_id=?";
    sqlite3_stmt* cstmt = nullptr;
    sqlite3_prepare_v2(db_, countSql, -1, &cstmt, nullptr);
    sqlite3_bind_int(cstmt, 1, event_id);
    int count = 0;
    if (sqlite3_step(cstmt) == SQLITE_ROW) count = sqlite3_column_int(cstmt, 0);
    sqlite3_finalize(cstmt);

    // Get seat limit
    const char* limitSql = "SELECT seat_limit FROM events WHERE id=?";
    sqlite3_stmt* lstmt = nullptr;
    sqlite3_prepare_v2(db_, limitSql, -1, &lstmt, nullptr);
    sqlite3_bind_int(lstmt, 1, event_id);
    int limit = 0;
    if (sqlite3_step(lstmt) == SQLITE_ROW) limit = sqlite3_column_int(lstmt, 0);
    sqlite3_finalize(lstmt);

    if (count >= limit) {
        sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        return -2; // full
    }

    const char* sql = "INSERT INTO registrations(event_id,user_id,token) VALUES(?,?,?)";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        return -1;
    }
    sqlite3_bind_int(stmt, 1, event_id);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_bind_text(stmt, 3, token.c_str(), -1, SQLITE_TRANSIENT);
    int id = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        id = (int)sqlite3_last_insert_rowid(db_);
        sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr);
    } else {
        sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        id = -3; // duplicate or error
    }
    sqlite3_finalize(stmt);
    return id;
}

std::optional<Registration> Database::getRegistration(int id) {
    std::lock_guard<std::mutex> lock(mtx_);
    const char* sql = R"(SELECT r.id,r.event_id,r.user_id,r.token,r.attended,r.attended_at,r.created_at,
                         u.name, e.title FROM registrations r
                         JOIN users u ON u.id=r.user_id JOIN events e ON e.id=r.event_id WHERE r.id=?)";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;
    sqlite3_bind_int(stmt, 1, id);
    std::optional<Registration> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Registration r;
        r.id = sqlite3_column_int(stmt, 0);
        r.event_id = sqlite3_column_int(stmt, 1);
        r.user_id = sqlite3_column_int(stmt, 2);
        r.token = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        r.attended = sqlite3_column_int(stmt, 4);
        if (sqlite3_column_text(stmt, 5)) r.attended_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        r.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        r.student_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        r.event_title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        result = r;
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<Registration> Database::getRegistrationByToken(const std::string& token) {
    std::lock_guard<std::mutex> lock(mtx_);
    const char* sql = R"(SELECT r.id,r.event_id,r.user_id,r.token,r.attended,r.attended_at,r.created_at,
                         u.name, e.title FROM registrations r
                         JOIN users u ON u.id=r.user_id JOIN events e ON e.id=r.event_id WHERE r.token=?)";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;
    sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_TRANSIENT);
    std::optional<Registration> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Registration r;
        r.id = sqlite3_column_int(stmt, 0);
        r.event_id = sqlite3_column_int(stmt, 1);
        r.user_id = sqlite3_column_int(stmt, 2);
        r.token = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        r.attended = sqlite3_column_int(stmt, 4);
        if (sqlite3_column_text(stmt, 5)) r.attended_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        r.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        r.student_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        r.event_title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        result = r;
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<Registration> Database::getRegistrationsForEvent(int event_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    const char* sql = R"(SELECT r.id,r.event_id,r.user_id,r.token,r.attended,r.attended_at,r.created_at,
                         u.name, e.title FROM registrations r
                         JOIN users u ON u.id=r.user_id JOIN events e ON e.id=r.event_id
                         WHERE r.event_id=? ORDER BY r.created_at)";
    sqlite3_stmt* stmt = nullptr;
    std::vector<Registration> list;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;
    sqlite3_bind_int(stmt, 1, event_id);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Registration r;
        r.id = sqlite3_column_int(stmt, 0);
        r.event_id = sqlite3_column_int(stmt, 1);
        r.user_id = sqlite3_column_int(stmt, 2);
        r.token = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        r.attended = sqlite3_column_int(stmt, 4);
        if (sqlite3_column_text(stmt, 5)) r.attended_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        r.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        r.student_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        r.event_title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        list.push_back(r);
    }
    sqlite3_finalize(stmt);
    return list;
}

std::vector<Registration> Database::getRegistrationsForUser(int user_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    const char* sql = R"(SELECT r.id,r.event_id,r.user_id,r.token,r.attended,r.attended_at,r.created_at,
                         u.name, e.title FROM registrations r
                         JOIN users u ON u.id=r.user_id JOIN events e ON e.id=r.event_id
                         WHERE r.user_id=? ORDER BY r.created_at DESC)";
    sqlite3_stmt* stmt = nullptr;
    std::vector<Registration> list;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;
    sqlite3_bind_int(stmt, 1, user_id);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Registration r;
        r.id = sqlite3_column_int(stmt, 0);
        r.event_id = sqlite3_column_int(stmt, 1);
        r.user_id = sqlite3_column_int(stmt, 2);
        r.token = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        r.attended = sqlite3_column_int(stmt, 4);
        if (sqlite3_column_text(stmt, 5)) r.attended_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        r.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        r.student_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        r.event_title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        list.push_back(r);
    }
    sqlite3_finalize(stmt);
    return list;
}

int Database::countRegistrations(int event_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    const char* sql = "SELECT COUNT(*) FROM registrations WHERE event_id=?";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, event_id);
    int c = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) c = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return c;
}

bool Database::markAttended(int reg_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    const char* sql = "UPDATE registrations SET attended=1, attended_at=datetime('now') WHERE id=? AND attended=0";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, reg_id);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_) > 0;
    sqlite3_finalize(stmt);
    return ok;
}

int Database::createCertificate(int reg_id, const std::string& code, const std::string& path) {
    std::lock_guard<std::mutex> lock(mtx_);
    const char* sql = "INSERT INTO certificates(registration_id,code,pdf_path) VALUES(?,?,?)";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;
    sqlite3_bind_int(stmt, 1, reg_id);
    sqlite3_bind_text(stmt, 2, code.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, path.c_str(), -1, SQLITE_TRANSIENT);
    int id = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) id = (int)sqlite3_last_insert_rowid(db_);
    sqlite3_finalize(stmt);
    return id;
}

std::optional<Certificate> Database::getCertificateByCode(const std::string& code) {
    std::lock_guard<std::mutex> lock(mtx_);
    const char* sql = R"(SELECT c.id,c.registration_id,c.code,c.pdf_path,c.issued_at,
                         u.name, e.title, e.start_at, u.department
                         FROM certificates c
                         JOIN registrations r ON r.id=c.registration_id
                         JOIN users u ON u.id=r.user_id
                         JOIN events e ON e.id=r.event_id WHERE c.code=?)";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;
    sqlite3_bind_text(stmt, 1, code.c_str(), -1, SQLITE_TRANSIENT);
    std::optional<Certificate> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Certificate c;
        c.id = sqlite3_column_int(stmt, 0);
        c.registration_id = sqlite3_column_int(stmt, 1);
        c.code = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        if (sqlite3_column_text(stmt, 3)) c.pdf_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        c.issued_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        c.student_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        c.event_title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        c.event_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        if (sqlite3_column_text(stmt, 8)) c.department = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        result = c;
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<Certificate> Database::getCertificateByRegId(int reg_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    const char* sql = R"(SELECT c.id,c.registration_id,c.code,c.pdf_path,c.issued_at,
                         u.name, e.title, e.start_at, u.department
                         FROM certificates c
                         JOIN registrations r ON r.id=c.registration_id
                         JOIN users u ON u.id=r.user_id
                         JOIN events e ON e.id=r.event_id WHERE c.registration_id=?)";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;
    sqlite3_bind_int(stmt, 1, reg_id);
    std::optional<Certificate> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Certificate c;
        c.id = sqlite3_column_int(stmt, 0);
        c.registration_id = sqlite3_column_int(stmt, 1);
        c.code = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        if (sqlite3_column_text(stmt, 3)) c.pdf_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        c.issued_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        c.student_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        c.event_title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        c.event_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        if (sqlite3_column_text(stmt, 8)) c.department = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        result = c;
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<Certificate> Database::getCertificatesForUser(int user_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    const char* sql = R"(SELECT c.id,c.registration_id,c.code,c.pdf_path,c.issued_at,
                         u.name, e.title, e.start_at, u.department
                         FROM certificates c
                         JOIN registrations r ON r.id=c.registration_id
                         JOIN users u ON u.id=r.user_id
                         JOIN events e ON e.id=r.event_id WHERE r.user_id=? ORDER BY c.issued_at DESC)";
    sqlite3_stmt* stmt = nullptr;
    std::vector<Certificate> list;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;
    sqlite3_bind_int(stmt, 1, user_id);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Certificate c;
        c.id = sqlite3_column_int(stmt, 0);
        c.registration_id = sqlite3_column_int(stmt, 1);
        c.code = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        if (sqlite3_column_text(stmt, 3)) c.pdf_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        c.issued_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        c.student_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        c.event_title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        c.event_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        if (sqlite3_column_text(stmt, 8)) c.department = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        list.push_back(c);
    }
    sqlite3_finalize(stmt);
    return list;
}

int Database::addNotification(int user_id, const std::string& message) {
    std::lock_guard<std::mutex> lock(mtx_);
    const char* sql = "INSERT INTO notifications(user_id,message) VALUES(?,?)";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, message.c_str(), -1, SQLITE_TRANSIENT);
    int id = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) id = (int)sqlite3_last_insert_rowid(db_);
    sqlite3_finalize(stmt);
    return id;
}

std::vector<Notification> Database::getNotifications(int user_id, bool unread_only) {
    std::lock_guard<std::mutex> lock(mtx_);
    std::string sql = "SELECT id,user_id,message,is_read,created_at FROM notifications WHERE user_id=?";
    if (unread_only) sql += " AND is_read=0";
    sql += " ORDER BY created_at DESC LIMIT 50";
    sqlite3_stmt* stmt = nullptr;
    std::vector<Notification> list;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return list;
    sqlite3_bind_int(stmt, 1, user_id);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Notification n;
        n.id = sqlite3_column_int(stmt, 0);
        n.user_id = sqlite3_column_int(stmt, 1);
        n.message = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        n.is_read = sqlite3_column_int(stmt, 3);
        n.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        list.push_back(n);
    }
    sqlite3_finalize(stmt);
    return list;
}

bool Database::markNotificationRead(int id, int user_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    const char* sql = "UPDATE notifications SET is_read=1 WHERE id=? AND user_id=?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_int(stmt, 2, user_id);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}
