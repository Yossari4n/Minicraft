#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <SFML/System.hpp>
#include <SFML/Network.hpp>
#include <SFML/Window.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

int main(int argc, const char * argv[]) {
    sf::TcpSocket socket;
    
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
    std::cout << " OK!\n";
    
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::cout << extensionCount << " extensions supported\n";

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    
    return EXIT_SUCCESS;
}
