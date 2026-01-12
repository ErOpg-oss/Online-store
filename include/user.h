#ifndef USER_H
#define USER_H

#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <functional>
#include "db_connection.h"
#include "order.h"

class User {
protected:
    int id;
    std::string name;
    DatabaseConnection<std::string>& db;
    std::vector<std::shared_ptr<Order>> orders; 

public:
    User(int id, std::string name, DatabaseConnection<std::string>& db);
    virtual ~User() = default;
    
    virtual void displayInfo() const = 0;
    virtual void createOrder() = 0; 
    virtual void cancelOrder(int orderId) = 0; 
    virtual void viewOrderStatus(int orderId) = 0;
    void viewAllProducts();
    
    void loadOrdersFromDB();
    std::vector<std::shared_ptr<Order>> getOrdersByStatus(const std::string& status) const;
    double calculateTotalSpent() const;
    
    int getId() const { return id; }
    const std::string& getName() const { return name; }
    const std::vector<std::shared_ptr<Order>>& getOrders() const { return orders; }
    
    void addOrder(std::shared_ptr<Order> order) { orders.push_back(order); }
    void removeOrder(int orderId);
};

class Admin : public User {
public:
    using User::User;
    void displayInfo() const override;
    void createOrder() override;
    void cancelOrder(int orderId) override;
    void viewOrderStatus(int orderId) override;


    void addProduct(const std::string&, double, int);
    void updateProduct(int, double, int);
    void deleteProduct(int);

    void viewAllOrders();
    void viewOrderDetails(int);
    void updateOrderStatus(int, const std::string&);
    void viewOrderStatusHistory(int);

    void viewAudit(int id);
    void generateCSVReport();
    
    bool hasPermission(const std::string& action) const;
};

class Manager : public User {
public:
    using User::User;
    void displayInfo() const override;
    void createOrder() override;
    void cancelOrder(int orderId) override;
    void viewOrderStatus(int orderId) override;

    void viewPendingOrders();
    void approveOrder(int);
    void updateStock(int, int);
    void viewOrderDetails(int);
    void updateOrderStatus(int, const std::string&);
    void viewApprovedOrdersHistory();
    bool hasPermission(const std::string& action) const;
};

class Customer : public User {
public:
    using User::User;
    void displayInfo() const override;
    void createOrder() override; 
    void cancelOrder(int orderId) override;
    void viewOrderStatus(int orderId) override;

    void addToOrder(int, int, int);
    void removeFromOrder(int, int);

    void viewMyOrders();
    void viewOrderStatusHistory(int);

    void makePayment(int);
    void returnOrder(int);
    
    bool canReturnOrder(int orderId) const;
    
    bool hasPermission(const std::string& action, int orderId = -1) const;

private:
    std::shared_ptr<Order> findOrderById(int orderId);
};

#endif
