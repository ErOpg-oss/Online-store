#include "../include/user.h"
#include <iostream>
#include <numeric>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>



User::User(int id, std::string name, DatabaseConnection<std::string>& db)
    : id(id), name(std::move(name)), db(db) {}

void User::loadOrdersFromDB() {
    orders.clear();
    auto result = db.executeQuery(
        "SELECT order_id, status, total_price FROM orders WHERE user_id=" + std::to_string(id)
    );
    
    for (const auto& row : result) {
        auto order = std::make_shared<Order>(std::stoi(row[0]));
        order->setTotalAmount(std::stod(row[2]));
        auto itemsResult = db.executeQuery(
            "SELECT product_id, quantity, price FROM order_items WHERE order_id=" + row[0]
        );
        
        orders.push_back(order);
    }
}
void User::viewAllProducts(){
    auto result = db.executeQuery("SELECT * FROM products");
    std::cout << "Все продукты (" << result.size() << "):\n";
    for (const auto& row : result) {
        for (const auto& field : row) {
            std::cout << field << "\t";
        }
        std::cout << std::endl;
    }
}


std::vector<std::shared_ptr<Order>> User::getOrdersByStatus(const std::string& status) const {
    std::vector<std::shared_ptr<Order>> filteredOrders;
    
    std::copy_if(orders.begin(), orders.end(), std::back_inserter(filteredOrders),
        [&](const std::shared_ptr<Order>& order) {
            auto result = db.executeQuery(
                "SELECT status FROM orders WHERE order_id=" + std::to_string(order->getId())
            );
            return !result.empty() && result[0][0] == status;
        }
    );
    
    return filteredOrders;
}

double User::calculateTotalSpent() const {
    return std::accumulate(orders.begin(), orders.end(), 0.0,
        [](double sum, const std::shared_ptr<Order>& order) {
            return sum + order->total();
        }
    );
}

void User::removeOrder(int orderId) {
    orders.erase(
        std::remove_if(orders.begin(), orders.end(),
            [orderId](const std::shared_ptr<Order>& order) {
                return order->getId() == orderId;
            }),
        orders.end()
    );
}


void Admin::displayInfo() const {
    std::cout << "Администратор: " << name << " (ID: " << id << ")\n";
}

bool Admin::hasPermission(const std::string& action) const {
    auto checkPermission = [](const std::string& role, const std::string& action) -> bool {
        if (role != "admin") return false;
        
        static const std::vector<std::string> adminActions = {
            "add_product", "update_product", "delete_product",
            "view_all_orders", "update_order_status", "view_audit",
            "generate_report", "cancel_any_order"
        };
        
        return std::find(adminActions.begin(), adminActions.end(), action) != adminActions.end();
    };
    
    return checkPermission("admin", action);
}

void Admin::cancelOrder(int orderId) {
    if (!hasPermission("cancel_any_order")) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    
    db.executeNonQuery(
            "CALL update_order_status_simple(" + std::to_string(orderId) + ",'canceled'," + std::to_string(this->id) + ")"
    );
    std::cout << "Заказ #" << orderId << " отменен администратором\n";
}

void Admin::viewOrderStatus(int orderId) {
    auto result = db.executeQuery(
        "SELECT status FROM orders WHERE order_id=" + std::to_string(orderId)
    );
    if (!result.empty()) {
        std::cout << "Статус заказа #" << orderId << ": " << result[0][0] << std::endl;
    } else {
        std::cout << "Заказ не найден\n";
    }
}

void Admin::addProduct(const std::string& n, double p, int q) {
    if (!hasPermission("add_product")) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    db.executeNonQuery(
        "INSERT INTO products(name,price,stock_quantity) VALUES ('" +
        n + "'," + std::to_string(p) + "," + std::to_string(q) + ")"
    );
    std::cout << "Продукт добавлен: " << n << std::endl;
}

void Admin::updateProduct(int id, double p, int q) {
    if (!hasPermission("update_product")) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    
    db.executeNonQuery(
        "UPDATE products SET price=" + std::to_string(p) +
        ", stock_quantity=" + std::to_string(q) +
        " WHERE product_id=" + std::to_string(id)
    );
    std::cout << "Продукт обновлен: ID=" << id << std::endl;
}

void Admin::deleteProduct(int id) {
    if (!hasPermission("delete_product")) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    
    db.executeNonQuery(
        "DELETE FROM products WHERE product_id=" + std::to_string(id)
    );
    std::cout << "Продукт удален: ID=" << id << std::endl;
}

void Admin::viewAllOrders() {
    if (!hasPermission("view_all_orders")) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    
    auto result = db.executeQuery("SELECT * FROM orders");
    std::cout << "Все заказы (" << result.size() << "):\n";
    for (const auto& row : result) {
        for (const auto& field : row) {
            std::cout << field << "\t";
        }
        std::cout << std::endl;
    }
}

void Admin::viewOrderDetails(int id) {
    auto result = db.executeQuery(
        "SELECT * FROM order_items WHERE order_id=" + std::to_string(id)
    );
    std::cout << "Детали заказа #" << id << ":\n";
    for (const auto& row : result) {
        for (const auto& field : row) {
            std::cout << field << "\t";
        }
        std::cout << std::endl;
    }
}
void Admin::createOrder(){
        std::cout << "Доступ запрещен\n";
        return;
}

void Admin::updateOrderStatus(int id, const std::string& s) {
    if (!hasPermission("update_order_status")) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    
    db.executeNonQuery(
        "CALL update_order_status_simple(" + std::to_string(id) + ",'" + s + "')"
    );
    std::cout << "Статус заказа #" << id << " изменен на: " << s << std::endl;
}

void Admin::viewOrderStatusHistory(int id) {
    auto result = db.executeQuery(
        "SELECT * FROM order_status_history WHERE order_id=" + std::to_string(id)
    );
    std::cout << "История статусов заказа #" << id << ":\n";
    for (const auto& row : result) {
        std::cout << "Было: " << row[2] << " -> Стало: " << row[3] 
                  << " (время: " << row[4] << ")\n";
    }
}

void Admin::viewAudit(int id) {
    if (!hasPermission("view_audit")) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    
    auto result = db.executeQuery("SELECT * FROM get_audit_log_by_user(" + std::to_string(id) + ")");
    for (const auto& row : result) {
        std::cout << row[0] << " #" << row[1] << ": " << row[2] 
                  << " (пользователь: " << row[3] << ", время: " << row[4] << ")\n";
    }
}

void Admin::generateCSVReport() {
    if (!hasPermission("generate_report")) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    auto result = db.executeQuery("SELECT generate_csv_report()");
    
    if (result.empty() || result[0].empty()) {
        std::cout << "Не удалось получить данные для отчета\n";
        return;
    }
    std::string csv_data = result[0][0];
    std::ofstream file("orders_report.csv");
    if (!file.is_open()) {
        std::cout << "Не удалось создать файл orders_report.csv\n";
        return;
    }
    
    file << csv_data;
    file.close();
    
    std::cout << "CSV отчет сгенерирован в orders_report.csv\n";
}

void Manager::displayInfo() const {
    std::cout << "Менеджер: " << name << " (ID: " << id << ")\n";
}

bool Manager::hasPermission(const std::string& action) const {
    auto checkPermission = [](const std::string& role, const std::string& action) -> bool {
        if (role != "manager") return false;
        
        static const std::vector<std::string> managerActions = {
            "view_pending_orders", "approve_order", "update_stock",
            "view_order_details", "update_order_status", "view_approved_history",
            "cancel_pending_order"
        };
        
        return std::find(managerActions.begin(), managerActions.end(), action) != managerActions.end();
    };
    
    return checkPermission("manager", action);
}

void Manager::viewOrderStatus(int orderId) {
    auto result = db.executeQuery(
        "SELECT status FROM orders WHERE order_id=" + std::to_string(orderId)
    );
    if (!result.empty()) {
        std::cout << "Статус заказа #" << orderId << ": " << result[0][0] << std::endl;
    } else {
        std::cout << "Заказ не найден\n";
    }
}

void Manager::viewPendingOrders() {
    if (!hasPermission("view_pending_orders")) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    
    auto result = db.executeQuery("SELECT * FROM orders WHERE status='pending'");
    std::cout << "Заказы на утверждение (" << result.size() << "):\n";
    for (const auto& row : result) {
        std::cout << "Заказ #" << row[0] << ": пользователь " << row[1] 
                  << ", сумма: " << row[3] << ", дата: " << row[4] << std::endl;
    }
}

void Manager::approveOrder(int id) {
    if (!hasPermission("approve_order")) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    
    db.executeNonQuery(
        "CALL update_order_status_simple(" + std::to_string(id) + ",'completed'," + std::to_string(this->id) + ")"
    );
    std::cout << "Заказ #" << id << " утвержден и завершен\n";
}

void Manager::updateStock(int id, int q) {
    if (!hasPermission("update_stock")) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    
    db.executeNonQuery(
        "UPDATE products SET stock_quantity=" + std::to_string(q) +
        " WHERE product_id=" + std::to_string(id)
    );
    std::cout << "Количество товара ID=" << id << " обновлено: " << q << std::endl;
}
void Manager::createOrder(){
        std::cout << "Доступ запрещен\n";
        return;
}
void Manager::viewOrderDetails(int id) {
    if (!hasPermission("view_order_details")) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    
    auto result = db.executeQuery(
        "SELECT * FROM order_items WHERE order_id=" + std::to_string(id)
    );
    std::cout << "Детали заказа #" << id << ":\n";
    for (const auto& row : result) {
        std::cout << "Товар: " << row[2] << ", количество: " << row[3] 
                  << ", цена: " << row[4] << std::endl;
    }
}

void Manager::updateOrderStatus(int id, const std::string& s) {
    if (!hasPermission("update_order_status")) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    
    if (s == "pending" || s == "completed" || s == "canceled" || s == "returned") {
        db.executeNonQuery(
            "CALL update_order_status_simple(" + std::to_string(id) + ",'" + s + "'," + std::to_string(this->id) + ")"
        );
        std::cout << "Статус заказа #" << id << " изменен на: " << s << std::endl;
    } else {
        std::cout << "Неверный статус. Допустимые: pending, completed, canceled, returned\n";
    }
}

void Manager::viewApprovedOrdersHistory() {
    if (!hasPermission("view_approved_history")) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    
    auto result = db.executeQuery(
        "SELECT * FROM orders WHERE status='completed'"
    );
    std::cout << "История завершенных заказов (" << result.size() << "):\n";
    for (const auto& row : result) {
        std::cout << "Заказ #" << row[0] << ": пользователь " << row[1] 
                  << ", сумма: " << row[3] << ", дата: " << row[4] << std::endl;
    }
}


void Customer::displayInfo() const {
    std::cout << "Покупатель: " << name << " (ID: " << id << ")\n";
}

bool Customer::hasPermission(const std::string& action, int orderId) const {
    auto checkPermission = [this](const std::string& action, int orderId) -> bool {
        if (action == "view_own_order") {
            if (orderId == -1) return false;
            
            auto result = db.executeQuery(
                "SELECT user_id FROM orders WHERE order_id=" + std::to_string(orderId)
            );
            return !result.empty() && std::stoi(result[0][0]) == this->id;
        }
        
        static const std::vector<std::string> customerActions = {
            "create_order", "view_own_orders", "make_payment", "return_order"
        };
        
        return std::find(customerActions.begin(), customerActions.end(), action) != customerActions.end();
    };
    
    return checkPermission(action, orderId);
}

std::shared_ptr<Order> Customer::findOrderById(int orderId) {
    auto isOrderWithId = [orderId](const std::shared_ptr<Order>& order) {
        return order->getId() == orderId;
    };
    auto foundOrder = std::find_if(orders.begin(), orders.end(), isOrderWithId);
    if (foundOrder != orders.end()) {
        return *foundOrder;
    }
    return nullptr;
}

void Customer::createOrder() {
    if (!hasPermission("create_order")) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    
    db.executeNonQuery(
        "INSERT INTO orders(user_id,status) VALUES (" +
        std::to_string(id) + ",'pending')"
    );
    
    auto result = db.executeQuery(
        "SELECT order_id FROM orders WHERE user_id=" + std::to_string(id) + 
        " ORDER BY order_date DESC LIMIT 1"
    );
    
    if (!result.empty()) {
        int orderId = std::stoi(result[0][0]);
        auto order = std::make_shared<Order>(orderId);
        orders.push_back(order);
        std::cout << "Новый заказ #" << orderId << " создан (статус: pending)\n";
    }
}

void Manager::cancelOrder(int orderId) {
    if (!hasPermission("cancel_pending_order")) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    
    auto result = db.executeQuery(
        "SELECT status FROM orders WHERE order_id=" + std::to_string(orderId)
    );
    
    if (!result.empty() && result[0][0] == "pending") {
        db.executeNonQuery(
            "CALL update_order_status_simple(" + std::to_string(orderId) + ",'canceled'," + std::to_string(this->id) + ")"
        );
        std::cout << "Заказ #" << orderId << " отменен менеджером\n";
    } else {
        std::cout << "Невозможно отменить заказ. Возможно, он уже не в статусе pending\n";
    }
}
void Customer::viewOrderStatus(int orderId) {
    if (!hasPermission("view_own_order", orderId)) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    
    auto result = db.executeQuery(
        "SELECT status FROM orders WHERE order_id=" + std::to_string(orderId) +
        " AND user_id=" + std::to_string(id)
    );
    if (!result.empty()) {
        std::cout << "Статус заказа #" << orderId << ": " << result[0][0] << std::endl;
    } else {
        std::cout << "Заказ не найден\n";
    }
}

void Customer::addToOrder(int o, int p, int q) {
    if (!hasPermission("view_own_order", o)) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    
    db.beginTransaction();
    
    try {
        auto checkResult = db.executeQuery(
            "SELECT user_id FROM orders WHERE order_id=" + std::to_string(o)
        );
        
        if (!checkResult.empty() && std::stoi(checkResult[0][0]) == id) {
            auto priceResult = db.executeQuery(
                "SELECT price FROM products WHERE product_id=" + std::to_string(p)
            );
            
            if (!priceResult.empty()) {
                double price = std::stod(priceResult[0][0]);
                db.executeNonQuery(
                    "INSERT INTO order_items(order_id, product_id, quantity, price) VALUES (" +
                    std::to_string(o) + "," + std::to_string(p) + "," + std::to_string(q) + "," +
                    std::to_string(price) + ")"
                );
                
                auto totalResult = db.executeQuery(
                    "SELECT COALESCE(SUM(quantity * price), 0) FROM order_items WHERE order_id=" + 
                    std::to_string(o)
                );
                
                if (!totalResult.empty()) {
                    double total = std::stod(totalResult[0][0]);
                    db.executeNonQuery(
                        "UPDATE orders SET total_price=" + std::to_string(total) + 
                        " WHERE order_id=" + std::to_string(o)
                    );
                    
                    auto order = findOrderById(o);
                    if (order) {
                        order->setTotalAmount(total);
                    }
                }
                
                std::cout << "Товар добавлен в заказ\n";
                db.commitTransaction();
            } else {
                std::cout << "Товар не найден\n";
                db.rollbackTransaction();
            }
        } else {
            std::cout << "Заказ не найден или нет доступа\n";
            db.rollbackTransaction();
        }
    } catch (...) {
        db.rollbackTransaction();
        throw;
    }
}

void Customer::removeFromOrder(int o, int p) {
    if (!hasPermission("view_own_order", o)) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    
    db.beginTransaction();
    
    try {
        auto checkResult = db.executeQuery(
            "SELECT user_id FROM orders WHERE order_id=" + std::to_string(o)
        );
        
        if (!checkResult.empty() && std::stoi(checkResult[0][0]) == id) {
            db.executeNonQuery(
                "DELETE FROM order_items WHERE order_id=" +
                std::to_string(o) + " AND product_id=" + std::to_string(p)
            );
            
            auto totalResult = db.executeQuery(
                "SELECT COALESCE(SUM(quantity * price), 0) FROM order_items WHERE order_id=" + 
                std::to_string(o)
            );
            
            if (!totalResult.empty()) {
                double total = std::stod(totalResult[0][0]);
                db.executeNonQuery(
                    "UPDATE orders SET total_price=" + std::to_string(total) + 
                    " WHERE order_id=" + std::to_string(o)
                );
                
                auto order = findOrderById(o);
                if (order) {
                    order->setTotalAmount(total);
                }
            }
            
            std::cout << "Товар удален из заказа\n";
            db.commitTransaction();
        } else {
            std::cout << "Заказ не найден или нет доступа\n";
            db.rollbackTransaction();
        }
    } catch (...) {
        db.rollbackTransaction();
        throw;
    }
}

void Customer::viewMyOrders() {
    if (!hasPermission("view_own_orders")) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    
    loadOrdersFromDB();
    
    double sum = calculateTotalSpent();
    std::cout << "Мои заказы (" << orders.size() << "):\n";
    std::cout << "Всего потрачено: " << sum << "\n";
    
    std::cout << "ЗАКАЗЫ ПО СТАТУСАМ \n";
    
    std::vector<std::string> statuses = {"pending", "completed", "canceled", "returned"};
    for (const auto& status : statuses) {
        auto filteredOrders = getOrdersByStatus(status);
        if (!filteredOrders.empty()) {
            std::cout << "\nСтатус: " << status << " (" << filteredOrders.size() << "):\n";
            for (const auto& order : filteredOrders) {
                auto result = db.executeQuery(
                    "SELECT total_price FROM orders WHERE order_id=" + std::to_string(order->getId())
                );
                if (!result.empty()) {
                    std::cout << "  Заказ #" << order->getId() << ": " 
                              << result[0][0] << " руб.\n";
                }
            }
        }
    }
}

void Customer::viewOrderStatusHistory(int id) {
    if (!hasPermission("view_own_order", id)) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    
    auto checkResult = db.executeQuery(
        "SELECT order_id FROM orders WHERE order_id=" + std::to_string(id) +
        " AND user_id=" + std::to_string(this->id)
    );
    
    if (!checkResult.empty()) {
        auto result = db.executeQuery(
            "SELECT * FROM order_status_history WHERE order_id=" + std::to_string(id)
        );
        if (!result.empty()) {
            std::cout << "История статусов заказа #" << id << ":\n";
            for (const auto& row : result) {
                std::cout << "Было: " << row[2] << " -> Стало: " << row[3] 
                          << " (время: " << row[4] << ")\n";
            }
        } else {
            std::cout << "История не найдена\n";
        }
    } else {
        std::cout << "Заказ не найден или нет доступа\n";
    }
}

bool Customer::canReturnOrder(int orderId) const {
    auto result = db.executeQuery(
        "SELECT can_return_order(" + std::to_string(orderId) + ")"
    );
    
    if (!result.empty()) {
        return result[0][0] == "t";
    }
    
    return false;
}

void Customer::makePayment(int orderId) {
    if (!hasPermission("make_payment")) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    
    auto checkResult = db.executeQuery(
        "SELECT order_id, total_price FROM orders WHERE order_id=" + 
        std::to_string(orderId) + " AND user_id=" + std::to_string(this->id) +
        " AND status='pending'"
    );
    
    if (!checkResult.empty()) {
        std::cout << "Выберите способ оплаты:\n";
        std::cout << "1. Карта\n2. Электронный кошелек\n3. СБП\n> ";
        int option;
        std::cin >> option;
        
        double total = std::stod(checkResult[0][1]);
        
        auto order = findOrderById(orderId);
        if (!order) {
            order = std::make_shared<Order>(orderId);
            order->setTotalAmount(total);
            orders.push_back(order);
        }
        
        order->setPayment(choosePayment(option));
        
        if (order->pay()) {
            std::cout << "Оплата " << total << " руб. прошла успешно\n";
            db.executeNonQuery(
                "CALL pay_order_simple(" + std::to_string(orderId) + "," + std::to_string(this->id) + ")"
            );
            std::cout << "Статус заказа #" << orderId << " изменен на: completed\n";
        } else {
            std::cout << "Оплата не удалась\n";
        }
    } else {
        std::cout << "Заказ не найден, нет доступа или нельзя оплатить\n";
    }
}

void Customer::returnOrder(int orderId) {
    if (!hasPermission("return_order")) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    
    if (!canReturnOrder(orderId)) {
        std::cout << "Невозможно вернуть заказ. Проверьте:\n";
        std::cout << "1. Заказ должен быть завершен (status='completed')\n";
        std::cout << "2. С момента заказа должно пройти не более 30 дней\n";
        return;
    }
    
    db.executeNonQuery(
        "CALL return_order_simple(" + std::to_string(orderId) + "," + std::to_string(this->id) + ")"
    );
    std::cout << "Заказ #" << orderId << " возвращен\n";
}
void Customer::cancelOrder(int orderId) {
    if (!hasPermission("view_own_order", orderId)) {
        std::cout << "Доступ запрещен\n";
        return;
    }
    
    auto result = db.executeQuery(
        "SELECT status FROM orders WHERE order_id=" + std::to_string(orderId) +
        " AND user_id=" + std::to_string(id)
    );
    
    if (!result.empty() && result[0][0] == "pending") {
        db.executeNonQuery(
            "UPDATE orders SET status='canceled' WHERE order_id=" + std::to_string(orderId)
        );
        
        removeOrder(orderId);
        
        std::cout << "Заказ #" << orderId << " отменен\n";
    } else {
        std::cout << "Невозможно отменить заказ. Возможно, он уже не в статусе pending\n";
    }
}