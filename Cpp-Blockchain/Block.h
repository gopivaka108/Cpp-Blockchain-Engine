#include<iostream>
#include<cstdint>
#include<string>
#include<ctime>

class Block {
public:
    std::string sPrevHash;

    Block(int Index, std::string Data);

    std::string GetHash();
    void MineBlock(int nDifficulty);
private:
    uint32_t _nIndex;
    uint64_t _nNonce;
    std::string _sHash;
    std::string _sData;
    time_t _tTime;

    std::string _CalculateHash() const;
};