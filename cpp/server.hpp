//
//  server.hpp
//  MyProtocolCore
//
//  Created by Артем Батанин on 19.05.2026.
//

#ifndef server_
#define server_

#include <iostream>
#include <asio.hpp>
#include "stuff.hpp"
#include "encoders.hpp"
#include "asymetric_encoders.hpp"
#include "hashes.hpp"

/* The classes below are exported */
#pragma GCC visibility push(default)




class Connection{
    
    
};

template<typename SymmetricCoder, typename AsymmetricCoder, typename Hasher, typename Connection, typename Session, typename Session_Handler>
class Server {
private:
    
    // ASIO - инициализируем ссылку в конструкторе, сокет пока просто привязываем к контексту
    asio::io_context& io_context_;
    asio::ip::udp::socket socket_;
    
    // Network
    asio::ip::udp::endpoint addr;
    int port_ = 0;
    std::array<uint8_t, 1500> buffer;
    asio::ip::udp::endpoint remote_endpoint;
    uint16_t len;
    
    // Security
    std::array<uint8_t, crypto_aead_xchacha20poly1305_ietf_KEYBYTES> psk_key_;
    std::vector<uint8_t> crypto_data{};
    std::vector<uint8_t> cert_public_key_{};
    std::vector<uint8_t> cert_private_key_{};
	
    // Logic
    // 0 - initialising, 1 - checking parameters, 2 - starting up listener, 3 - starting up worker, 4 - online, -1 - critical error
    short state = 0;
    std::atomic<bool> is_running = false;
    // 0 - local mode, 1 - internet mode without PSK, 2 - internet mode with PSK
    short mode = 0;
public:
    // Logs
    Logger lgr = Logger(io_context_);
    SymmetricCoder psk_encoder = SymmetricCoder();
	AsymmetricCoder asym_encoder = AsymmetricCoder();
	AsymmetricCoder cert_encoder = AsymmetricCoder();

    

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
        else {
            lgr.log(3, "SET_MODE", "Mode cannot be changed after initialization");
        }
    }

    void set_port(int port) {
        if (state == 0) {
            port_ = port;
        }
    }

    void set_psk(const std::array<uint8_t, SymmetricCoder::KEY_BYTES>& key) {
        if (state == 0) {
            if (psk_encoder.set_key(key)) {
				lgr.log(0, "SET_PSK", "PSK set successfully");
            }
            else {
				lgr.log(3, "SET_PSK", "Failed to set PSK");
            }
        }
        else {
			lgr.log(3, "SET_PSK", "PSK cannot be changed after initialization");
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
            lgr.log(4, "PQ Certificate generator", "Not sig");
            return;
        }
        cert_public_key_.resize(sig->length_public_key);
        cert_private_key_.resize(sig->length_secret_key);
        
        OQS_SIG_keypair(sig, cert_public_key_.data(), cert_private_key_.data());
        OQS_SIG_free(sig);
    }



    bool run() {
        if (state == -1) return false;

        // FURST CHECK
        // Фаза 1: Проверка параметров
        state = 1;
        if (port_ <= 0 || port_ > 65535) {
            lgr.log(3, "First check: Validation", "Invalid port number");
            state = -1;
            return false;
        }
        
        if (mode == 2){
            if (psk_encoder.is_key_set_func() == 0) {
                lgr.log(2, "First check: Validation", "Warning: PSK key might be empty in mode 2");
                return false;
            }

        }
        
        // Фаза 2: Открытие сокета и бинд порта
        state = 2;
        try {
            socket_.open(asio::ip::udp::v4());
            socket_.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), port_));
            lgr.log(0, "First check: Network", "Socket bound successfully to port " + std::to_string(port_));
        } catch (const std::system_error& e) {
            lgr.log(3, "First check: Network", std::string("Socket bind error: ") + e.what());
            state = -1;
            return false;
        }
        
    
        // Фаза 3 и 4: Запуск воркеров и выход в онлайн
        state = 3;
        asio::co_spawn(
            socket_.get_executor(),
            start_receive(), // вызываем функцию, передавая её результат в spawn
            asio::detached
        );
        
        state = 4; // Online!
        lgr.log(0, "Lifecycle", "Server is now ONLINE 228");
        return true;
    }
    void stop_server(){
        lgr.stop_autosave();
        is_running = false;
        state = 1001;
    }

private:
    asio::awaitable<void> start_receive() {
        try {
            // Твой асинхронный цикл while
            lgr.log(0, "Reciever", "Starting receive loop");
            while (mode > 0) {
                // co_await «замораживает» корутину, пока не придут данные. Поток при этом не блокируется!
                len = co_await socket_.async_receive_from(
                    asio::buffer(buffer),
                    remote_endpoint,
                    asio::use_awaitable // Указываем standalone Asio использовать корутины
                );

                // Ошибки нет, работаем с данными напрямую
                //lgr.log(0, "reciever",   "DATa");
            }
        }
        catch (const std::system_error& e) {
            // В standalone Asio все ошибки сети летят как стандартные std::system_error
            std::cerr << "Сеть отвалилась или сокет закрыт: " << e.what() << "\n";
        }
    }

};










#pragma GCC visibility pop
#endif

