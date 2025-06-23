#include <iostream>
#include <string>
#include <thread>
#include <boost/asio.hpp>
#include "platform.hpp"

using boost::asio::ip::tcp;

int main() {
    std::cout << "ParellelStone Game Server v1.0.0" << std::endl;
    std::cout << "Platform: " << Platform::GetPlatformName() << std::endl;
    std::cout << "C++20 Development Environment with Boost.Asio Ready!" << std::endl;
    
    std::cout << "Thread count: " << std::thread::hardware_concurrency() << std::endl;
    
    try {
        boost::asio::io_context io_context;
        
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 8080));
        std::cout << "Server listening on port 8080..." << std::endl;
        
        // Simple echo server example
        while (true) {
            tcp::socket socket(io_context);
            acceptor.accept(socket);
            
            std::cout << "Client connected from: " 
                      << socket.remote_endpoint().address().to_string() 
                      << ":" << socket.remote_endpoint().port() << std::endl;
            
            // Simple response
            std::string response = "Hello from ParellelStone Game Server!\n";
            boost::asio::write(socket, boost::asio::buffer(response));
            
            socket.close();
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    
    return 0;
}