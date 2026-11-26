//
//  Lightning.cpp
//

#include "Lightning.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <cmath>
#include <cstdlib>

namespace gps {

    static float rnd(float lo, float hi)
    {
        return lo + (hi - lo) * (rand() / (float)RAND_MAX);
    }

    // A strike flickers: a main flash and a couple of return strokes, each decaying fast.
    static const int PULSES = 3;
    static const float PULSE_TIME[PULSES] = { 0.0f, 0.09f, 0.24f };
    static const float PULSE_AMPLITUDE[PULSES] = { 1.0f, 0.55f, 0.8f };
    static const float STRIKE_DURATION = 0.7f;

    void Lightning::Init()
    {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        // per vertex: position (3), edge -1..1 across the ribbon (1), intensity (1)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (GLvoid*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (GLvoid*)(3 * sizeof(float)));
        glBindVertexArray(0);
    }

    // Midpoint displacement: split the segment and push the middle sideways, recursively.
    void Lightning::BuildChannel(const glm::vec3& from, const glm::vec3& to, int depth, float jitter,
                                 std::vector<glm::vec3>& out)
    {
        if (depth == 0) {
            out.push_back(to);
            return;
        }
        glm::vec3 mid = 0.5f * (from + to);
        float len = glm::length(to - from);
        mid += glm::vec3(rnd(-1.0f, 1.0f), rnd(-0.3f, 0.3f), rnd(-1.0f, 1.0f)) * len * jitter;
        BuildChannel(from, mid, depth - 1, jitter, out);
        BuildChannel(mid, to, depth - 1, jitter, out);
    }

    void Lightning::Strike(const glm::vec3& cameraPos, const glm::vec3& cameraForward, double now)
    {
        strikeTime = now;
        branches.clear();

        // Somewhere ahead of the camera (within ~50 degrees of the view), 120-220 units away.
        glm::vec2 fwd = glm::normalize(glm::vec2(cameraForward.x, cameraForward.z) + glm::vec2(1e-4f));
        float angle = atan2f(fwd.y, fwd.x) + glm::radians(rnd(-50.0f, 50.0f));
        float distance = rnd(120.0f, 220.0f);
        glm::vec3 ground = cameraPos + glm::vec3(cosf(angle), 0.0f, sinf(angle)) * distance;
        ground.y = -5.0f; // below the terrain, so hills hide the bottom of the bolt
        glm::vec3 top = ground + glm::vec3(rnd(-25.0f, 25.0f), 0.0f, rnd(-25.0f, 25.0f));
        top.y = 115.0f; // just under the cloud sheet

        Branch main;
        main.intensity = 1.0f;
        main.points.push_back(top);
        BuildChannel(top, ground, 7, 0.18f, main.points);
        branches.push_back(main);

        // Side branches fork off the upper part of the channel and fade out partway down.
        int forks = 2 + rand() % 4;
        for (int i = 0; i < forks; i++) {
            const std::vector<glm::vec3>& mp = branches[0].points;
            const glm::vec3 start = mp[(size_t)(rnd(0.1f, 0.6f) * (mp.size() - 1))];
            float length = rnd(20.0f, 50.0f);
            glm::vec3 end = start + glm::vec3(rnd(-1.0f, 1.0f), rnd(-1.6f, -0.7f), rnd(-1.0f, 1.0f)) * length;
            Branch b;
            b.intensity = rnd(0.3f, 0.6f);
            b.points.push_back(start);
            BuildChannel(start, end, 5, 0.22f, b.points);
            branches.push_back(b);
        }
    }

    float Lightning::Flash(double now) const
    {
        float t = (float)(now - strikeTime);
        if (t < 0.0f || t > STRIKE_DURATION)
            return 0.0f;
        float flash = 0.0f;
        for (int i = 0; i < PULSES; i++)
            if (t >= PULSE_TIME[i])
                flash += PULSE_AMPLITUDE[i] * expf(-(t - PULSE_TIME[i]) / 0.05f);
        return glm::min(flash, 1.2f);
    }

    void Lightning::Draw(gps::Shader& shader, const glm::mat4& view, const glm::mat4& projection,
                         const glm::vec3& cameraPos, double now)
    {
        float brightness = Flash(now);
        if (brightness < 0.02f || branches.empty())
            return;

        // Rebuild the ribbons each frame so they keep facing the camera.
        std::vector<float> data;
        for (size_t b = 0; b < branches.size(); b++) {
            const Branch& br = branches[b];
            float halfWidth = 2.0f * (0.5f + 0.5f * br.intensity);
            for (size_t i = 0; i + 1 < br.points.size(); i++) {
                glm::vec3 p0 = br.points[i], p1 = br.points[i + 1];
                glm::vec3 along = p1 - p0;
                glm::vec3 toCamera = cameraPos - 0.5f * (p0 + p1);
                glm::vec3 side = glm::normalize(glm::cross(along, toCamera)) * halfWidth;
                // branches taper towards their tips
                float fade = br.intensity * (b == 0 ? 1.0f : 1.0f - (float)i / br.points.size());
                const glm::vec3 corners[6] = { p0 - side, p0 + side, p1 + side, p0 - side, p1 + side, p1 - side };
                const float edges[6] = { -1, 1, 1, -1, 1, -1 };
                for (int k = 0; k < 6; k++) {
                    data.push_back(corners[k].x); data.push_back(corners[k].y); data.push_back(corners[k].z);
                    data.push_back(edges[k]); data.push_back(fade);
                }
            }
        }

        shader.useShaderProgram();
        glUniformMatrix4fv(gps::uniformLocation(shader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(gps::uniformLocation(shader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(gps::uniformLocation(shader.shaderProgram, "boltBrightness"), brightness);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STREAM_DRAW);

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE); // light adds up
        glDepthMask(GL_FALSE);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(data.size() / 5));
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glBindVertexArray(0);
    }
}
