//
//  Embers.cpp
//

#include "Embers.hpp"
#include "glm/glm.hpp"
#include <vector>
#include <cstdlib>

namespace gps {

    void Embers::Init(int n)
    {
        count = n;
        // One vec4 per particle, all 0..1: x = lifetime, y = phase, z = identity, w = kind.
        std::vector<glm::vec4> seeds(n);
        srand(1357);
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

    void Embers::Draw(gps::Shader& shader)
    {
        shader.useShaderProgram();
        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE); // glowing: light adds up
        glDepthMask(GL_FALSE);

        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, count);
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glDisable(GL_PROGRAM_POINT_SIZE);
    }
}
