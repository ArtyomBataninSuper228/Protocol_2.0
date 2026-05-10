//
//  stuff.hpp
//  Protocol_UDP
//
//  Created by Артем Батанин on 07.05.2026.
//

#include <vector>
#include <string>
#include <cstdint>


// Функция для генерации таблицы CRC32
std::vector<uint32_t> generate_crc_table() {
    std::vector<uint32_t> table(256);
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t ch = i;
        for (size_t j = 0; j < 8; ++j) {
            ch = (ch & 1) ? (0xEDB88320 ^ (ch >> 1)) : (ch >> 1);
        }
        table[i] = ch;
    }
    return table;
}

// Вычисление CRC32
uint32_t crc32(const std::string& data) {
    static auto table = generate_crc_table();
    uint32_t crc = 0xFFFFFFFF;
    for (char byte : data) {
        crc = (crc >> 8) ^ table[(crc ^ static_cast<uint8_t>(byte)) & 0xFF];
    }
    return ~crc;
}


uint32_t crc32(unsigned char *buf, size_t start,  size_t end){
    static auto table = generate_crc_table();
    uint_least32_t crc;
    crc = 0xFFFFFFFFUL;
    for(int i = 0; i < start; i++){
        buf++;
    }
    while (end > start){
        
        crc = table[(crc ^ *buf++) & 0xFF] ^ (crc >> 8);
        start++;
    }
    return crc ^ 0xFFFFFFFFUL;
}

uint32_t crc32(const char *buf, size_t start,  size_t end){
    static auto table = generate_crc_table();
    uint_least32_t crc;
    crc = 0xFFFFFFFFUL;
    for(int i = 0; i < start; i++){
        buf++;
    }
    while (end > start){
        crc = table[(crc ^ *buf++) & 0xFF] ^ (crc >> 8);
        start++;
        
    }
    return crc ^ 0xFFFFFFFFUL;
}



#ifndef stuff_
#define stuff_

/* The classes below are exported */
#pragma GCC visibility push(default)
std::vector<uint32_t> generate_crc_table();
uint32_t crc32(const std::string& data);
uint32_t crc32(unsigned char *buf, size_t start,  size_t end);
uint32_t crc32(const char *buf, size_t start,  size_t end);

#pragma GCC visibility pop
#endif
