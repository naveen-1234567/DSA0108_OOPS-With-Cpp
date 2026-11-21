#pragma once
#include <sqlite3.h>
#include <string>
#include <vector>
#include <optional>
#include <mutex>

struct User {
    int id;
    std::string name;
    std::string email;
    std::string password_hash;
    std::string role;
    std::string department;
};

struct Event {
    int id;
    std::string title;
    std::string type;
    std::string theme;
    std::string description;
    std::string venue;
    std::string start_at;
    std::string end_at;
    int seat_limit;
    std::string status;
    int reviewer_id;
    std::string review_reason;
    int created_by;
    std::string created_at;
    std::string updated_at;
    int registered_count; // computed
};

struct Registration {
    int id;
    int event_id;
    int user_id;
    std::string token;
    int attended;
    std::string attended_at;
    std::string created_at;
    std::string student_name; // joined
    std::string event_title;
};

struct Certificate {
    int id;
    int registration_id;
    std::string code;
    std::string pdf_path;
    std::string issued_at;
    std::string student_name;
    std::string event_title;
    std::string event_date;
    std::string department;
};

struct Notification {
    int id;
    int user_id;
    std::string message;
    int is_read;
    std::string created_at;
};

class Database {
public:
    explicit Database(const std::string& path);
    ~Database();

    bool open();
    void close();
    bool exec(const std::string& sql);
    sqlite3* handle() { return db_; }

    // Users
    std::optional<User> findUserByEmail(const std::string& email);
    std::optional<User> findUserById(int id);
    int createUser(const std::string& name, const std::string& email,
                   const std::string& password_hash, const std::string& role,
                   const std::string& department);

    // Events
    int createEvent(const Event& e);
    std::optional<Event> getEvent(int id);
    std::vector<Event> listEvents(const std::string& status_filter = "",
                                  const std::string& type_filter = "",
                                  int created_by = -1);
    bool updateEventStatus(int id, const std::string& status, int reviewer_id,
                           const std::string& reason);
    bool hasVenueConflict(const std::string& venue, const std::string& start_at,
                          const std::string& end_at, int exclude_id = -1);

    // Registrations
    int registerUser(int event_id, int user_id, const std::string& token);
    std::optional<Registration> getRegistration(int id);
    std::optional<Registration> getRegistrationByToken(const std::string& token);
    std::vector<Registration> getRegistrationsForEvent(int event_id);
    std::vector<Registration> getRegistrationsForUser(int user_id);
    int countRegistrations(int event_id);
    bool markAttended(int reg_id);

    // Certificates
    int createCertificate(int reg_id, const std::string& code, const std::string& path);
    std::optional<Certificate> getCertificateByCode(const std::string& code);
    std::optional<Certificate> getCertificateByRegId(int reg_id);
    std::vector<Certificate> getCertificatesForUser(int user_id);

    // Notifications
    int addNotification(int user_id, const std::string& message);
    std::vector<Notification> getNotifications(int user_id, bool unread_only = false);
    bool markNotificationRead(int id, int user_id);

private:
    sqlite3* db_ = nullptr;
    std::string path_;
    std::mutex mtx_;
};
