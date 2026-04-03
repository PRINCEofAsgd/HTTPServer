#pragma once
#include <iostream>

class Timestamp {
private:
    time_t sec_since_epoch_;

public:
    Timestamp();
    Timestamp(int64_t sec_since_epoch);
    ~Timestamp();

    void set_now();
    static Timestamp now();
    time_t get_toint() const;
    std::string get_tostring() const;
};
