//
//  Rain.cpp
//

#include "Rain.hpp"
#include <vector>
#include <cstdlib>

namespace gps {

    void Rain::Init(int count)
    {
        maxDrops = count;

        // One vec4 per drop: xyz = position in the box (0..1), w = speed/brightness variation.
        std::vector<glm::vec4> seeds(count);
        srand(1234);
        for (int i = 0; i < count; i++)
            seeds[i] = glm::vec4(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX,
                                 rand() / (float)RAND_MAX, rand() / (float)RAND_MAX);

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &seedVBO);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, seedVBO);
        glBufferData(GL_ARRAY_BUFFER, seeds.size() * sizeof(glm::vec4), seeds.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (GLvoid*)0);
        glVertexAttribDivisor(0, 1); // one seed per drop (instance); the 2 line ends use gl_VertexID
        glBindVertexArray(0);
    }

    void Rain::Draw(gps::Shader& shader, float amount)
    {
        int drops = (int)(maxDrops * glm::clamp(amount, 0.0f, 1.0f));
        if (drops <= 0)
            return;

        shader.useShaderProgram();
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE); // depth-tested against the scene, but drops don't hide each other

        glBindVertexArray(vao);
        glDrawArraysInstanced(GL_LINES, 0, 2, drops);
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
}
