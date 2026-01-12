#ifndef AUTH_H
#define AUTH_H

#include "db_connection.h"
#include <string>
#include <utility>

class Authentication {
    DatabaseConnection<std::string>& db;

public:
    Authentication(DatabaseConnection<std::string>& db);

    bool registerUser(const std::string& name, const std::string& email, const std::string& password, const std::string& role);
    std::pair<int, std::string> loginUser(const std::string& email, const std::string& password);
    
    bool isEmailExists(const std::string& email); 
};

#endif 