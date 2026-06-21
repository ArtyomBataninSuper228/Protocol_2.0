#include <iostream>
#include <asio.hpp>
#include "stuff.hpp"
#include "vector"
#include <sodium.h>
#include <oqs/oqs.h>
using asio::ip::udp;
//🍺



};

class Server {
private:
    // ASIO - инициализируем ссылку в конструкторе, сокет пока просто привязываем к контексту
    asio::io_context& io_context_;
    asio::ip::udp::socket socket_;
    
    // Network
    asio::ip::udp::endpoint addr;
    int port_ = 0;
    
    // Security
    std::array<uint8_t, crypto_aead_xchacha20poly1305_ietf_KEYBYTES> psk_key_;
    std::vector<uint8_t> crypto_data;
    std::vector<uint8_t> cert_public_key_;
    std::vector<uint8_t> cert_private_key_;
    
    // Logic (твои флаги состояний)
    // 0 - initialising, 1 - checking parameters, 2 - starting up listener, 3 - starting up worker, 4 - online, -1 - critical error
    short state = 0;
    // 0 - internet mode without PSK, 1 - local mode, 2 - internet mode with PSK
    short mode = 0;
    
    // Logs
    Logger lgr = Logger();
    

public:
    Server(asio::io_context& io_context)
        : io_context_(io_context),
          socket_(io_context)
    {
        state = 0; // Initialising
        if (sodium_init() < 0) {
            lgr.log(3, "Constructor", "Libsodium init failed!");
            state = -1;
        }
    }


    void set_mode(short target_mode) {
        if (state == 0) {
            mode = target_mode;
        }
    }

    void set_port(int port) {
        if (state == 0) {
            port_ = port;
        }
    }

    void set_psk(const std::array<uint8_t, crypto_aead_xchacha20poly1305_ietf_KEYBYTES>& key) {
        if (state == 0) {
            psk_key_ = key;
        }
    }

    // Загрузка готового постквантового сертификата
    void set_pq_certificate(const std::vector<uint8_t>& public_key, const std::vector<uint8_t>& private_key) {
        if (state == 0) {
            cert_public_key_ = public_key;
            cert_private_key_ = private_key;
        }
    }

    // Или метод автоматической генерации ключей, если сертификата нет
    void generate_pq_certificate() {
        OQS_SIG* sig = OQS_SIG_new(OQS_SIG_alg_ml_dsa_65);
        if (!sig) {
            state = -1;
            return;
        }
        cert_public_key_.resize(sig->length_public_key);
        cert_private_key_.resize(sig->length_secret_key);
        
        OQS_SIG_keypair(sig, cert_public_key_.data(), cert_private_key_.data());
        OQS_SIG_free(sig);
    }



    bool run() {
        if (state == -1) return false;

        // Фаза 1: Проверка параметров
        state = 1;
        if (port_ <= 0 || port_ > 65535) {
            add_log(3, "Validation", "Invalid port number");
            state = -1;
            return false;
        }
        if (mode == 2 && psk_key_[0] == 0) { // Пример проверки PSK для режима 2
            add_log(2, "Validation", "Warning: PSK key might be empty in mode 2");
        }

        // Фаза 2: Открытие сокета и бинд порта
        state = 2;
        try {
            // Вот теперь, когда мы знаем порт, мы открываем сокет в ОС!
            socket_.open(asio::ip::udp::v4());
            socket_.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), port_));
            add_log(0, "Network", "Socket bound successfully to port " + std::to_string(port_));
        } catch (const std::system_error& e) {
            add_log(3, "Network", std::string("Bind error: ") + e.what());
            state = -1;
            return false;
        }

        // Фаза 3 и 4: Запуск воркеров и выход в онлайн
        state = 3;
        start_async_receive();
        
        state = 4; // Online!
        add_log(0, "Lifecycle", "Server is now ONLINE");
        return true;
    }

private:
    void start_async_receive() {
        // Тут будет твой async_receive_from
    }

};



int main() {
    try {
        asio::io_context io_context;

        // Создаем сокет на порту 8080
        
        udp::socket socket(io_context, udp::endpoint(udp::v4(), 8080));
        std::cout << "Server started on port 8080..." << std::endl;
        uint32_t hash;
        size_t len;
        unsigned char bytes[4];
        while (true) {
            unsigned char buffer[1500];
            udp::endpoint remote_endpoint;

            // Читаем данные
            len = socket.receive_from(asio::buffer(buffer), remote_endpoint);
            auto t_st = std::chrono::high_resolution_clock::now();
            buffer[len] = '\0';
            //sendto(sockfd, (const char *)hello, strlen(hello), MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);
            hash = crc32(buffer, 4, len);
            
            // Big-Endian
            bytes[3] = (hash >> 0) & 0xFF;
            bytes[2] = (hash >> 8) & 0xFF;
            bytes[1] = (hash >> 16) & 0xFF;
            bytes[0] = (hash >> 24) & 0xFF;
            
            if (not  ((bytes[0] == buffer[0]) and (bytes[1] == buffer[1]) and (bytes[2] == buffer[2]) and (bytes[3] == buffer[3]))){
                std::cout<<"Unsuccesfull hash"<<std::endl;
                continue;
            }
            auto t_end = std::chrono::high_resolution_clock::now();
            
            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t_end-t_st).count();
            std::cout<<ns<<std::endl;
            // socket.send_to(asio::buffer("OK"), remote_endpoint);
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
