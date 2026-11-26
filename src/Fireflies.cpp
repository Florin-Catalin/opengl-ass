//
//  Fireflies.cpp
//

#include "Fireflies.hpp"
#include <vector>
#include <cstdlib>

namespace gps {

    void Fireflies::Init(int n, const glm::vec3& lo, const glm::vec3& hi)
    {
        count = n;
        areaMin = lo;
        areaMax = hi;

        // One vec4 per firefly: xyz = home position in the area (0..1), w = personality (0..1).
        std::vector<glm::vec4> seeds(n);
        srand(4321);
        for (int i = 0; i < n; i++)
            seeds[i] = glm::vec4(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX,
                                 rand() / (float)RAND_MAX, rand() / (float)RAND_MAX);

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, seeds.size() * sizeof(glm::vec4), seeds.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (GLvoid*)0);
        glBindVertexArray(0);
    }

    void Fireflies::Draw(gps::Shader& shader, float visibility)
    {
        if (visibility <= 0.01f || count == 0)
            return;

        shader.useShaderProgram();
        glUniform1f(gps::uniformLocation(shader.shaderProgram, "visibility"), visibility);

        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE); // glows add up
        glDepthMask(GL_FALSE);

        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, count);
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glDisable(GL_PROGRAM_POINT_SIZE);
    }
}
