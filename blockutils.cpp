#include "blockutils.h"
#include <fstream>
#include <sstream>
#include <iostream>  

void printBlock(const Block& block) {
    std::cout << "hash: " << block.hash << "\n";
    std::cout << "height: " << block.height << "\n";
    std::cout << "total: " << block.total << "\n";
    std::cout << "time: " << block.time << "\n";
    std::cout << "relayed_by: " << block.relayed_by << "\n";
    std::cout << "prev_block: " << block.prev_block << "\n";
}

void printBlocks(std::vector<Block> blocks) {
    for (size_t i = 0; i < blocks.size(); ++i) {
        printBlock(blocks[i]);
        if (i != blocks.size() - 1)
            std::cout << "   |\n   |\n   V\n";
    }
}

bool findHash(std::string target_hash, std::vector<Block> blocks) {
    for (const auto& block : blocks) {
        if (block.hash == target_hash) {
            printBlock(block);
            return true;
        }
    }
    return false;
}

void find_height(int target_height, std::vector<Block> blocks) {
    for (const auto& block : blocks) {
        if (block.height == target_height) {
            printBlock(block);
            return;
        }
    }
    std::cerr << "Block with height " << target_height << " not found.\n";
}

void createCsv() {
    std::vector<Block> blocks = getBlocksFromFile("blocks.txt");

    std::ofstream csv("a.csv");
    if (!csv.is_open()) {
        std::cerr << "Failed to create a.csv\n";
        return;
    }

    csv << "hash_value,height,total,time,relayed_by,prev_block\n";
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
}

void reload(std::vector<Block> blocks) {
    int block_count = blocks.size();

    if (block_count == 0) {
        std::cerr << "No blocks found. Check blocks.txt\n";
        return;
    }

    std::string command = "./matala.sh " + std::to_string(block_count);
    int result = system(command.c_str());

    if (result != 0) {
        std::cerr << "Failed to execute script.\n";
        return;
    }

    std::cout << "Reloaded database with " << block_count << " blocks.\n";
}

std::vector<Block> getBlocksFromFile(const std::string& filename) {
    std::ifstream file(filename);
    std::vector<Block> blocks;

    if (!file.is_open()) {
        std::cerr << "Failed to open file.\n";
        return blocks;
    }

    std::string line;
    Block current;

    while (std::getline(file, line)) {
        if (line.find("Hash: ") == 0)
            current.hash = line.substr(6);
        else if (line.find("Height: ") == 0)
            current.height = std::stoi(line.substr(8));
        else if (line.find("Total: ") == 0)
            current.total = std::stoll(line.substr(7));
        else if (line.find("Time: ") == 0)
            current.time = line.substr(6);
        else if (line.find("Relayed by: ") == 0)
            current.relayed_by = line.substr(13);
        else if (line.find("Previous Block: ") == 0)
            current.prev_block = line.substr(16);
        else if (line.find("------------------------") == 0) {
            blocks.push_back(current);
            current = Block();
        }
    }

    return blocks;
}
