//
//  Fireflies.hpp
//  Glowing points that drift around the camp at night and blink like real fireflies. Positions
//  and blinking are computed in the vertex shader from a per-firefly seed and the time.
//

#ifndef Fireflies_hpp
#define Fireflies_hpp

#include <GL/glew.h>
#include "glm/glm.hpp"
#include "Shader.hpp"

namespace gps {

    class Fireflies
    {
    public:
        // count fireflies spread over the box [areaMin, areaMax] (world space).
        void Init(int count, const glm::vec3& areaMin, const glm::vec3& areaMax);

        // visibility (0..1) fades them all in/out; other uniforms are set by the caller.
        void Draw(gps::Shader& shader, float visibility);

        glm::vec3 AreaMin() const { return areaMin; }
        glm::vec3 AreaSize() const { return areaMax - areaMin; }

    private:
        GLuint vao = 0, vbo = 0;
        int count = 0;
        glm::vec3 areaMin, areaMax;
    };
}

#endif /* Fireflies_hpp */
