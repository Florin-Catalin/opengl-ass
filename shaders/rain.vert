#version 410 core

layout(location=0) in vec4 seed; // per drop: xyz = spot in the box (0..1), w = variation

uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraPosWorld;
uniform float time;
uniform vec3 rainVelocity;  // world units per second (mostly down, slanted by the wind)
uniform vec3 rainBoxSize;   // the box of rain around the camera
uniform float streakLength; // how long each streak is drawn (motion blur)

out float dropFade;

void main()
{
	// Each drop falls forever along rainVelocity; mod() wraps it back into the box around the
	// camera. Positions stay fixed in the world as the camera moves; only the wrapping follows it.
	vec3 travel = rainVelocity * time * (0.85 + 0.3 * seed.w);
	vec3 p = mod(seed.xyz * rainBoxSize + travel - cameraPosWorld, rainBoxSize);
	p += cameraPosWorld - 0.5 * rainBoxSize;

	if (gl_VertexID == 1)
		p -= normalize(rainVelocity) * streakLength; // the tail of the streak

	// Fade out towards the box edges so wrapping drops don't pop, and right at the camera.
	vec3 rel = (p - cameraPosWorld) / (0.5 * rainBoxSize);
	float edge = max(max(abs(rel.x), abs(rel.y)), abs(rel.z));
	dropFade = (1.0 - smoothstep(0.6, 1.0, edge)) * smoothstep(0.5, 2.0, length(p - cameraPosWorld));
	dropFade *= 0.6 + 0.4 * seed.w;

	gl_Position = projection * view * vec4(p, 1.0);
}
