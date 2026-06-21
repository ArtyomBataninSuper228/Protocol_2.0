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




/* The classes below are exported */
#pragma GCC visibility push(default)


class ServerConnection{
public:
    
    
private:
    uint32_t id = 0;
    SessionCryptoStorage crypto;
    ThreadSafeQueue packet_queue;
    
};


class Server{
public:
    bool is_local = false;
    uint32_t port = 8000;
    udp::endpoint address = udp::v4();
    udp::socket socket;
    std::unordered_map<uint64_t, ServerConnection> connections;
    std::mutex connections_mtx;

    void start_server(){
        try {
            asio::io_context io_context;

            // Создаем сокет на порту 8080
            
            udp::socket socket(io_context, udp::endpoint(this.endpoint, this.port));
            std::cout << "Server started on port " << this.port << "...." << std::endl;
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
    }

private:
    bool use_pre_encoding = false;
    
    
    
    
};








#pragma GCC visibility pop
#endif

