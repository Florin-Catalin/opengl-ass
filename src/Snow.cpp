//
//  Snow.cpp
//

#include "Snow.hpp"
#include "glm/glm.hpp"
#include <vector>
#include <cstdlib>

namespace gps {

    void Snow::Init(int n)
    {
        maxFlakes = n;

        // One vec4 per flake: xyz = spot in the box (0..1), w = size/speed variation (0..1).
        std::vector<glm::vec4> seeds(n);
        srand(9876);
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

    void Snow::Draw(gps::Shader& shader, float amount)
    {
        int flakes = (int)(maxFlakes * glm::clamp(amount, 0.0f, 1.0f));
        if (flakes <= 0)
            return;

        shader.useShaderProgram();
        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE); // hidden behind the scene, but flakes don't hide each other

        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, flakes);
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glDisable(GL_PROGRAM_POINT_SIZE);
    }
}
