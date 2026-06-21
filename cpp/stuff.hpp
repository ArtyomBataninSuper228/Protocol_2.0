//
//  stuff.hpp
//  Protocol_UDP
//
//  Created by Артем Батанин on 07.05.2026.
//

#ifndef stuff_
#define stuff_

// 1. Все необходимые системные библиотеки
#include <vector>
#include <string>
#include <cstdint>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <array>
#include <sodium.h>
#include <chrono>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>


#pragma GCC visibility push(default)

// ============================================================================
// БЛОК ФУНКЦИЙ CRC32
// ============================================================================

// Функция для генерации таблицы CRC32
inline std::vector<uint32_t> generate_crc_table() {
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

// Вычисление CRC32 для std::string
inline uint32_t crc32(const std::string& data) {
    static auto table = generate_crc_table();
    uint32_t crc = 0xFFFFFFFF;
    for (char byte : data) {
        crc = (crc >> 8) ^ table[(crc ^ static_cast<uint8_t>(byte)) & 0xFF];
    }
    return ~crc;
}

// Вычисление CRC32 для unsigned char*
inline uint32_t crc32(unsigned char *buf, size_t start, size_t end) {
    static auto table = generate_crc_table();
    uint_least32_t crc = 0xFFFFFFFFUL;
    for(size_t i = 0; i < start; i++) {
        buf++;
    }
    while (end > start) {
        crc = table[(crc ^ *buf++) & 0xFF] ^ (crc >> 8);
        start++;
    }
    return crc ^ 0xFFFFFFFFUL;
}

// Вычисление CRC32 для const char*
inline uint32_t crc32(const char *buf, size_t start, size_t end) {
    static auto table = generate_crc_table();
    uint_least32_t crc = 0xFFFFFFFFUL;
    for(size_t i = 0; i < start; i++) {
        buf++;
    }
    while (end > start) {
        crc = table[(crc ^ *buf++) & 0xFF] ^ (crc >> 8);
        start++;
    }
    return crc ^ 0xFFFFFFFFUL;
}

// ============================================================================
// БЛОК СТРУКТУР И КЛАССОВ ОБЩЕГО НАЗНАЧЕНИЯ
// ============================================================================

struct LogEntry{
    int id;
    int level;
    int64_t timestamp_ns;
    std::string time_str;
    std::string place;
    std::string text;
    
    std::string to_json() const {
            nlohmann::json j;
            j["id"] = id;
            j["level"] = level;
            j["timestamp_ns"] = timestamp_ns;
            j["time"] = time_str;
            j["place"] = place;
            j["text"] = text; // Если тут будут кавычки или \n, библиотека сама их заэкранирует!

            // j.dump() — это полный аналог json.dumps(j) в Python
            return j.dump();
        }
};
class Logger{
private:
    std::deque<LogEntry> logs_;
    int loglevel_ = 1;
    int printlevel = 1;
    size_t loglength_ = 10000;
    int next_id_ = 0;
    std::mutex mutex_;
    asio::steady_timer timer_;

public:
    Logger(asio::io_context& io, size_t loglength = 10000)
            : loglength_(loglength), timer_(io) {}

    void log(int level, const std::string& place, const std::string& text) {
        std::lock_guard<std::mutex> lock(mutex_); // Блокируем логгер на время записи
        auto now = std::chrono::system_clock::now();
        int64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
        std::string ctime_raw = std::ctime(&now_time_t);
        if (!ctime_raw.empty() && ctime_raw.back() == '\n') {
                    ctime_raw.pop_back();
                }
        if (level >= loglevel_){
            logs_.push_back({next_id_++, level, ns, ctime_raw, place, text});
            if (logs_.size() > loglength_) {
                logs_.pop_front();
            }
        }
        
        
        if (level >= printlevel) {
                    std::string level_str = "Unknown";
                    switch (level) {
                        case 0: level_str = "Debug";     break;
                        case 1: level_str = "Info";      break;
                        case 2: level_str = "Warning";   break;
                        case 3: level_str = "Exception"; break;
                        case 4: level_str = "Critical";  break;
                        default: break;
                    }
            std::cout << "{" << ctime_raw << "} " << "[" << place << "] " <<  "|" << level_str << "| " << text << std::endl;
        }
        
    }

    // Метод для получения копии логов (например, для отправки админу)
    std::deque<LogEntry> get_logs() {
        std::lock_guard<std::mutex> lock(mutex_);
        return logs_;
    }
    
    std::string export_all_to_json() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Создаем пустой JSON-массив
        nlohmann::json j_array = nlohmann::json::array();
        
        for (const auto& entry : logs_) {
            nlohmann::json j;
            j["id"] = entry.id;
            j["level"] = entry.level;
            j["timestamp_ns"] = entry.timestamp_ns;
            j["time"] = entry.time_str;
            j["place"] = entry.place;
            j["text"] = entry.text;
            
            j_array.push_back(j); // Просто пушим объект в массив
        }
        
        // dump(2) означает сделать красивый отступ в 2 пробела (как indent=2 в Python)
        // Если нужно сжать в одну строку без пробелов — вызывай просто dump() без параметров
        return j_array.dump(2);
    }
    void save_to_file(const std::string& target_file) {
            // Получаем готовую JSON строку (метод сам заблокирует мьютекс, всё безопасно)
            std::string json_data = export_all_to_json();

            std::ofstream file(target_file, std::ios::out | std::ios::trunc); // Перезаписываем свежие логи
            if (file.is_open()) {
                file << json_data;
                file.close();
                // Выводим системное уведомление (минуя мьютекс логгера, напрямую в консоль)
                std::cout << "[Logger] Логи успешно сохранены в файл: " << target_file << std::endl;
            } else {
                std::cerr << "[Logger] КРИТИЧЕСКАЯ ОШИБКА: Не удалось открыть файл для записи логов!" << std::endl;
            }
        }
    void start_autosave(int interval_seconds = 60, const std::string& filename = "server_logs.json") {
        timer_.expires_after(std::chrono::seconds(interval_seconds));
        
        // Добавляем mutable в конец списка аргументов лямбды!
        timer_.async_wait([this, interval_seconds, filename](const std::error_code& ec) mutable {
            if (!ec) {
                save_to_file(filename);
                start_autosave(interval_seconds, filename);
            }
        });
    }
};

// ============================================================================
// БЛОК СТРУКТУР И КЛАССОВ ДЛЯ СЕТЕВОГО СОЕДИНЕНИЯ
// ============================================================================

struct NetworkPacket {
    std::vector<uint8_t> data;
};

struct SessionCryptoStorage {
    std::array<uint8_t, 32> chacha_key = {0};
    std::array<uint8_t, 8> current_salt = {0};
    std::array<uint8_t, 24> current_hash = {0};
    
    ~SessionCryptoStorage() {
        sodium_memzero(chacha_key.data(), chacha_key.size());
        sodium_memzero(current_salt.data(), current_salt.size());
        sodium_memzero(current_hash.data(), current_hash.size());
    }
};


class ThreadSafeQueue {
private:
    std::queue<NetworkPacket> raw_queue;
    std::mutex mtx;
    std::condition_variable cv;

public:
    void push(NetworkPacket pkt) {
        std::lock_guard<std::mutex> lock(mtx);
        raw_queue.push(std::move(pkt));
        cv.notify_one();
    }

    bool pop(NetworkPacket& out_pkt) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return !raw_queue.empty(); });
        
        out_pkt = std::move(raw_queue.front());
        raw_queue.pop();
        return true;
    }
};

#pragma GCC visibility pop
#endif /* stuff_ */
