//
//  Tornado.cpp
//

#include "Tornado.hpp"
#include "glm/glm.hpp"
#include <vector>
#include <cstdlib>

namespace gps {

    void Tornado::Init(int rings, int segments, int debris)
    {
        // Funnel: a grid of (u = around 0..1, v = up 0..1). The seam vertex is duplicated (u = 1)
        // so the noise pattern can wrap without a jump.
        std::vector<glm::vec2> grid;
        for (int r = 0; r <= rings; r++)
            for (int s = 0; s <= segments; s++)
                grid.push_back(glm::vec2(s / (float)segments, r / (float)rings));
        std::vector<GLuint> indices;
        for (int r = 0; r < rings; r++)
            for (int s = 0; s < segments; s++) {
                GLuint a = r * (segments + 1) + s, b = a + 1;
                GLuint c = a + (segments + 1), d = c + 1;
                indices.insert(indices.end(), { a, b, c, b, d, c });
            }
        funnelIndexCount = (GLsizei)indices.size();

        glGenVertexArrays(1, &funnelVAO);
        glGenBuffers(1, &funnelVBO);
        glGenBuffers(1, &funnelEBO);
        glBindVertexArray(funnelVAO);
        glBindBuffer(GL_ARRAY_BUFFER, funnelVBO);
        glBufferData(GL_ARRAY_BUFFER, grid.size() * sizeof(glm::vec2), grid.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, funnelEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (GLvoid*)0);
        glBindVertexArray(0);

        // Debris: one vec4 seed per particle (all 0..1).
        debrisCount = debris;
        std::vector<glm::vec4> seeds(debris);
        srand(2468);
        for (int i = 0; i < debris; i++)
            seeds[i] = glm::vec4(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX,
                                 rand() / (float)RAND_MAX, rand() / (float)RAND_MAX);
        glGenVertexArrays(1, &debrisVAO);
        glGenBuffers(1, &debrisVBO);
        glBindVertexArray(debrisVAO);
        glBindBuffer(GL_ARRAY_BUFFER, debrisVBO);
        glBufferData(GL_ARRAY_BUFFER, seeds.size() * sizeof(glm::vec4), seeds.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (GLvoid*)0);
        glBindVertexArray(0);
    }

    void Tornado::DrawFunnel(gps::Shader& shader)
    {
        shader.useShaderProgram();
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE); // both walls of the tube show, and nothing hides behind it in depth

        glBindVertexArray(funnelVAO);
        glDrawElements(GL_TRIANGLES, funnelIndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    void Tornado::DrawDebris(gps::Shader& shader)
    {
        shader.useShaderProgram();
        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        glBindVertexArray(debrisVAO);
        glDrawArrays(GL_POINTS, 0, debrisCount);
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glDisable(GL_PROGRAM_POINT_SIZE);
    }
}
