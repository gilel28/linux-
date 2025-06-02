#include <iostream>
#include <fstream>
#include <vector>
#include "blockutils.h"


int main() {
    std::vector<Block> blocks = getBlocksFromFile("blocks.txt");

    std::ofstream csv("a.csv");
    if (!csv.is_open()) {
        std::cerr << "Failed to create a.csv\n";
        return 1;
    }

    // כותרת
    csv << "hash_value,height,total,time,relayed_by,prev_block\n";

    // תוכן
    for (const auto& block : blocks) {
        csv << block.hash << ','
            << block.height << ','
            << block.total << ','
            << block.time << ','
            << block.relayed_by << ','
            << block.prev_block << '\n';
    }

    csv.close();
    std::cout << "Exported to a.csv\n";
    return 0;
}
