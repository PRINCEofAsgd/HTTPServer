#include "../../include/mysql/MySQLClient.h"
#include <cstdio>

MySQLClient::MySQLClient(const std::string& host, unsigned int port, 
    const std::string& database, const std::string& user, const std::string& password) : 
    host_(host), port_(port), connected_(false) , database_(database), user_(user), password_(password) 
{
    mysql_ = mysql_init(nullptr);
    if (!mysql_) fprintf(stderr, "mysql_init failed\n");
}

MySQLClient::~MySQLClient() {
    disconnect();
    if (mysql_) {
        mysql_close(mysql_);
        mysql_ = nullptr;
    }
}

bool MySQLClient::connect() {
    if (connected_) return true;
    if (!mysql_) {
        mysql_ = mysql_init(nullptr);
        if (!mysql_) return false;
    }

    mysql_options(mysql_, MYSQL_SET_CHARSET_NAME, "utf8"); // 设置字符集为utf8
    
    // 连接MySQL服务器
    if (!mysql_real_connect(mysql_, host_.c_str(), user_.c_str(), password_.c_str(), 
    database_.c_str(), port_, nullptr, 0)) return false;

    connected_ = true;
    return true;
}

void MySQLClient::disconnect() {
    if (connected_ && mysql_) {
        mysql_close(mysql_);
        mysql_ = mysql_init(nullptr);
        connected_ = false;
    }
}

bool MySQLClient::isConnected() const { return connected_; }

bool MySQLClient::execute(const std::string& sql) {
    if (!connected_) if (!connect()) return false;
    if (mysql_query(mysql_, sql.c_str())) return false;
    return true;
}

std::vector<std::map<std::string, std::string>> MySQLClient::query(const std::string& sql) {
    std::vector<std::map<std::string, std::string>> result;
    if (!connected_) if (!connect()) return result;

    // 执行查询
    if (mysql_query(mysql_, sql.c_str())) return result;

    // 获取查询结果
    MYSQL_RES* res = mysql_store_result(mysql_);
    if (!res) return result;

    // 获取字段信息
    MYSQL_FIELD* fields = mysql_fetch_fields(res);
    int num_fields = mysql_num_fields(res);

    // 遍历结果集
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        std::map<std::string, std::string> row_data;
        unsigned long* lengths = mysql_fetch_lengths(res);
        
        for (int i = 0; i < num_fields; ++i) {
            std::string field_name = fields[i].name;
            std::string field_value;
            if (row[i]) field_value = std::string(row[i], lengths[i]);
            row_data[field_name] = field_value;
        }
        
        result.push_back(row_data);
    }

    mysql_free_result(res);
    return result;
}

std::string MySQLClient::getError() const {
    if (mysql_) return mysql_error(mysql_);
    return "MySQL client not initialized";
}

unsigned long long MySQLClient::getAffectedRows() const {
    if (mysql_) return mysql_affected_rows(mysql_);
    return 0;
}

unsigned long long MySQLClient::getInsertId() const {
    if (mysql_) return mysql_insert_id(mysql_);
    return 0;
}