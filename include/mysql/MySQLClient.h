#ifndef MYSQL_CLIENT_H
#define MYSQL_CLIENT_H

#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <map>

class MySQLClient {
public:
    MySQLClient(const std::string& host = "localhost", unsigned int port = 3306, const std::string& database = "myserver", 
                const std::string& user = "loki", const std::string& password = "Loki4819");
    ~MySQLClient();

    bool connect();
    void disconnect();
    bool isConnected() const;

    bool execute(const std::string& sql); // 执行查询
    std::vector<std::map<std::string, std::string>> query(const std::string& sql); // 执行查询并返回结果

    std::string getError() const;               // 获取错误信息
    unsigned long long getAffectedRows() const; // 获取影响的行数
    unsigned long long getInsertId() const;     // 获取插入的ID

private:
    MYSQL* mysql_;
    std::string host_;
    unsigned int port_;
    bool connected_;
    std::string database_;
    std::string user_;
    std::string password_;
};

#endif // MYSQL_CLIENT_H