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

void run_context(asio::io_context& io_context) {
	std::cout << "Запускаем цикл Asio..." << std::endl;
	io_context.run();
}


int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    try {
        asio::io_context io_context;
		using myserver = Server<XChaCha20Poly1305Coder, OqsEncoder<OQS_Level5>, UniversalHasher<Trait_BLAKE2b>, Connection, Connection, Connection>;
        Server s = myserver(io_context);
        s.set_mode(0);
		s.psk_encoder.generate_key();
        s.set_port(8080);
        s.lgr.set_printlevel(0);
        s.lgr.set_loglevel(1);
        s.lgr.set_printlevel(1);
        s.lgr.start_autosave(1);
        asio::steady_timer timer = asio::steady_timer(io_context);
        //stop_autosave(s.lgr, 30, timer);
		s.run();
		std::thread t = std::thread(run_context, std::ref(io_context));
		t.detach();
        while (1) {
            std::cout<< "Pacets: " << s.num << std::endl;
            sleep(2);
        }
        sleep(30000);
		s.stop_server();
        
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
         
    return 0;
}
