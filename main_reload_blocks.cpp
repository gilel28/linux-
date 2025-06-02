#include <iostream>
#include <cstdlib>   // system
#include <vector>
#include <string>
#include "blockutils.h"


int main() {
    std::vector<Block> blocks = getBlocksFromFile("blocks.txt");
    int block_count = blocks.size();

    if (block_count == 0) {
        std::cerr << "No blocks found. Check blocks.txt\n";
        return 1;
    }

    std::string command = "./matala.sh " + std::to_string(block_count);
    int result = system(command.c_str());

    if (result != 0) {
        std::cerr << "Failed to execute script.\n";
        return 1;
    }

    std::cout << "Reloaded database with " << block_count << " blocks.\n";
    return 0;
}
