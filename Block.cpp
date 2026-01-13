#include "Block.h"

Block::Block(int Index, std::string Data) {
    _nIndex = Index;
    _sData = Data;
    _nNonce = 0;
    _tTime = time(nullptr);
}

std::string Block::GetHash() {
    return _sHash;
}

void Block::MineBlock(int _Difficulty) {
    _sHash = _CalculateHash();
}

std::string Block::_CalculateHash() const {
    return std::to_string(_nIndex) + _sData + std::to_string(_nNonce);
}
