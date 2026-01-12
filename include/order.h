#ifndef ORDER_H
#define ORDER_H

#include <vector>
#include <memory>
#include <numeric>
#include <algorithm>
#include <string>
#include "payment.h"

class OrderItem {
private:
    int productId;
    std::string name;
    int quantity;
    double price;

public:
    OrderItem(int productId, std::string name, int quantity, double price);
    int getProductId() const;
    double getTotal() const;
    double getPrice() const { return price; }
};

class Order {
private:

    int id;
    std::vector<OrderItem> items;
    std::unique_ptr<PaymentStrategy> payment;
    double totalAmount = 0.0;
    std::string status; 

public:
    Order(int id);
    
    int getId() const { return id; }
    double getTotalAmount() const { return totalAmount; }
    const std::string& getStatus() const { return status; }
    void setStatus(const std::string& s) { status = s; } 
    void setTotalAmount(double amount) { totalAmount = amount; }

    void addItem(const OrderItem& item);
    void removeItem(int productId);
    
    std::vector<OrderItem> getItemsByPriceRange(double minPrice, double maxPrice) const;
    
    double total() const;
    
    void setPayment(std::unique_ptr<PaymentStrategy> p);
    bool pay();
    
    bool canCancel() const { return status == "pending"; }
    
    bool canReturn() const { return status == "completed"; }
};
#endif 
