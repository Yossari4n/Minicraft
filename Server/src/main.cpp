#include <SFML/Network.hpp>

#include <iostream>

int main(int argc, const char * argv[]) {
    sf::TcpListener listener;

    std::cout << "Binding the listener to a port... ";
    if (listener.listen(53000) != sf::Socket::Done) {
        std::cout << " ERROR!\n";
        return EXIT_FAILURE;
    }
    std::cout << " OK!\n";
    
    sf::TcpSocket client;

    std::cout << "Accepting a new connection...";
    if (listener.accept(client) != sf::Socket::Done) {
        std::cout << " ERROR!\n";
        return EXIT_FAILURE;
    }
    std::cout << " OK!\n";
    
    char data[100];
    size_t received_size;
    
    std::cout << "Receiving message... ";
    if (client.receive(data, sizeof(data), received_size) != sf::Socket::Done) {
        std::cout << " ERROR!\n";
        return EXIT_FAILURE;
    }
    std::cout << " OK!\n";
    
    std::cout << "Message: " << data;
    
    return 0;
}
