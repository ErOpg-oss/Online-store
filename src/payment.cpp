#include "../include/payment.h"
#include <iostream>

bool CardPayment::pay(double amount) {
    std::cout << "Оплата картой: " << amount << "\n";
    return true;
}
std::string CardPayment::name() const { return "Card"; }

bool WalletPayment::pay(double amount) {
    std::cout << "Оплата кошельком: " << amount << "\n";
    return true;
}
std::string WalletPayment::name() const { return "Wallet"; }

bool SBPPayment::pay(double amount) {
    std::cout << "Оплата СБП: " << amount << "\n";
    return true;
}
std::string SBPPayment::name() const { return "SBP"; }

std::unique_ptr<PaymentStrategy> choosePayment(int option) {
    if (option == 1) return std::make_unique<CardPayment>();
    if (option == 2) return std::make_unique<WalletPayment>();
    return std::make_unique<SBPPayment>();
}