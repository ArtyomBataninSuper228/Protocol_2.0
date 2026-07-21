//
//  client.hpp
//  MyProtocolCore
//
//  Created by Артем Батанин on 17.07.2026.
//

#ifndef client_
#define client_
#include "stuff.hpp"
#include "encoders.hpp"
#include "asymetric_encoders.hpp"
#include "hashes.hpp"
#pragma once

//˔˔˔˔˔˔˔˔˔˔˔˔˔˔˔˔˔˔˔˔˔˔˔˔˔˔

template <typename SymmetricEncoder, typename AsymetricEncoder, typename Hasher>
class Client{
    std::atomic<short> state = 0;
    std::atomic<bool> is_running = false;
    int mode = 0;
    int port_;
    asio::ip::udp::endpoint server_addr_;
    asio::io_context& io_context_;
    asio::ip::udp::socket socket_;
    using p_buffer = Karusel_Buffer<10, 2000>;
    std::unique_ptr<p_buffer> packet_queue = std::make_unique<p_buffer>();
    asio::steady_timer wait_timer;
    
    //reciever data
    std::array<uint8_t, 1500> buffer;
    asio::ip::udp::endpoint remote_endpoint;
    uint16_t len;
    std::thread sender;
    std::thread reciever;
    
public:
    SymmetricEncoder s_coder = SymmetricEncoder();
    AsymetricEncoder a_coder = AsymetricEncoder();
    Hasher hasher = Hasher();
    Logger lgr = Logger(io_context_);
    
    Client(asio::io_context& io_context, int port, std::string addr):io_context_(io_context),
    socket_(io_context), port_(port), server_addr_(asio::ip::make_address(addr), port_), wait_timer(io_context)
    {
        
        
        state = 0; // Initialising
        if (sodium_init() < 0) {
            lgr.log(3, "Constructor", "Libsodium init failed!");
            state = -1;
        }
        
    }
    
    
    
    bool set_mode(int new_mode){
        if(state.load() > 0){
            return false;
        }
        this->mode = new_mode;
        return true;
    }
    
    bool connect(){
        
        packet_queue->is_run = true;
        try {
            socket_.open(asio::ip::udp::v4());
        } catch (const std::system_error& e) {
            lgr.log(3, "First check: Network", std::string("Socket bind error: ") + e.what());
            state = -1;
            return false;
        }
        is_running = true;
        
        sender = std::thread([this]() {
            this->start_send();
            });
         
        reciever = std::thread([this]() {
            this->start_receive();
            });
        switch(mode){
            case 0:{
                std::vector<uint8_t> data;
                for(uint16_t i = 0; i < 1400; i++){
                    data.push_back(i%256);
                }
                auto now = std::chrono::steady_clock::now().time_since_epoch();
                ns nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(now);
                for(int64_t i = 0; i <40000; i++){
                    send_packet(data, nanoseconds+ns(200000*i));
                }
                
                break;
            }
            case 1:{
                a_coder.generate_keypair();
                break;
            }
            case 2:{
                
                break;
            }
                
        }
        
        return true;
    }
    void send_packet(std::vector<uint8_t> data, ns timestamp = ns(0)){
        //NetworkPacket packet = NetworkPacket();
        
        switch(mode){
            case 0:{
                auto data_hash = hasher.hash_single(data);
                data.insert(data.end(), data_hash.begin(), data_hash.end());
                while(not packet_queue->push(convert_vector_to_array(data, server_addr_), timestamp)){
                    wait_timer.expires_after(SPIN_THRESHOLD);
                    wait_timer.wait();
                }
                
                break;
            }
            case 1:{
                
                break;
            }
            case 2:{
                
                break;
            }
                
        }
    }
    void start_send(){
        while (is_running){
            if(not packet_queue->send(socket_)){
                break;
            }
        }
    }
    
    void start_receive() {
        try {
            lgr.log(0, "Reciever", "Starting receive loop");
            while (is_running) {
                /*
                // co_await «замораживает» корутину, пока не придут данные. Поток при этом не блокируется!
                len = socket.receive_from(
                    asio::buffer(buffer),
                    remote_endpoint,
                    asio::use_awaitable // Указываем standalone Asio использовать корутины
                );
                */
                len = this->socket_.receive_from(asio::buffer(buffer), remote_endpoint);
                if (len < 0) {
                    lgr.log(3, "Reciever", "Socket is closed");
                    break;
                }
                switch (mode)
                {
                case 0:
                    {
                        if (len < 2*hasher.HASH_SIZE or len > 1500) {
                            //lgr.log(0, "Reciever", "Received data is too short to contain a hash. Length: " + std::to_string(len));
                            break;
                        }
                        auto data_hash = hasher.hash_single(buffer.data(), len - hasher.HASH_SIZE);
                        if (sodium_memcmp(data_hash.data(), buffer.data() + (len - Hasher::HASH_SIZE), Hasher::HASH_SIZE) != 0) {
                            //lgr.log(3, "Reciever", "Hash mismatch! Data integrity check failed.");
                            break;
                        }
                        

                        break;
                    }
                case 1:
                    lgr.log(0, "Reciever", "Received data from " + remote_endpoint.address().to_string() + ":" + std::to_string(remote_endpoint.port()) + ", length: " + std::to_string(len) + std::to_string(Hasher::HASH_SIZE));
                    // Здесь можно добавить обработку данных в режиме 1
                    break;
                case 2:
                    lgr.log(0, "Reciever", "Received data from " + remote_endpoint.address().to_string() + ":" + std::to_string(remote_endpoint.port()) + ", length: " + std::to_string(len));
                    // Здесь можно добавить обработку данных в режиме 2
                    break;
                default:
                    break;
                }

            }
        }
            catch (const std::system_error& e) {
                // В standalone Asio все ошибки сети летят как стандартные std::system_error
                lgr.log(3, "Reciever",  e.what());
            }
        }
    void close(){
        packet_queue->stop();
        is_running = false;
        socket_.close();
    }
    
    
    
};

#pragma GCC visibility push(default)










#pragma GCC visibility pop
#endif
