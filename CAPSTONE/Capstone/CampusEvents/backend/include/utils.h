#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp> // we will use a simple json or implement minimal

// Minimal JSON helpers to avoid external dep if possible; for real use nlohmann
// For this project we embed a tiny JSON writer/reader or assume nlohmann available.
// To keep build simple we implement basic response helpers without full JSON lib.

std::string sha256(const std::string& input);
std::string generateToken();
std::string generateCertCode();
std::string hmacSign(const std::string& data, const std::string& key);
bool verifyHmac(const std::string& data, const std::string& signature, const std::string& key);

// Simple JSON string builders
std::string jsonOk(const std::string& dataJson = "null");
std::string jsonError(const std::string& code, const std::string& message);
std::string escapeJson(const std::string& s);

// Config
struct Config {
    std::string jwt_secret = "campus-events-secret-change-me";
    std::string qr_hmac_key = "qr-hmac-key-change-me";
    int port = 8080;
    std::string db_path = "campus.db";
    std::string cors_origin = "*";
};

Config loadConfig();
