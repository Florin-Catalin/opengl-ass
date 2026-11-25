//
//  Tornado.hpp
//  A tornado: a translucent funnel (a tube mesh whose shape, sway and spin are computed in
//  shaders/tornado.vert/.frag) and a cloud of debris and dust whirling around its base
//  (points animated in shaders/tornadoDebris.vert).
//

#ifndef Tornado_hpp
#define Tornado_hpp

#include <GL/glew.h>
#include "Shader.hpp"

namespace gps {

    class Tornado
    {
    public:
        // Builds the funnel grid (rings x segments) and the debris seeds. Call once, after GL init.
        void Init(int rings, int segments, int debrisCount);

        // Both draw with additive-free alpha blending and no depth writes; uniforms (base
        // position, matrices, lighting, time, visibility) are set by the caller.
        void DrawFunnel(gps::Shader& shader);
        void DrawDebris(gps::Shader& shader);

    private:
        GLuint funnelVAO = 0, funnelVBO = 0, funnelEBO = 0;
        GLsizei funnelIndexCount = 0;
        GLuint debrisVAO = 0, debrisVBO = 0;
        int debrisCount = 0;
    };
}

#endif /* Tornado_hpp */
