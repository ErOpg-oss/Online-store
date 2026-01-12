#ifndef MENU_H
#define MENU_H
#include <memory>
#include "db_connection.h"
#include "auth.h"
#include "user.h"

class Menu {
public:
    DatabaseConnection<std::string>& db;
    Authentication auth;
    std::shared_ptr<User> currentUser;

    void pause();

    void loginAsRole(const std::string& role);
    void registrationMenu();

    void adminMenu();
    void managerMenu();
    void customerMenu();

    Menu(DatabaseConnection<std::string>& db);
    void showMainMenu();
};

#endif
