#include "../include/menu.h"
#include <iostream>


Menu::Menu(DatabaseConnection<std::string>& db)
    : db(db), auth(db) {}


void Menu::pause() {
    std::cout << "\nНажмите Enter";
    std::cin.ignore();
    std::cin.get();
}

void Menu::registrationMenu() {
    std::string name, email, password, role;
    
    std::cout << "РЕГИСТРАЦИЯ\n";
    
    std::cout << "Имя: ";
    std::cin >> name;
    
    std::cout << "Email: ";
    std::cin >> email;
    
    std::cout << "Пароль: ";
    std::cin >> password;
    
    std::cout << "Роль (admin/manager/customer): ";
    std::cin >> role;
    
    if (role != "admin" && role != "manager" && role != "customer") {
        std::cout << "Неверная роль. Допустимые: admin, manager, customer\n";
        pause();
        return;
    }
    
    if (auth.registerUser(name, email, password, role)) {
        std::cout << "Пользователь зарегистрирован\n";
    }
    
    pause();
}

void Menu::loginAsRole(const std::string& role) {
    std::string email, password;

    std::cout << "Email: ";
    std::cin >> email;
    std::cout << "Пароль: ";
    std::cin >> password;

    auto res = auth.loginUser(email, password);

    if (res.first == 0 || res.second != role) {
        std::cout << "Ошибка авторизации или неверная роль\n";
        currentUser.reset();
        return;
    }

    auto nameRes = db.executeQuery(
        "SELECT name FROM users WHERE user_id=" + std::to_string(res.first)
    );
    db.executeNonQuery("SET SESSION app.user_id = " + std::to_string(res.first));

    std::string name;

    if (nameRes.empty()) {
        name = "User";
    } else {
        name = nameRes[0][0];
    }

    if (role == "admin")
        currentUser = std::make_shared<Admin>(res.first, name, db);
    else if (role == "manager")
        currentUser = std::make_shared<Manager>(res.first, name, db);
    else
        currentUser = std::make_shared<Customer>(res.first, name, db);
    
    if (currentUser) {
        currentUser->displayInfo();
        currentUser->loadOrdersFromDB();
    }
}

void Menu::showMainMenu() {
    int choice;

    while (true) {
        std::cout <<
            "\n=== ГЛАВНОЕ МЕНЮ ===\n"
            "1. Войти как администратор\n"
            "2. Войти как менеджер\n"
            "3. Войти как покупатель\n"
            "4. Регистрация\n"
            "5. Выход\n> ";

        std::cin >> choice;

        if (choice == 1) {
            loginAsRole("admin");
            if (currentUser) adminMenu();
        }
        else if (choice == 2) {
            loginAsRole("manager");
            if (currentUser) managerMenu();
        }
        else if (choice == 3) {
            loginAsRole("customer");
            if (currentUser) customerMenu();
        }
        else if (choice == 4) {
            registrationMenu();  
        }
        else if (choice == 5) {
            return;
        }
    }
}

void Menu::adminMenu() {
    auto admin = std::dynamic_pointer_cast<Admin>(currentUser);
    int choise;

    while (true) {
        std::cout <<
            "\n=== МЕНЮ АДМИНИСТРАТОРА ===\n"
            "1. Добавить продукт\n"
            "2. Обновить продукт\n"
            "3. Удалить продукт\n"
            "4. Просмотр всех заказов\n"
            "5. Детали заказа\n"
            "6. Изменить статус заказа\n"
            "7. История статусов заказа\n"
            "8. Отменить заказ\n"
            "9. Журнал аудита\n"
            "10. CSV-отчёт\n"
            "11. Выход\n> ";

        std::cin >> choise;

        if (choise == 1) {
            std::string name; double price; int quantity;
            std::cout << "Название, цена, количество: \n";
            std::cin >> name >> price >> quantity;
            admin->addProduct(name, price, quantity);
        }
        else if (choise == 2) {
            int id, quantity; double price;
            admin -> viewAllProducts();
            std::cout << "ID, новая_цена, новое_количество: \n";
            std::cin >> id >> price >> quantity;
            admin->updateProduct(id, price, quantity);
        }
        else if (choise == 3) {
            int id;
            admin -> viewAllProducts();
            std::cout << "ID продукта: ";
            std::cin >> id;
            admin->deleteProduct(id);
        }
        else if (choise == 4) admin->viewAllOrders();
        else if (choise == 5) {
            int id; 
            std::cout << "ID заказа: ";
            std::cin >> id;
            admin->viewOrderDetails(id);
        }
        else if (choise == 6) {
            int id; std::string s;
            std::cout << "ID_заказа, новый_статус: \n";
            std::cin >> id >> s;
            admin->updateOrderStatus(id, s);
        }
        else if (choise == 7) {
            int id; 
            std::cout << "ID заказа: ";
            std::cin >> id;
            admin->viewOrderStatusHistory(id);
        }
        else if (choise == 8) { 
            int id; 
            std::cout << "ID заказа для отмены: ";
            std::cin >> id;
            admin->cancelOrder(id);
        }
        else if (choise == 9){
            int id;
            std::cout<<"Введите id пользователя"<<std::endl;
            std::cin>>id;
            admin->viewAudit(id);
        }
        else if (choise == 10) admin->generateCSVReport();
        else if (choise == 11) {
            currentUser.reset();
            return;
        }

        pause();
    }
}


void Menu::managerMenu() {
    auto manager = std::dynamic_pointer_cast<Manager>(currentUser);
    int choise;

    while (true) {
        std::cout <<
            "\n=== МЕНЮ МЕНЕДЖЕРА ===\n"
            "1. Заказы на утверждение (pending)\n"
            "2. Утвердить заказ\n"
            "3. Обновить склад\n"
            "4. Детали заказа\n"
            "5. Изменить статус заказа\n"
            "6. Отменить заказ\n" 
            "7. История утверждённых заказов\n"
            "8. Выход\n> ";

        std::cin >> choise;

        if (choise == 1) manager->viewPendingOrders();
        else if (choise == 2) {
            int id; 
            std::cout << "ID заказа: ";
            std::cin >> id;
            manager->approveOrder(id);
        }
        else if (choise == 3) {
            int id, quantity; 
            std::cout << "ID_продукта, новое_количество: \n";
            std::cin >> id >> quantity;
            manager->updateStock(id, quantity);
        }
        else if (choise == 4) {
            int id; 
            std::cout << "ID заказа: ";
            std::cin >> id;
            manager->viewOrderDetails(id);
        }
        else if (choise == 5) {
            int id; std::string s;
            std::cout << "ID_заказа, новый_статус: \n";
            std::cin >> id >> s;
            manager->updateOrderStatus(id, s);
        }
        else if (choise == 6) {
            int id; 
            std::cout << "ID заказа для отмены: ";
            std::cin >> id;
            manager->cancelOrder(id);
        }
        else if (choise == 7) manager->viewApprovedOrdersHistory();
        else if (choise == 8) {
            currentUser.reset();
            return;
        }

        pause();
    }
}


void Menu::customerMenu() {
    auto customer = std::dynamic_pointer_cast<Customer>(currentUser);
    int choise;

    while (true) {
        std::cout <<
            "\n=== МЕНЮ ПОКУПАТЕЛЯ ===\n"
            "1. Создать заказ \n"
            "2. Добавить товар в заказ\n"
            "3. Удалить товар из заказа\n"
            "4. Мои заказы\n"
            "5. Статус заказа\n"
            "6. Оплатить заказ \n"
            "7. Отменить заказ \n"
            "8. Возврат заказа \n"
            "9. История статусов\n"
            "10. Выход\n> ";

        std::cin >> choise;

        if (choise == 1) customer->createOrder();
        else if (choise == 2) {
            int order_id, product_id, quantity;
            std::cout << "ID_заказа, ID_продукта, количество: \n";
            std::cin >> order_id >> product_id >> quantity;
            customer->addToOrder(order_id, product_id, quantity);
        }
        else if (choise == 3) {
            customer -> viewAllProducts();
            int order_id, product_id;
            std::cout << "ID_заказа, ID_продукта: \n";
            std::cin >> order_id >> product_id;
            customer->removeFromOrder(order_id, product_id);
        }
        else if (choise == 4) customer->viewMyOrders();
        else if (choise == 5) {
            int id; 
            std::cout << "ID заказа: ";
            std::cin >> id;
            customer->viewOrderStatus(id);
        }
        else if (choise == 6) {
            int id; 
            std::cout << "ID заказа: ";
            std::cin >> id;
            customer->makePayment(id);
        }
        else if (choise == 7) {
            int id; 
            std::cout << "ID заказа для отмены: ";
            std::cin >> id;
            customer->cancelOrder(id);
        }
        else if (choise == 8) {
            int id; 
            std::cout << "ID заказа: ";
            std::cin >> id;
            customer->returnOrder(id);
        }
        else if (choise == 9) {
            int id; 
            std::cout << "ID заказа: ";
            std::cin >> id;
            customer->viewOrderStatusHistory(id);
        }
        else if (choise == 10) {
            currentUser.reset();
            return;
        }

        pause();
    }
}