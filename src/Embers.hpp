//
//  Embers.hpp
//  The campfire's visible flames and the sparks rising from it, as additive point sprites.
//  Each particle is one seed; its life cycle (birth, rise, drift, cooling) is computed in
//  shaders/embers.vert from the seed and the time.
//

#ifndef Embers_hpp
#define Embers_hpp

#include <GL/glew.h>
#include "Shader.hpp"

namespace gps {

    class Embers
    {
    public:
        void Init(int count);
        // Uniforms (fire position, wind, strength, matrices, time) are set by the caller.
        void Draw(gps::Shader& shader);

    private:
        GLuint vao = 0, vbo = 0;
        int count = 0;
    };
}

#endif /* Embers_hpp */
