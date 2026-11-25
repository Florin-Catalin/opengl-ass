//
//  Collision.hpp
//  2D (XZ-plane) collision between oriented boxes, using the separating axis test.
//

#ifndef Collision_hpp
#define Collision_hpp

#include "glm/glm.hpp"
#include <vector>

namespace gps {

    // Axis-aligned bounds in a model's own space.
    struct Bounds
    {
        glm::vec3 min;
        glm::vec3 max;
    };

    // A box footprint on the ground, as its 4 corners in world XZ (counter- or clockwise order).
    struct Footprint
    {
        glm::vec2 corners[4];
    };

    // Projects a model-space box through a model matrix and keeps its XZ footprint.
    inline Footprint makeFootprint(const Bounds& b, const glm::mat4& model)
    {
        const glm::vec2 local[4] = {
            glm::vec2(b.min.x, b.min.z), glm::vec2(b.max.x, b.min.z),
            glm::vec2(b.max.x, b.max.z), glm::vec2(b.min.x, b.max.z)
        };
        Footprint f;
        for (int i = 0; i < 4; i++) {
            glm::vec4 w = model * glm::vec4(local[i].x, 0.0f, local[i].y, 1.0f);
            f.corners[i] = glm::vec2(w.x, w.z);
        }
        return f;
    }

    inline void projectFootprint(const Footprint& f, const glm::vec2& axis, float& lo, float& hi)
    {
        lo = hi = glm::dot(f.corners[0], axis);
        for (int i = 1; i < 4; i++) {
            float p = glm::dot(f.corners[i], axis);
            lo = glm::min(lo, p);
            hi = glm::max(hi, p);
        }
    }

    // True if the two convex footprints overlap (separating axis theorem on both boxes' edges).
    inline bool footprintsOverlap(const Footprint& a, const Footprint& b)
    {
        const Footprint* boxes[2] = { &a, &b };
        for (int k = 0; k < 2; k++) {
            for (int i = 0; i < 2; i++) { // a rectangle has only 2 distinct edge directions
                glm::vec2 edge = boxes[k]->corners[i + 1] - boxes[k]->corners[i];
                glm::vec2 axis(-edge.y, edge.x);
                float aLo, aHi, bLo, bHi;
                projectFootprint(a, axis, aLo, aHi);
                projectFootprint(b, axis, bLo, bHi);
                if (aHi <= bLo || bHi <= aLo)
                    return false;
            }
        }
        return true;
    }

    inline bool overlapsAny(const Footprint& f, const std::vector<Footprint>& obstacles)
    {
        for (size_t i = 0; i < obstacles.size(); i++)
            if (footprintsOverlap(f, obstacles[i]))
                return true;
        return false;
    }
}

#endif /* Collision_hpp */
