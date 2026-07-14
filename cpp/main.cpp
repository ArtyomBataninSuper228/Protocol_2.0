#include <iostream>
#include <asio.hpp>
#include "stuff.hpp"
#include "vector"
#include <sodium.h>
#include <oqs/oqs.h>
#include "server.hpp"
#ifdef _WIN32
#include <windows.h>
#endif

using asio::ip::udp;
/*
🍺
🍺🍺
🍺🍺🍺🍺
🍺🍺🍺🍺🍺🍺🍺🍺
🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺
🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺
🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺🍺
*/





void stop_autosave(Logger &lgr, int time, asio::steady_timer &timer){
    timer.expires_after(std::chrono::seconds(time));
    timer.async_wait([&lgr](const std::error_code& ec) mutable {
        if (!ec) {
            
            std::cout<<"Stopping!"<<std::endl;
            lgr.stop_autosave();
        }
        else{
            std::cerr<<"Error "<<ec.message()<<std::endl;
        }
        });
}




int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    try {
        asio::io_context io_context;
		using myserver = Server<ChaCha20_Poly1305_Encoder, OqsEncoder<OQS_Level5>, UniversalHasher<Trait_BLAKE2b>, Connection, Connection, Connection>;
        Server s = myserver(io_context);
        s.set_mode(2);
		s.psk_encoder.generate_key();
        s.set_port(8080);
        s.lgr.set_printlevel(0);
        s.lgr.set_loglevel(0);
        s.lgr.start_autosave(1);
        asio::steady_timer timer = asio::steady_timer(io_context);
        stop_autosave(s.lgr, 10, timer);
        if (s.run()) {
                    std::cout << "Запускаем цикл Asio..." << std::endl;
                    io_context.run();
                }
        /*
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
         */
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
         
    return 0;
}
