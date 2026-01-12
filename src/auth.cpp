#include "../include/auth.h"
#include "../include/hash.h"

Authentication::Authentication(DatabaseConnection<std::string>& db)
    : db(db) {}

bool Authentication::registerUser(const std::string& name, const std::string& email, const std::string& password, const std::string& role) {
    
    std::string hashedPassword = sha256(password);
    
    db.executeNonQuery(
        "INSERT INTO users(name, email, password_hash, role) VALUES ('" +
        name + "','" + email + "','" + hashedPassword + "','" + role + "')"
    );
    return true;
}

std::pair<int, std::string> Authentication::loginUser(const std::string& email, const std::string& password) {
    
    std::string hashedPassword = sha256(password);
    
    auto r = db.executeQuery(
        "SELECT user_id, role FROM users WHERE email='" +
        email + "' AND password_hash='" + hashedPassword + "'"
    );
    
    if (r.empty()) return {0, ""};
    return {std::stoi(r[0][0]), r[0][1]};
}