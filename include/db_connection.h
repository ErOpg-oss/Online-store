#ifndef DB_CONNECTION_H
#define DB_CONNECTION_H

#include <pqxx/pqxx>
#include <memory>
#include <vector>
#include <string>

template<typename T>
class DatabaseConnection {
    std::unique_ptr<pqxx::connection> conn;
    std::unique_ptr<pqxx::work> txn;

public:
    DatabaseConnection(const std::string& connStr) {
        conn = std::make_unique<pqxx::connection>(connStr);
        if (!conn->is_open())
            throw std::runtime_error("DB connection failed");
    }

    std::vector<std::vector<T>> executeQuery(const std::string& sql) {
        if (!txn) {
            pqxx::work w(*conn);
            pqxx::result r = w.exec(sql);
            
            std::vector<std::vector<T>> result;
            for (const auto& row : r) {
                std::vector<T> line;
                for (const auto& field : row)
                    line.push_back(field.as<T>());
                result.push_back(line);
            }
            return result;
        } else {
            pqxx::result r = txn->exec(sql);
            
            std::vector<std::vector<T>> result;
            for (const auto& row : r) {
                std::vector<T> line;
                for (const auto& field : row)
                    line.push_back(field.as<T>());
                result.push_back(line);
            }
            return result;
        }
    }
    void executeNonQuery(const std::string& sql) {
        if (!txn) beginTransaction();
        txn->exec(sql);
        commitTransaction();
    }

    void beginTransaction() {
        txn = std::make_unique<pqxx::work>(*conn);
    }

    void commitTransaction() {
        if (txn) {
            txn->commit();
            txn.reset();
        }
    }

    void rollbackTransaction() {
        txn.reset();
    }

    void createFunction(const std::string& sql) {
        executeNonQuery(sql);
    }

    void createTrigger(const std::string& sql) {
        executeNonQuery(sql);
    }

    bool getTransactionStatus() const {
        return txn.get() != nullptr;
    }

    ~DatabaseConnection() {
        if (txn) {
            txn->abort();
        }
        conn.reset();
    }
};

#endif
