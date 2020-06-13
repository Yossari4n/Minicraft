#include "Renderer.h"

#include <SFML/System.hpp>
#include <SFML/Network.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

int main(int argc, const char * argv[]) {
    /*sf::TcpSocket socket;
    
    std::cout << "Connecting to socket...";
    if (socket.connect("192.168.8.119", 53000) != sf::Socket::Done) {
        std::cout << " ERROR!\n";
        return EXIT_FAILURE;
    }
    std::cout << " OK!\n";
    
    char data[] = "Hello world!\n";
    
    std::cout << "Sending message... ";
    if (socket.send(data, sizeof(data)) != sf::Socket::Done) {
        std::cout << " ERROR!\n";
        return EXIT_FAILURE;
    }
    std::cout << " OK!\n";*/
    
    Renderer renderer;
    auto window = renderer.Initialize();
    
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        renderer.DrawFrame();
    }
    
    renderer.Destroy();
    
    return EXIT_SUCCESS;
}
