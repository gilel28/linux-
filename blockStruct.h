#include <string>

struct Block {
    std::string hash;
    int height;
    long long total;
    std::string time;
    std::string received_time;
    std::string relayed_by;
    std::string prev_block;
};