#ifndef PAYMENT_H
#define PAYMENT_H
#include <string>
#include <memory>

class PaymentStrategy {
public:
    virtual bool pay(double amount) = 0;
    virtual std::string name() const = 0;
    virtual ~PaymentStrategy() = default;
};

class CardPayment : public PaymentStrategy {
public:
    bool pay(double amount) override;
    std::string name() const override;
};

class WalletPayment : public PaymentStrategy {
public:
    bool pay(double amount) override;
    std::string name() const override;
};

class SBPPayment : public PaymentStrategy {
public:
    bool pay(double amount) override;
    std::string name() const override;
};

std::unique_ptr<PaymentStrategy> choosePayment(int option);
#endif