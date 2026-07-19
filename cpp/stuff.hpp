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
#include <asio.hpp>
#include <thread>

// Для интринсиков x86/x64
#if defined(_MSC_VER) || defined(__i386__) || defined(__x86_64__)
    #include <immintrin.h>
    #define HARDWARE_PAUSE() _mm_pause()
#elif defined(__aarch64__) || defined(__arm__)
    // Для ARM (например, Raspberry Pi или Apple Silicon)
    #define HARDWARE_PAUSE() asm volatile("yield" ::: "memory") // ARM 'yield' instruction, not OS yield!
#else
    #define HARDWARE_PAUSE() do {} while(0)
#endif

using namespace std::chrono_literals;

#if defined(_WIN32)
    // В Windows стандартный квант времени/шаг таймера составляет 15.625 мс
    constexpr std::chrono::nanoseconds SPIN_THRESHOLD = 15625000ns;

#elif defined(__linux__)
    // В Linux базовое минимальное квантование (min granularity) обычно около 3 мс
    constexpr std::chrono::nanoseconds SPIN_THRESHOLD = 3000000ns;

#elif defined(__APPLE__)
    // В macOS/iOS стандартный шаг планировщика чаще всего равен 10 мс
    constexpr std::chrono::nanoseconds SPIN_THRESHOLD = 10000000ns;

#else
    // Значение по умолчанию для остальных ОС (10 мс)
    constexpr std::chrono::nanoseconds SPIN_THRESHOLD = 10000000ns;
#endif




#pragma once

#pragma GCC visibility push(default)
using ns = std::chrono::nanoseconds;

inline void precise_pacing_sleep(std::chrono::nanoseconds duration) {
    auto start = std::chrono::high_resolution_clock::now();
    auto target = start + duration;

    // 1. Грубый сон через ОС (ТОЛЬКО если ждать реально долго, например > 2 мс)
    // В активном потоке отправки этот блок может вообще никогда не срабатывать
    auto bulk_time = duration - std::chrono::milliseconds(2);
    if (bulk_time.count() > 0) {
        std::this_thread::sleep_for(bulk_time);
    }

    // 2. Сверхточный Spin-wait с аппаратной паузой.
    // ОС не вмешивается, квант времени не отдается!
    while (std::chrono::high_resolution_clock::now() < target) {
        HARDWARE_PAUSE();
    }
}

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



struct Crc32Hasher {
    template <std::size_t N>
    uint32_t operator()(const std::array<uint8_t, N>& p) const {
        return crc32(p);
    }
};



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
    std::string global_preset;
    
    std::string to_json() const {
            nlohmann::json j;
            j["id"] = id;
            j["level"] = level;
            j["timestamp_ns"] = timestamp_ns;
            j["time"] = time_str;
            j["place"] = place;
            j["text"] = text;// Если тут будут кавычки или \n, библиотека сама их заэкранирует!
        j["global_preset"] = global_preset;

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
    
    std::atomic<bool> is_saving_{false};

public:
    std::string global_preset = "";
    
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
            
            std::cout << "{" << ctime_raw << "} " << "[" <<global_preset<< ": " << place << "] " <<  "|" << level_str << "| " << text << std::endl;
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
                this->log(0, "Logger",  std::string("Логи успешно сохранены в файл: ") + target_file);

            } else {
                std::cerr << "[Logger] КРИТИЧЕСКАЯ ОШИБКА: Не удалось открыть файл для записи логов!" << std::endl;
            }
        }
    void start_autosave(int interval_seconds = 60, const std::string& filename = "server_logs.json") {
        is_saving_ = true;
        timer_.expires_after(std::chrono::seconds(interval_seconds));
        
        timer_.async_wait([this, interval_seconds, filename](const std::error_code& ec) mutable {
            if (!is_saving_) {
                
                std::cout << "[Logger] Асинхронное автосохранение успешно остановлено." << std::endl;
                return;
            }
            if (ec){
                log(4, "Logger autosaver", ec.message());
                std::cerr<<"[Logger] " <<ec.message()<<std::endl;
                return;
            }

            save_to_file(filename);
            start_autosave(interval_seconds, filename);
        });
    }
    void set_loglevel(int level){
        loglevel_ = level;
    }
    void set_printlevel(int level){
        printlevel = level;
    }
    void stop_autosave() {
        is_saving_ = false;
        timer_.cancel();
        }
};

// ============================================================================
// БЛОК СТРУКТУР И КЛАССОВ ДЛЯ СЕТЕВОГО СОЕДИНЕНИЯ
// ============================================================================



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

struct NetworkPacket {
    std::vector<uint8_t> data;
    std::chrono::nanoseconds timestamp;
    bool operator<(const NetworkPacket& other) const {
            // Меняем логику: текущий пакет "меньше" (хуже),
            // если его время БОЛЬШЕ времени другого пакета, что бы priority_queue первым отдавала элемент с наименьшим временем отправки.
            return timestamp > other.timestamp;
        }
};


class ThreadSafeQueue {
private:
    std::priority_queue<NetworkPacket> raw_queue;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<std::chrono::nanoseconds::rep> next_target{
            std::chrono::nanoseconds::max().count()
        };
    std::atomic<bool> stopped{false};

    public:
    void stop() {
            stopped.store(true, std::memory_order_release);
            cv.notify_all(); // будим всех, кто спит в cv.wait / wait_until
        }
        void push(NetworkPacket pkt) {
            auto t = pkt.timestamp.count();
            {
                std::lock_guard<std::mutex> lock(mtx);
                raw_queue.push(std::move(pkt));
            }
            auto cur = next_target.load(std::memory_order_relaxed);
            while (t < cur && !next_target.compare_exchange_weak(
                       cur, t, std::memory_order_relaxed)) {}
            cv.notify_one();
        }
    
    bool pop_batch(std::vector<NetworkPacket>& out_batch) {
        std::unique_lock<std::mutex> lock(mtx);

        // 1. Ждем, пока очередь пуста или не пришел сигнал остановки
        cv.wait(lock, [this] {
            return !raw_queue.empty() || stopped.load(std::memory_order_acquire);
        });

        if (stopped.load(std::memory_order_acquire)) {
            return false;
        }

        auto now = std::chrono::steady_clock::now().time_since_epoch();
        auto target = raw_queue.top().timestamp;

        // 2. Если время первого пакета еще не пришло - засыпаем через ОС
        if (target > now) {
            // Просыпаемся либо по таймеру, либо если кто-то добавит более срочный пакет
            cv.wait_until(lock, std::chrono::steady_clock::time_point(target));
            // После пробуждения просто возвращаем пустой батч,
            // цикл отправителя вызовет метод заново
            return true;
        }

        // 3. Собираем пачку: забираем ВСЕ пакеты, чье время (target) <= now
        while (!raw_queue.empty()) {
            auto pkt_time = raw_queue.top().timestamp;
            
            // Если следующий пакет отправлять еще рано — останавливаем сбор пачки
            if (pkt_time > std::chrono::steady_clock::now().time_since_epoch()) {
                break;
            }

            out_batch.push_back(std::move(const_cast<NetworkPacket&>(raw_queue.top())));
            raw_queue.pop();
        }

        return true;
    }

    bool pop(NetworkPacket& out_pkt) {
            std::unique_lock<std::mutex> lock(mtx, std::defer_lock);

            while (true) {
                lock.lock();

                if (stopped.load(std::memory_order_acquire)) {
                    return false; // сигнал остановки — выходим сразу
                }

                if (raw_queue.empty()) {
                    cv.wait(lock, [this] {
                        return !raw_queue.empty() || stopped.load(std::memory_order_acquire);
                    });
                    if (stopped.load(std::memory_order_acquire)) {
                        return false;
                    }
                }

                auto now = std::chrono::steady_clock::now().time_since_epoch();
                auto target = raw_queue.top().timestamp;

                if (target <= now) {
                    out_pkt = std::move(const_cast<NetworkPacket&>(raw_queue.top()));
                    raw_queue.pop();
                    return true;
                }

                auto remaining = target - now;

                if (remaining > SPIN_THRESHOLD) {
                    cv.wait_until(lock, std::chrono::steady_clock::now() + (remaining - SPIN_THRESHOLD));
                    lock.unlock();
                    continue; // проверит stopped в начале следующей итерации
                }

                lock.unlock();
                while (true) {
                    if (stopped.load(std::memory_order_acquire)) {
                        return false; // выход из spin-фазы по флагу
                    }
                    now = std::chrono::steady_clock::now().time_since_epoch();
                    if (now >= target) break;
                    HARDWARE_PAUSE();
                }
            }
        }
            
};

class Zero_Class{
    
};

#pragma GCC visibility pop
#endif /* stuff_ */
