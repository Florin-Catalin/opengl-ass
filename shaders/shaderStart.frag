#version 410 core

in vec3 normal;
in vec4 fragPosEye;
in vec4 fragPosLightSpace;
in vec2 fragTexCoords;
in vec4 fragPos;
in vec3 normalWorld;
in vec3 posModel;

out vec4 fColor;

//lighting
uniform vec3 posLightPos;
uniform vec3 viewPos;
uniform	mat3 normalMatrix;
uniform mat3 lightDirMatrix;
uniform	vec3 lightColor;
uniform	vec3 lightDir;
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;
uniform int lightsOn;

//day/night cycle: ambient light and point-light (lamp) color are set per frame
uniform vec3 ambientLight;
uniform vec3 pointLightColor;

//campfire: flickering point light, position given in eye space
uniform vec3 fireLightPosEye;
uniform vec3 fireColor;
uniform float fireIntensity;

#include "clouds.glsl"

uniform float rainAmount; // 0..1: wet surfaces (darker, shinier) and thicker haze

// Puddles: on the terrain only, filled by rain and drying slowly. They mirror the sky (skybox,
// clouds, moon) and show raindrop ripples while it rains.
uniform float puddleSurface;  // 1 while drawing the terrain, 0 for everything else
uniform float puddleWetness;  // 0 = dry .. 1 = puddles at their largest
uniform samplerCube skyboxCube;
uniform vec3 skyTint;
uniform vec3 cameraPosWorld;
uniform vec3 moonDirWorld;
uniform float moonVisibility;
uniform vec3 cloudLitColor;
uniform vec3 cloudShadowColor;
uniform float time;


//fog
uniform	vec3 startFog;
uniform vec3 fogColor;


vec3 o;

float constant = 1.0f;
float pointLinear = 10.0f;
float pointQuadratic = 3.0f;
float pointSpecularStrenght = 10.0f;

vec3 ambient;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float shininess = 64.0f;

uniform vec3 pointLight;


void computeLightComponents()
{		
	vec3 cameraPosEye = vec3(0.0f);//in eye coordinates, the viewer is situated at the origin
	
	//transform normal
	vec3 normalEye = normalize(normalMatrix * normal);	
	
	//compute light direction
	vec3 lightDirN = normalize(lightDirMatrix * lightDir);	

	//compute view direction 
	vec3 viewDirN = normalize(cameraPosEye - fragPosEye.xyz);
	
	//compute half vector
	vec3 halfVector = normalize(lightDirN + viewDirN);
		
	//compute ambient light
	ambient = ambientLight;
	
	//compute diffuse light
	diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
	
	//compute specular light
	float specCoeff = pow(max(dot(halfVector, normalEye), 0.0f), shininess);
	specular = specularStrength * specCoeff * lightColor;
}

float computeShadow()
{	
	// perform perspective divide
    vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    if(normalizedCoords.z > 1.0f)
        return 0.0f;
    // Transform to [0,1] range
    normalizedCoords = normalizedCoords * 0.5f + 0.5f;
    // Get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, normalizedCoords.xy).r;    
    // Get depth of current fragment from light's perspective
    float currentDepth = normalizedCoords.z;
    // Check whether current frag pos is in shadow
    float bias = 0.005f;
    float shadow = currentDepth - bias> closestDepth  ? 1.0f : 0.0f;

    return shadow;	
}

void computePointLight()
{
	vec3 norm = normalize(normalMatrix * normal);
	vec3 lightDir = normalize(posLightPos - fragPos.xyz);

    // diffuse shading
    float diff = max(dot(norm, lightDir), 0.0);

    // specular shading
	vec3 viewDir = normalize(viewPos - fragPos.xyz);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);

    // attenuation
    float distance    = length(posLightPos - fragPos.xyz);
    float attenuation = 1.0 / (constant + pointLinear * distance + 
  			     pointQuadratic * (distance * distance));    

    // combine results
    ambient  += pointLightColor * vec3(texture(diffuseTexture, fragTexCoords)) * attenuation;
    diffuse  += pointLightColor * diff * vec3(texture(diffuseTexture, fragTexCoords)) * attenuation;
    specular += pointLightColor * spec * pointSpecularStrenght * attenuation;
}

// Campfire light: diffuse + Blinn-Phong specular, all in eye space.
// Returns the light added on top of the textured color.
vec3 computeFireLight(vec3 albedo, vec3 specMap)
{
	if (fireIntensity <= 0.0f)
		return vec3(0.0f);

	vec3 normalEye = normalize(normalMatrix * normal);
	vec3 toLight = fireLightPosEye - fragPosEye.xyz;
	float dist = length(toLight);
	vec3 L = toLight / dist;
	vec3 V = normalize(-fragPosEye.xyz);
	vec3 H = normalize(L + V);

	float attenuation = fireIntensity / (1.0f + 0.35f * dist + 0.25f * dist * dist);
	float diff = max(dot(normalEye, L), 0.0f);
	// Keep a small specular floor so the fire glints on surfaces whose specular map is black.
	float spec = diff > 0.0f ? pow(max(dot(normalEye, H), 0.0f), 48.0f) : 0.0f;
	vec3 specStrength = max(specMap, vec3(0.25f));

	return fireColor * attenuation * (diff * albedo + 1.5f * spec * specStrength);
}

// Expanding rings where raindrops hit the water. Returns a sideways tilt for the surface normal.
vec2 rainRipples(vec2 p)
{
	vec2 tilt = vec2(0.0);
	for (int layer = 0; layer < 2; layer++) {
		vec2 q = p * (3.0 + 2.0 * float(layer)) + float(layer) * 17.0;
		ivec2 cell = ivec2(floor(q));
		for (int dx = -1; dx <= 1; dx++)
		for (int dy = -1; dy <= 1; dy++) {
			ivec2 c = cell + ivec2(dx, dy);
			float h = cloudHash(c + ivec2(layer * 101, 7));
			vec2 center = vec2(c) + vec2(cloudHash(c + ivec2(3, 11)), cloudHash(c + ivec2(19, 5)));
			float age = fract(time * (0.6 + 0.4 * h) + h); // each drop restarts on its own clock
			vec2 d = q - center;
			float r = length(d);
			float front = r - age * 0.9;                    // the ring grows outwards
			float ring = sin(front * 25.0) * smoothstep(0.15, 0.0, abs(front)) * (1.0 - age);
			tilt += ring * d / max(r, 1e-3);
		}
	}
	return tilt * 0.15;
}

// What a puddle mirrors in direction R: the skybox, then clouds, then the moon (with its halo).
vec3 reflectedSky(vec3 R)
{
	vec3 sky = texture(skyboxCube, R).rgb * skyTint;
	float cloud = cloudDensityAlong(fragPos.xyz, R);
	sky = mix(sky, mix(cloudShadowColor, cloudLitColor, 0.6f), cloud);

	float m = clamp(dot(R, moonDirWorld), -1.0f, 1.0f);
	float disc = smoothstep(cos(0.045f), cos(0.035f), m);           // moon is ~0.04 rad across
	float halo = exp(-max(acos(m) - 0.04f, 0.0f) / 0.06f) * 0.35f;
	sky += (vec3(0.8f, 0.88f, 1.0f) * disc + vec3(0.45f, 0.6f, 1.0f) * halo) * moonVisibility * (1.0f - cloud);
	return sky;
}

// Puddles on flat terrain. Returns the colour with puddles (and their damp rims) applied.
vec3 applyPuddles(vec3 color)
{
	if (puddleSurface < 0.5f || puddleWetness <= 0.0f)
		return color;

	float flatness = smoothstep(0.9f, 0.97f, normalize(normalWorld).y);
	float n = cloudFbm(posModel.xz * 0.18f + vec2(37.0f, 11.0f));
	float threshold = mix(0.78f, 0.58f, puddleWetness);             // wetter -> bigger puddles
	float puddle = smoothstep(threshold, threshold + 0.04f, n) * flatness;
	float damp = smoothstep(threshold - 0.08f, threshold, n) * flatness;
	color *= 1.0f - 0.35f * damp * puddleWetness;                    // dark wet ground around them
	if (puddle <= 0.0f)
		return color;

	vec2 tilt = rainRipples(posModel.xz) * rainAmount;
	vec3 N = normalize(vec3(tilt.x, 1.0f, tilt.y));
	vec3 V = normalize(cameraPosWorld - fragPos.xyz);
	vec3 R = reflect(-V, N);
	R.y = abs(R.y); // ripples must not make it look under the ground

	// Water reflects little straight down and almost everything at grazing angles (Fresnel).
	float fresnel = 0.04f + 0.96f * pow(1.0f - max(dot(N, V), 0.0f), 5.0f);
	float reflectivity = mix(0.35f, 1.0f, fresnel);
	vec3 water = mix(color * 0.5f, reflectedSky(R), reflectivity);
	return mix(color, water, puddle);
}

float computeFog()
{
 float fogDensity = 0.010f + 0.025f * rainAmount;
 float fragmentDistance = length(fragPosEye);
 float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));

 return clamp(fogFactor, 0.0f, 1.0f);
}

void main() 
{
	computeLightComponents();
	
	if(lightsOn==1)
	{
		//computeLightComponents();
		computePointLight();
	}	
		 
	float shadow = computeShadow();
	// Clouds between this point and the sun/moon block part of the direct light.
	float cloudShade = 1.0f - 0.6f * cloudDensityAlong(fragPos.xyz, normalize(lightDir));
	diffuse *= cloudShade;
	specular *= cloudShade;
	
	// Wet surfaces look darker and get a glossy sheen.
	vec3 albedo = vec3(texture(diffuseTexture, fragTexCoords)) * (1.0f - 0.3f * rainAmount);
	vec3 specMap = vec3(texture(specularTexture, fragTexCoords));
	specMap = max(specMap, vec3(0.35f * rainAmount));

	//modulate with diffuse map
	ambient *= albedo;
	diffuse *= albedo;
	//modulate with specular map
	specular *= specMap;
	
	//modulate with shadow (the sun/moon shadow map does not apply to the fire)
	vec3 color = min((ambient + (1.0f - shadow)*diffuse) + (1.0f - shadow)*specular
		+ computeFireLight(albedo, specMap), 1.0f);
    vec3 colorFinal = applyPuddles(min(color, 1.0f));

	// Fog when switched on (F), and rain haze while it rains.
	float fogAmount = max(startFog.x, 0.85f * rainAmount);
	if (fogAmount > 0.0f) {
		float fogFactor = computeFog();
		fColor = vec4(mix(colorFinal, fogColor, (1.0f - fogFactor) * fogAmount), 1.0f);
	} else {
		fColor = vec4(colorFinal, 1.0f);
	}
}
