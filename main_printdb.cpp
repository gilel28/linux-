#include <iostream>
#include "blockutils.h"

/*void printBlock(const Block& block) {
    std::cout << "hash: " << block.hash << "\n";
    std::cout << "height: " << block.height << "\n";
    std::cout << "total: " << block.total << "\n";
    std::cout << "time: " << block.time << "\n";
    std::cout << "relayed_by: " << block.relayed_by << "\n";
    std::cout << "prev_block: " << block.prev_block << "\n";
}
 void printBlocks(std::vector<Block> blocks)
 {
    for (size_t i = 0; i < blocks.size(); ++i) {
        printBlock(blocks[i]);
        if (i != blocks.size() - 1)
            std::cout << "   |\n   |\n   V\n";
    }

 }
    */
int main() {
    std::vector<Block> blocks = getBlocksFromFile("blocks.txt");
    printBlocks(blocks);
   
    return 0;
}
