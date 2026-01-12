#include "../include/order.h"

OrderItem::OrderItem(int productId, std::string name, int quantity, double price)
    : productId(productId), name(std::move(name)), quantity(quantity), price(price) {}

int OrderItem::getProductId() const {
    return productId;
}

double OrderItem::getTotal() const {
    return quantity * price;
}


Order::Order(int id) : id(id), status("pending") {}

void Order::addItem(const OrderItem& item) {
    items.push_back(item);
}

void Order::removeItem(int productId) {
    auto predicate = [productId](const OrderItem& item) {
        return item.getProductId() == productId;
    };
    
    items.erase(
        std::remove_if(items.begin(), items.end(), predicate),
        items.end()
    );
}

std::vector<OrderItem> Order::getItemsByPriceRange(double minPrice, double maxPrice) const {
    std::vector<OrderItem> filteredItems;
        std::copy_if(items.begin(), items.end(), std::back_inserter(filteredItems),
        [minPrice, maxPrice](const OrderItem& item) {
            return item.getPrice() >= minPrice && item.getPrice() <= maxPrice;
        });
    
    return filteredItems;
}
double Order::total() const {
    if (totalAmount > 0) {
        return totalAmount;
    }
    
    return std::accumulate(
        items.begin(), items.end(), 0.0,
        [](double sum, const OrderItem& i) {
            return sum + i.getTotal();
        }
    );
}

void Order::setPayment(std::unique_ptr<PaymentStrategy> p) {
    payment = std::move(p);
}

bool Order::pay() {
    return payment && payment->pay(total());
}
