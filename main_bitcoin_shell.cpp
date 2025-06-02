
#include <iostream>
#include <vector>
#include <string>
#include "blockutils.h"

int main() {
    while (true) {
        std::vector<Block> blocks = getBlocksFromFile("blocks.txt");

        std::cout << "\nChoose an option:\n";
        std::cout << "1. Print db\n";
        std::cout << "2. Print block by hash\n";
        std::cout << "3. Print block by height\n";
        std::cout << "4. Export data to csv\n";
        std::cout << "5. Refresh data\n";
        std::cout << "0. Exit\n";
        std::cout << "Enter your choice: ";

        int choice;
        std::cin >> choice;

        if (choice == 0) break;

        switch (choice) {
            case 1:
                printBlocks(blocks);
                break;
            case 2: {
                std::string hash;
                std::cout << "Enter block hash: ";
                std::cin >> hash;
                if (!findHash(hash, blocks)) {
                    std::cout << "Block not found.\n";
                }
                break;
            }
            case 3: {
                int height;
                std::cout << "Enter block height: ";
                std::cin >> height;
                find_height(height, blocks);
                break;
            }
            case 4:
                createCsv();
                break;
            case 5:
                reload(blocks);
                break;
            default:
                std::cout << "Invalid choice.\n";
        }
    }

    return 0;
}
