#include "utils.h"
#include <sstream>
#include <iomanip>
#include <random>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <iostream>

#ifdef USE_OPENSSL
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#else
// Fallback simple hash (not cryptographically strong, demo only)
#include <functional>
#endif

std::string sha256(const std::string& input) {
#ifdef USE_OPENSSL
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return ss.str();
#else
    // Simple demo hash (replace with real in production)
    std::hash<std::string> h;
    size_t val = h(input);
    std::stringstream ss;
    ss << std::hex << val << std::hex << (val * 31);
    return ss.str();
#endif
}

std::string generateToken() {
    static std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<uint64_t> dist;
    std::stringstream ss;
    ss << std::hex << dist(rng) << dist(rng);
    return ss.str();
}

std::string generateCertCode() {
    static const char alphanum[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, sizeof(alphanum) - 2);
    std::string code = "CERT-";
    for (int i = 0; i < 10; ++i) code += alphanum[dist(rng)];
    return code;
}

std::string hmacSign(const std::string& data, const std::string& key) {
#ifdef USE_OPENSSL
    unsigned char* result;
    unsigned int len = 32;
    result = HMAC(EVP_sha256(), key.data(), key.size(),
                  reinterpret_cast<const unsigned char*>(data.data()), data.size(),
                  nullptr, &len);
    std::stringstream ss;
    for (unsigned int i = 0; i < len; ++i)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)result[i];
    return ss.str();
#else
    return sha256(key + data + key);
#endif
}

bool verifyHmac(const std::string& data, const std::string& signature, const std::string& key) {
    return hmacSign(data, key) == signature;
}

std::string escapeJson(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else out += c;
        }
    }
    return out;
}

std::string jsonOk(const std::string& dataJson) {
    return std::string("{\"ok\":true,\"data\":") + dataJson + "}";
}

std::string jsonError(const std::string& code, const std::string& message) {
    return std::string("{\"ok\":false,\"error\":{\"code\":\"") + escapeJson(code) +
           "\",\"message\":\"" + escapeJson(message) + "\"}}";
}

Config loadConfig() {
    Config cfg;
    if (const char* p = std::getenv("PORT")) cfg.port = std::atoi(p);
    if (const char* p = std::getenv("DB_PATH")) cfg.db_path = p;
    if (const char* p = std::getenv("JWT_SECRET")) cfg.jwt_secret = p;
    if (const char* p = std::getenv("QR_HMAC_KEY")) cfg.qr_hmac_key = p;
    if (const char* p = std::getenv("CORS_ORIGIN")) cfg.cors_origin = p;
    return cfg;
}
