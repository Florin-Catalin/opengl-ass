#version 410 core

in vec3 normal;
in vec4 fragPosEye;
in vec4 fragPosLightSpace;
in vec2 fragTexCoords;
in vec4 fragPos;

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

float computeFog()
{
 float fogDensity = 0.010f;
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
	
	vec3 albedo = vec3(texture(diffuseTexture, fragTexCoords));
	vec3 specMap = vec3(texture(specularTexture, fragTexCoords));

	//modulate with diffuse map
	ambient *= albedo;
	diffuse *= albedo;
	//modulate with specular map
	specular *= specMap;
	
	//modulate with shadow (the sun/moon shadow map does not apply to the fire)
	vec3 color = min((ambient + (1.0f - shadow)*diffuse) + (1.0f - shadow)*specular
		+ computeFireLight(albedo, specMap), 1.0f);
    vec3 colorFinal = min(color, 1.0f);

	if (startFog.x==1.0f){
		float fogFactor = computeFog();
		fColor = vec4(fogColor, 1.0f)*(1.0f-fogFactor)+vec4(colorFinal, 1.0f)*fogFactor;
	}else{
		fColor = vec4(colorFinal, 1.0f);
	}
}
