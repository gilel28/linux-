#ifndef BLOCK_IO_H
#define BLOCK_IO_H

#include <vector>
#include <string>
#include "blockStruct.h"


void printBlock(const Block& block);

void printBlocks(std::vector<Block> blocks);

bool findHash(std::string target_hash, std::vector<Block> blocks);

void find_height(int target_height, std::vector<Block> blocks);

void createCsv();

void reload( std::vector<Block> blocks);

 std::vector<Block> getBlocksFromFile(const std::string& filename);
#endif
