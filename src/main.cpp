#include "../include/db_connection.h"
#include "../include/menu.h"

int main() {
    DatabaseConnection<std::string> db(
        "host=localhost port=5432 dbname=online-store user=postgres"
    );

    Menu menu(db);
    menu.showMainMenu();
    return 0;
}
