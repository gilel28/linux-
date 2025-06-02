#include <iostream>
#include <vector>
#include <string>
#include "blockutils.h"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " --hash <block_hash> | --height <block_height>\n";
        return 1;
    }

    std::string mode = argv[1];
    std::vector<Block> blocks = getBlocksFromFile("blocks.txt");

    if (mode == "--hash") {
        std::string target_hash = argv[2];
        bool found = findHash(target_hash, blocks);
        if (!found) {
            std::cerr << "Block with hash " << target_hash << " not found.\n";
            return 1;
        }
    } else if (mode == "--height") {
        int target_height = std::stoi(argv[2]);
        find_height(target_height, blocks);
    } else {
        std::cerr << "Unknown option: " << mode << "\n";
        std::cerr << "Usage: " << argv[0] << " --hash <block_hash> | --height <block_height>\n";
        return 1;
    }

    return 0;
}