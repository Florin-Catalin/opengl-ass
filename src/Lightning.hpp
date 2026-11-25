//
//  Lightning.hpp
//  Lightning strikes during rain: a jagged, branching bolt drawn as camera-facing glowing
//  ribbons, plus a flicker envelope that the scene uses to flash the sky, clouds and ambient.
//

#ifndef Lightning_hpp
#define Lightning_hpp

#include <GL/glew.h>
#include "glm/glm.hpp"
#include "Shader.hpp"
#include <vector>

namespace gps {

    class Lightning
    {
    public:
        void Init();

        // Starts a strike at a random spot in front of the camera, some distance away.
        void Strike(const glm::vec3& cameraPos, const glm::vec3& cameraForward, double now);

        // Flash strength (0 = nothing, ~1 = brightest flicker) at time now.
        float Flash(double now) const;

        // Draws the bolt with additive blending; call after the sky, before transparent rain.
        void Draw(gps::Shader& shader, const glm::mat4& view, const glm::mat4& projection,
                  const glm::vec3& cameraPos, double now);

    private:
        struct Branch
        {
            std::vector<glm::vec3> points;
            float intensity; // main channel 1, side branches dimmer
        };

        std::vector<Branch> branches;
        double strikeTime = -100.0;
        GLuint vao = 0, vbo = 0;

        void BuildChannel(const glm::vec3& from, const glm::vec3& to, int depth, float jitter,
                          std::vector<glm::vec3>& out);
    };
}

#endif /* Lightning_hpp */
