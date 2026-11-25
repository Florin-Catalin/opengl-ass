//
//  Rain.hpp
//  Rain streaks drawn as instanced GL_LINES. Every drop's position is computed on the GPU from a
//  random seed and the time, wrapped inside a box that follows the camera, so there is no
//  per-frame CPU work and drops never visibly pop in or out.
//

#ifndef Rain_hpp
#define Rain_hpp

#include <GL/glew.h>
#include "glm/glm.hpp"
#include "Shader.hpp"

namespace gps {

    class Rain
    {
    public:
        // Creates the seed buffer for up to maxDrops drops. Call once, after GL is initialised.
        void Init(int maxDrops);

        // amount (0..1) scales how many drops are drawn. Uniforms other than the drop count
        // (matrices, camera, time, colour) are set by the caller on the bound shader.
        void Draw(gps::Shader& shader, float amount);

    private:
        GLuint vao = 0;
        GLuint seedVBO = 0;
        int maxDrops = 0;
    };
}

#endif /* Rain_hpp */
