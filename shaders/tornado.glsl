// Shared funnel shape, pulled in with #include "tornado.glsl" by the funnel and debris shaders,
// so the debris whirls around the same bent axis the funnel is drawn on.

uniform vec3 tornadoBase;   // world position of the funnel's foot
uniform float funnelHeight; // up to the cloud base
uniform float tornadoTime;  // seconds
uniform float tornadoWidth; // 0..1: thin rope while forming or lifting, 1 = full width
uniform float shellScale;   // funnel only: 1 = the dense core, larger = the torn outer dust shell

// Horizontal offset of the funnel's axis at height fraction v: it leans and snakes, more higher up.
vec2 funnelAxis(float v)
{
	float t = tornadoTime;
	return vec2(sin(v * 3.0 + t * 0.6) * 14.0 + sin(v * 7.0 - t * 1.1) * 2.0,
	            cos(v * 2.4 + t * 0.45) * 11.0 + cos(v * 6.0 + t * 0.9) * 1.6) * v;
}

// Funnel radius at height fraction v: narrow at the ground, flaring into the cloud base.
float funnelRadius(float v)
{
	float r = mix(1.2, 28.0, pow(v, 1.5)) + 2.5 * exp(-v * 25.0); // + a flared foot
	r *= 1.0 + 0.08 * sin(v * 40.0 - tornadoTime * 3.0);          // wobbling bulges
	return r * mix(0.3, 1.0, tornadoWidth);
}
