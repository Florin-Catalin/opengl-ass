//
//  Snow.hpp
//  Falling snowflakes as point sprites. Each flake is one seed; its fall, flutter and the
//  wrapping box around the camera are computed in shaders/snow.vert.
//

#ifndef Snow_hpp
#define Snow_hpp

#include <GL/glew.h>
#include "Shader.hpp"

namespace gps {

    class Snow
    {
    public:
        // Creates the seed buffer for up to maxFlakes flakes. Call once, after GL is initialised.
        void Init(int maxFlakes);

        // amount (0..1) scales how many flakes are drawn. Other uniforms are set by the caller.
        void Draw(gps::Shader& shader, float amount);

    private:
        GLuint vao = 0, vbo = 0;
        int maxFlakes = 0;
    };
}

#endif /* Snow_hpp */
