#ifndef Renderer_h
#define Renderer_h

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Renderer {
public:
    void Initialize();
    void DrawFrame();
    void Destroy();
    
private:
    
};

#endif
