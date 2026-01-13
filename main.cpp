#include <iostream>
#include "Block.h"
#include <vector>

int main() {
    std::vector<Block> Chain;

    // 1. Genesis Block
    Block b1(0, "Genesis Block");
    b1.MineBlock(1);
    Chain.push_back(b1);

    // 2. Second Block
    Block b2(1, "Second Block");
    b2.sPrevHash = b1.GetHash(); // Link them
    b2.MineBlock(1);
    Chain.push_back(b2);

    // 3. Print
    for(Block b : Chain) {
        std::cout << "Hash      : " << b.GetHash() << std::endl;
        std::cout << "Prev Hash : " << b.sPrevHash << std::endl; // Fixed line
        std::cout << "------------------------" << std::endl;
    }

    return 0;
}