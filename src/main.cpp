//
//  main.cpp
//  OpenGL Shadows
//
//  Created by CGIS on 05/12/16.
//  Copyright � 2016 CGIS. All rights reserved.
//

#include <iostream>
#include "glm/glm.hpp"//core glm functionality
#include "glm/gtc/matrix_transform.hpp"//glm extension for generating common transformation matrices
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <cmath>
#include "Shader.hpp"
#include "Camera.hpp"
#include "SkyBox.hpp"
#include "Planets.hpp"
#include "Rain.hpp"
#include "Lightning.hpp"
#include "Fireflies.hpp"
#include "Snow.hpp"
#include "Tornado.hpp"
#define TINYOBJLOADER_IMPLEMENTATION

#include "Model3D.hpp"
#include "Mesh.hpp"





int glWindowWidth = 1920;
int glWindowHeight = 1080;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

const GLuint SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;

glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;
glm::mat3 lightDirMatrix;
GLuint lightDirMatrixLoc;

GLuint fogLoc;
glm::vec3 lightDir;
GLuint lightDirLoc;
glm::vec3 lightColor;
GLuint lightColorLoc;
glm::vec3 posLightPos;
GLuint posLightPosLoc;

glm::vec3 viewPos;
GLuint viewPosLoc;
GLuint fogDensityLoc;
GLuint lightsOnLoc;

// Day/night cycle. timeOfDay runs 0..1: 0 = sunrise, 0.25 = noon, 0.5 = sunset, 0.75 = midnight.
bool dayNightRunning = true;
float timeOfDay = 0.05f;
const float DAY_LENGTH_SECONDS = 60.0f;
double lastFrameTime = 0.0;
glm::vec3 currentLightDir = glm::vec3(0.0f, 1.0f, 2.0f); // direction towards the sun or moon
glm::vec3 skyColor = glm::vec3(1.0f);
float moonGlow = 0.0f; // 0..1, how strongly the moon lights the scene and glows in the sky

// Clouds: one noise sheet shared by the sky, moon and scene shaders (shaders/clouds.glsl).
float cloudCoverage = 0.3f;                  // 0 = clear, 1 = overcast; [ and ] change it
const glm::vec2 WIND_VELOCITY(3.0f, 1.2f);   // world units per second
glm::vec2 cloudOffset = glm::vec2(0.0f);
glm::vec3 cloudLitColor = glm::vec3(1.0f);
glm::vec3 cloudShadowColor = glm::vec3(0.6f);
void setCloudUniforms(GLuint program);

// Shooting stars, spawned with M. They are drawn in the skybox shader (behind the clouds).
struct Meteor
{
	glm::vec3 start;   // unit direction where it appears
	glm::vec3 tangent; // unit direction of travel, perpendicular to start
	double spawnTime;
	float duration;    // seconds
	float arc;         // total angle travelled (radians)
	float tail;        // trail length (radians)
	float brightness;
	float fireball;    // 0 = ordinary white meteor, 1 = bright green-orange fireball
};
const int MAX_METEORS = 8; // must match the skybox shader
std::vector<Meteor> meteors;
void spawnMeteor();

// Rain, toggled with R. rainAmount eases towards 0 or 1 so storms roll in and clear up gradually.
bool raining = false;
float rainAmount = 0.0f;
const float RAIN_FADE_SECONDS = 4.0f;
const int RAIN_MAX_DROPS = 12000;
gps::Rain rain;
gps::Shader rainShader;
glm::vec3 rainColor = glm::vec3(0.6f);

// Fireflies around the camp at night (not in the rain).
gps::Fireflies fireflies;
gps::Shader firefliesShader;
const int FIREFLY_COUNT = 160;

// Snow, toggled with V (it replaces the rain, and R replaces it). snowAmount eases like rainAmount.
// snowCover is how much snow lies on upward-facing surfaces: it builds up while it snows and
// melts afterwards, faster in sunshine and much faster in the rain.
bool snowing = false;
float snowAmount = 0.0f;
float snowCover = 0.0f;
const float SNOW_COVER_SECONDS = 40.0f; // to go from bare ground to fully white
const float SNOW_MELT_SECONDS = 50.0f;  // in full sun; at night it barely melts, rain clears it in ~20 s
const int SNOW_MAX_FLAKES = 7000;
gps::Snow snow;
gps::Shader snowShader;
glm::vec3 snowColor = glm::vec3(0.9f);
// Sandstorm, toggled with U (it replaces rain or snow, and R or V replace it). Dust hides the sky
// and the far scene, light turns orange and dim, and sand streaks past with a strong wind.
bool sandstorm = false;
float sandAmount = 0.0f;
const float SAND_FADE_SECONDS = 8.0f; // storms roll in and settle more slowly than rain
const int SAND_MAX_GRAINS = 9000;
gps::Rain sand;                       // the rain streak renderer, blown sideways
glm::vec3 dustColor = glm::vec3(0.7f, 0.5f, 0.3f);
glm::vec2 dustOffset = glm::vec2(0.0f); // how far the dust billows have blown

// Tornado, toggled with O: the funnel grows down from the clouds, wanders across the valley and
// thins and lifts when switched off. tornadoAmount eases between 0 and 1.
bool tornadoOn = false;
float tornadoAmount = 0.0f;
const float TORNADO_FORM_SECONDS = 8.0f;
const float TORNADO_FUNNEL_HEIGHT = 115.0f; // its top sits in the cloud sheet (CLOUD_HEIGHT 120)
double tornadoSpawnTime = 0.0;
float tornadoPhase = 0.0f;                  // where along its wandering path it starts
glm::vec3 tornadoBase = glm::vec3(0.0f);
float tornadoShake = 0.0f;                  // 0..1, camera shake when it is close
// This frame's lighting from updateDayNight(), for effects drawn with their own shaders.
glm::vec3 ambientNow = glm::vec3(0.2f), directNow = glm::vec3(1.0f), fogColorNow = glm::vec3(0.5f);
gps::Tornado tornado;
gps::Shader tornadoShader;
gps::Shader tornadoDebrisShader;

// The largest of rainAmount, snowAmount, sandAmount and tornadoAmount: how much the weather closes in.
float overcast = 0.0f;

// Puddles: fill up while it rains (about 15 s) and dry slowly afterwards (about 90 s).
float puddleWetness = 0.0f;
const float PUDDLE_FILL_SECONDS = 15.0f;
const float PUDDLE_DRY_SECONDS = 90.0f;
const GLint SKYBOX_TEXTURE_UNIT = 5; // the scene shader samples the sky cubemap here for puddles
const GLint REFLECTION_TEXTURE_UNIT = 6;
const float PUDDLE_PLANE_Y = 0.013f; // height of the flat camp floor in the terrain, where puddles form
GLuint reflectionFBO = 0, reflectionTexture = 0, reflectionDepth = 0;
int reflectionWidth = 0, reflectionHeight = 0;

// Lightning: random strikes every 4-12 s while it rains hard; Z strikes immediately.
gps::Lightning lightning;
gps::Shader lightningShader;
double nextStrikeTime = 0.0; // 0 = not scheduled

// Rainbow: appears after the rain is switched off and stays about a minute and a half, if the sun
// is out and low enough (below the bow's radius, otherwise the bow is below the horizon).
const float RAINBOW_HOLD_SECONDS = 75.0f; // fully visible until then, then fades over 20 s
double rainStoppedTime = -1000.0;
float rainbowStrength = 0.0f;
void strikeLightning();
GLuint ambientLightLoc;
GLuint pointLightColorLoc;
GLuint fogColorLoc;

// Moon model drawn in the sky at night (objects/moon, converted from low_poly_moon.glb).
gps::Model3D moon;
gps::Shader moonShader;
glm::vec3 sunSkyDir = glm::vec3(0.0f, 1.0f, 0.0f);  // unit direction towards the sun
glm::vec3 moonSkyDir = glm::vec3(0.0f, -1.0f, 0.0f); // unit direction towards the moon
const glm::vec3 MOON_MODEL_CENTER = glm::vec3(0.0f, 10.2f, 0.0f); // the OBJ sits on y = 0
const float MOON_DISTANCE = 300.0f; // well inside the 1000-unit far plane
const float MOON_SCALE = 1.2f;

// Stars: the star sphere turns with the sun and moon (same arc and K/L azimuth), and drifts
// about 1 degree per day relative to the sun, as the real sky does over a year.
glm::mat3 starRotation = glm::mat3(1.0f);

// Planets. The calendar starts at J2000 + 6815 days (late August 2018), when Venus was the
// evening star and Mars (near opposition), Jupiter and Saturn were all up in the evening.
const double PLANET_START_DAY = 6815.0;
int dayCount = 0; // whole days elapsed in the scene
glm::vec3 planetDirs[gps::PLANET_COUNT];
glm::vec3 planetColors[gps::PLANET_COUNT];
float planetBrightness[gps::PLANET_COUNT];      // model radius ~10, so ~12 units across 300 units

// Campfire inside the house model (center of its stone ring, in model space).
const glm::vec3 FIRE_POS_MODEL = glm::vec3(-3.15f, 0.5f, -5.53f);
bool fireOn = true;
glm::vec3 fireJitter = glm::vec3(0.0f); // this frame's flame wobble, reused by the puddle reflection
// 1 = burning normally, 0 = put out. Rain douses it over ~10 s; it is relit gradually once the
// rain has fully stopped.
float fireStrength = 1.0f;
const float FIRE_DOUSE_SECONDS = 10.0f;
const float FIRE_RELIGHT_SECONDS = 8.0f;
GLuint fireLightPosEyeLoc;
GLuint fireColorLoc;
GLuint fireIntensityLoc;
float nightFactor = 0.0f; // 0 at day, 1 at night

// Start a little above the camp, looking over the tank and tents towards the hills.
gps::Camera myCamera(glm::vec3(0.0f, 3.5f, 10.0f), glm::vec3(-1.5f, 1.5f, -4.0f));
GLfloat cameraSpeed = 0.05f;

bool pressedKeys[1024];
GLfloat angle;
GLfloat angleF = 0;
GLfloat birdAngle;
GLfloat lightAngle;
GLfloat distanceRocket = 0;
GLfloat lampAmbient = 0;




gps::Model3D tents;
gps::Model3D grass;


gps::Model3D house;

gps::Model3D bird;
gps::Model3D tank;

gps::Model3D farmer;
gps::Model3D wood;
gps::Shader myCustomShader;
gps::Shader depthMapShader;

float angleTank = 0.0f;
float tankSpeed = 4.0f;     // units per second
float tankTurnSpeed = 1.5f; // radians per second
glm::vec3 tankPosition = glm::vec3(0.0f, 0.0f, 0.0f);

// Collision: the tank (farmer.obj) against the bounds of each mesh in the static scene objects.
gps::Bounds tankBounds;
std::vector<gps::Bounds> houseBounds, tentsBounds, woodBounds, rocketBounds;
float frameDt = 0.0f; // seconds since the previous frame


GLuint shadowMapFBO;
GLuint depthMapTexture;

gps::SkyBox mySkyBox;
gps::Shader skyboxShader;



glm::vec3 pointLightPos = glm::vec3(0.0f, 0.0f, 0.0f);

GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
		case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height)
{
	fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);
	//TODO
	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	myCustomShader.useShaderProgram();

	//set projection matrix
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	//send matrix data to shader
	GLint projLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set Viewport transform
	glViewport(0, 0, retina_width, retina_height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_N && action == GLFW_PRESS)
		dayNightRunning = !dayNightRunning;

	if (key == GLFW_KEY_B && action == GLFW_PRESS)
		fireOn = !fireOn;

	if (key == GLFW_KEY_T && action == GLFW_PRESS) {
		timeOfDay += 0.125f;
		if (timeOfDay >= 1.0f) {
			timeOfDay -= 1.0f;
			dayCount++;
		}
	}

	if (key == GLFW_KEY_LEFT_BRACKET && action == GLFW_PRESS)
		cloudCoverage = glm::max(cloudCoverage - 0.15f, 0.0f);

	if (key == GLFW_KEY_RIGHT_BRACKET && action == GLFW_PRESS)
		cloudCoverage = glm::min(cloudCoverage + 0.15f, 1.0f);

	if (key == GLFW_KEY_M && action == GLFW_PRESS)
		spawnMeteor();

	if (key == GLFW_KEY_Z && action == GLFW_PRESS && rainAmount > 0.3f)
		strikeLightning();

	if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		raining = !raining;
		if (!raining)
			rainStoppedTime = glfwGetTime();
		else {
			snowing = false;
			sandstorm = false;
		}
	}

	if (key == GLFW_KEY_V && action == GLFW_PRESS)
	{
		snowing = !snowing;
		if (snowing) {
			raining = false; // no rainbow: it changed to snow, it didn't clear up
			sandstorm = false;
		}
	}

	if (key == GLFW_KEY_O && action == GLFW_PRESS)
	{
		tornadoOn = !tornadoOn;
		if (tornadoOn && tornadoAmount <= 0.0f) {
			tornadoSpawnTime = glfwGetTime();
			tornadoPhase = rand() / (float)RAND_MAX * 6.2831853f;
		}
	}

	if (key == GLFW_KEY_U && action == GLFW_PRESS)
	{
		sandstorm = !sandstorm;
		if (sandstorm)
			raining = snowing = false;
	}

	if (key == GLFW_KEY_P && action == GLFW_PRESS)
		dayCount += 10; // watch the planets move against the stars

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
	myCamera.mouse_callback(xpos, ypos);
}

void move(gps::MOVE_DIRECTION direction, float speed)
{
	float step = speed * frameDt;
	switch (direction) {
	case gps::MOVE_FORWARD:
		tankPosition += step * glm::vec3(sin(angleF), 0.0f, cos(angleF));
		break;
	case gps::MOVE_BACKWARD:
		tankPosition -= step * glm::vec3(sin(angleF), 0.0f, cos(angleF));
		break;
	case gps::MOVE_RIGHT:
		angleF -= tankTurnSpeed * frameDt;
		break;
	case gps::MOVE_LEFT:
		angleF += tankTurnSpeed * frameDt;
		break;
	}
}

// The tank turns with its heading (angleF), so it drives along its own long axis.
glm::mat4 tankModelMatrix()
{
	glm::mat4 m = glm::rotate(glm::mat4(1.0f), glm::radians(angleTank), glm::vec3(0, 1, 0));
	m = glm::translate(m, tankPosition);
	return glm::rotate(m, angleF, glm::vec3(0, 1, 0));
}

static void addFootprints(std::vector<gps::Footprint>& out, const std::vector<gps::Bounds>& parts, const glm::mat4& model)
{
	for (size_t i = 0; i < parts.size(); i++) {
		// Skip flat decals and anything entirely above the tank.
		if (parts[i].max.y - parts[i].min.y < 0.15f || parts[i].min.y > tankBounds.max.y)
			continue;
		out.push_back(gps::makeFootprint(parts[i], model));
	}
}

// Footprints of everything the tank can hit, with the current scene rotation (Q/E).
std::vector<gps::Footprint> sceneObstacles()
{
	glm::mat4 sceneModel = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	std::vector<gps::Footprint> obstacles;
	addFootprints(obstacles, houseBounds, sceneModel);
	addFootprints(obstacles, tentsBounds, sceneModel);
	addFootprints(obstacles, woodBounds, sceneModel);
	if (distanceRocket < 1.0f) // still on the launch pad
		addFootprints(obstacles, rocketBounds, sceneModel);
	return obstacles;
}

// Counts how many obstacles the tank overlaps in its current state.
static int tankOverlapCount(const std::vector<gps::Footprint>& obstacles)
{
	gps::Footprint tank = gps::makeFootprint(tankBounds, tankModelMatrix());
	int n = 0;
	for (size_t i = 0; i < obstacles.size(); i++)
		if (gps::footprintsOverlap(tank, obstacles[i]))
			n++;
	return n;
}

// Applies the arrow-key movement, then undoes it if it drives the tank into something.
// Tries sliding along each axis before giving up, so the tank can scrape along walls.
// A move that doesn't add new overlaps is allowed, so rotating the scene onto the tank
// (Q/E) never leaves it stuck.
void moveTankWithCollision()
{
	if (!pressedKeys[GLFW_KEY_UP] && !pressedKeys[GLFW_KEY_DOWN])
		return;

	std::vector<gps::Footprint> obstacles = sceneObstacles();
	int before = tankOverlapCount(obstacles);
	glm::vec3 oldPos = tankPosition;
	float oldAngleF = angleF, oldAngleTank = angleTank;

	gps::MOVE_DIRECTION dir = pressedKeys[GLFW_KEY_UP] ? gps::MOVE_FORWARD : gps::MOVE_BACKWARD;
	angleTank += dir == gps::MOVE_FORWARD ? 0.0015f : -0.0015f;
	if (pressedKeys[GLFW_KEY_LEFT])
		move(gps::MOVE_LEFT, tankSpeed);
	if (pressedKeys[GLFW_KEY_RIGHT])
		move(gps::MOVE_RIGHT, tankSpeed);
	move(dir, tankSpeed);

	if (tankOverlapCount(obstacles) <= before)
		return;

	// Blocked: keep the old heading and try moving along X only, then Z only.
	glm::vec3 delta = tankPosition - oldPos;
	angleF = oldAngleF;
	angleTank = oldAngleTank;
	const glm::vec3 slides[2] = { glm::vec3(delta.x, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, delta.z) };
	for (int i = 0; i < 2; i++) {
		tankPosition = oldPos + slides[i];
		if (tankOverlapCount(obstacles) <= before)
			return;
	}
	tankPosition = oldPos;
}

bool isWireframe = false;

bool startFog = false;
int lightsOn = 0;

int acc = 1;
void processMovement()
{
	birdAngle += 0.3f;
	if (birdAngle > 360.0f)
		birdAngle -= 360.0f;

	/*if (birdAngle >= 180) {
		fprintf(stdout, "bird is at point: %f \n", birdAngle);
	}*/

	if (pressedKeys[GLFW_KEY_C]) {
		distanceRocket += 0.015 * acc; 
		acc++; 
	}

	if (pressedKeys[GLFW_KEY_Q]) {
		angle += 0.1f;
		if (angle > 360.0f)
			angle -= 360.0f;
	}

	if (pressedKeys[GLFW_KEY_E]) {
		angle -= 0.1f;
		if (angle < 0.0f)
			angle += 360.0f;
	}

	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_K]) {
		lightAngle += 0.3f;
		if (lightAngle > 360.0f)
			lightAngle -= 360.0f;
	}

	if (pressedKeys[GLFW_KEY_L]) {
		lightAngle -= 0.3f;
		if (lightAngle < 0.0f)
			lightAngle += 360.0f;
	}

	//wireframe test
	if (pressedKeys[GLFW_KEY_X]) {
		if (isWireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		isWireframe = !isWireframe;
	}

	if (pressedKeys[GLFW_KEY_F]) {
		startFog = true;
	}

	if (pressedKeys[GLFW_KEY_G]) {
		startFog = false;
	}

	
	moveTankWithCollision();
	
	if (pressedKeys[GLFW_KEY_H]) {
		lightsOn = 1;
	}

	if (pressedKeys[GLFW_KEY_J]) {
		lightsOn = 0;
	}
}

bool initOpenGLWindow()
{
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_SAMPLES, 4);
	glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Shader Example", NULL, NULL);
	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwMakeContextCurrent(glWindow);


	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouseCallback);
	glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	return true;
}

void initOpenGLState()
{
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, retina_width, retina_height);

	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	//glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initFBOs()
{
	//generate FBO ID
	glGenFramebuffers(1, &shadowMapFBO);

	//create depth texture for FBO
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//attach texture to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix()
{
	const GLfloat near_plane = 1.0f, far_plane = 10.0f;
	glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);

	glm::mat4 lightView = glm::lookAt(currentLightDir, myCamera.getCameraTarget(), glm::vec3(0.0f, 1.0f, 0.0f));

	return lightProjection * lightView;
}

void initModels()
{
	
	tents = gps::Model3D("objects/tents/tents.obj", "objects/tents/");
	grass = gps::Model3D("objects/grass/GRASS.obj", "objects/grass/");
	
	house = gps::Model3D("objects/house/house1.obj", "objects/house/");

	bird = gps::Model3D("objects/vultur/vultur.obj", "objects/vultur/");
	tank = gps::Model3D("objects/goat1/goat1.obj", "objects/goat1/");

	farmer = gps::Model3D("objects/farmer/farmer.obj", "objects/farmer/");
	wood = gps::Model3D("objects/wood/wood.obj", "objects/wood/");
	moon = gps::Model3D("objects/moon/moon.obj", "objects/moon/");
	rain.Init(RAIN_MAX_DROPS);
	snow.Init(SNOW_MAX_FLAKES);
	sand.Init(SAND_MAX_GRAINS);
	tornado.Init(64, 48, 3000);
	lightning.Init();
	// The camp floor is around y = 0; the house, tents and wood pile lie within this box.
	fireflies.Init(FIREFLY_COUNT, glm::vec3(-14.0f, 0.3f, -14.0f), glm::vec3(14.0f, 2.2f, 10.0f));

	tankBounds = farmer.GetBounds();
	houseBounds = house.GetMeshBounds();
	tentsBounds = tents.GetMeshBounds();
	woodBounds = wood.GetMeshBounds();
	rocketBounds = tank.GetMeshBounds();
}

void initShaders()
{
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	depthMapShader.loadShader("shaders/simpleDepthMap.vert", "shaders/simpleDepthMap.frag");
	moonShader.loadShader("shaders/moon.vert", "shaders/moon.frag");
	rainShader.loadShader("shaders/rain.vert", "shaders/rain.frag");
	snowShader.loadShader("shaders/snow.vert", "shaders/snow.frag");
	tornadoShader.loadShader("shaders/tornado.vert", "shaders/tornado.frag");
	tornadoDebrisShader.loadShader("shaders/tornadoDebris.vert", "shaders/tornadoDebris.frag");
	lightningShader.loadShader("shaders/lightning.vert", "shaders/lightning.frag");
	firefliesShader.loadShader("shaders/fireflies.vert", "shaders/fireflies.frag");
}

void initUniforms()
{
	myCustomShader.useShaderProgram();

	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");

	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");

	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");

	lightDirMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDirMatrix");

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 1.0f, 2.0f);
	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	//set point light
	posLightPos = glm::vec3(1.0f,5.0f,1.0f);
	posLightPosLoc = glGetUniformLocation(myCustomShader.shaderProgram, "posLightPos");
	glUniform3fv(posLightPosLoc, 1, glm::value_ptr(posLightPos));

	fogLoc = glGetUniformLocation(myCustomShader.shaderProgram, "startFog");
	fogColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "fogColor");
	ambientLightLoc = glGetUniformLocation(myCustomShader.shaderProgram, "ambientLight");
	lightsOnLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightsOn");
	fireLightPosEyeLoc = glGetUniformLocation(myCustomShader.shaderProgram, "fireLightPosEye");
	fireColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "fireColor");
	fireIntensityLoc = glGetUniformLocation(myCustomShader.shaderProgram, "fireIntensity");
	// samplerCube must not default to unit 0, where a 2D texture is bound.
	glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "skyboxCube"), SKYBOX_TEXTURE_UNIT);
	glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "reflectionMap"), REFLECTION_TEXTURE_UNIT);
	pointLightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "pointLightColor");
	glUniform3fv(pointLightColorLoc, 1, glm::value_ptr(glm::vec3(1.0f, 0.8f, 0.5f))); // warm lamp





}

bool invisibilistyt = false;

// Draws the moon far away along moonSkyDir, centred on the camera so it doesn't move as you walk.
// Drawn after the skybox with alpha blending, faded by nightFactor.
void drawMoon()
{
	float alpha = glm::clamp(nightFactor * 1.2f, 0.0f, 1.0f) * (1.0f - sandAmount); // lost in the dust
	if (alpha < 0.01f)
		return;

	glm::vec3 cameraPos = glm::vec3(glm::inverse(view)[3]);
	glm::mat4 m = glm::translate(glm::mat4(1.0f), cameraPos + moonSkyDir * MOON_DISTANCE);
	m = glm::rotate(m, (float)glfwGetTime() * 0.05f, glm::vec3(0.0f, 1.0f, 0.0f)); // slow spin
	m = glm::scale(m, glm::vec3(MOON_SCALE));
	m = glm::translate(m, -MOON_MODEL_CENTER);

	moonShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(moonShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(m));
	glUniformMatrix4fv(glGetUniformLocation(moonShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(moonShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniform3fv(glGetUniformLocation(moonShader.shaderProgram, "sunDirWorld"), 1, glm::value_ptr(sunSkyDir));
	glUniform1f(glGetUniformLocation(moonShader.shaderProgram, "moonAlpha"), alpha);
	glUniform3fv(glGetUniformLocation(moonShader.shaderProgram, "cameraPosWorld"), 1, glm::value_ptr(cameraPos));
	setCloudUniforms(moonShader.shaderProgram);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	moon.Draw(moonShader);
	glDisable(GL_BLEND);
}

// Places the planets and the star field for the current date and time.
// In the sky's own (sun-locked) frame the sun sits at sunDir0 = (2, 0, 1); the ecliptic is the
// great circle through it, with "east" (towards the evening sky) along (0, -1, 0).
// skyTurn maps that frame to world space; daylight is 1 at day, 0 at night.
void updatePlanets(const glm::mat3& skyTurn, float daylight)
{
	const glm::vec3 sunDir0 = glm::normalize(glm::vec3(2.0f, 0.0f, 1.0f));
	const glm::vec3 east = glm::vec3(0.0f, -1.0f, 0.0f);
	const glm::vec3 eclipticPole = glm::cross(sunDir0, east);

	double days = PLANET_START_DAY + dayCount + timeOfDay;
	gps::PlanetSighting sightings[gps::PLANET_COUNT];
	double sunLongitude = gps::computePlanets(days, sightings);

	// Stars keep fixed ecliptic longitudes, so in the sun-locked frame they slide back by the
	// sun's longitude; mapping world -> star sphere undoes that.
	glm::mat3 yearTurn = glm::mat3(glm::rotate(glm::mat4(1.0f), (float)glm::radians(sunLongitude), eclipticPole));
	starRotation = yearTurn * glm::transpose(skyTurn);

	// Faintest magnitude visible: about -5 in daylight (nothing), +6 at full night.
	float night = glm::smoothstep(0.5f, 1.0f, 1.0f - daylight);
	float limitingMagnitude = glm::mix(-5.0f, 6.0f, night);

	for (int k = 0; k < gps::PLANET_COUNT; k++) {
		float e = glm::radians(sightings[k].elongationDeg);
		float b = glm::radians(sightings[k].latitudeDeg);
		glm::vec3 local = cosf(b) * (cosf(e) * sunDir0 + sinf(e) * east) + sinf(b) * eclipticPole;
		planetDirs[k] = skyTurn * local;
		planetColors[k] = gps::PLANETS[k].color;

		float m = sightings[k].magnitude;
		float twilightFade = glm::clamp((limitingMagnitude - m) / 1.5f, 0.0f, 1.0f);
		float glareFade = glm::smoothstep(10.0f, 18.0f, fabsf(sightings[k].elongationDeg)); // lost in the sun's glare
		float brightness = glm::clamp(0.55f + 0.22f * (0.5f - m), 0.3f, 1.7f); // magnitude -> display
		planetBrightness[k] = brightness * twilightFade * glareFade * (1.0f - overcast);
	}
}

static float randomRange(float lo, float hi)
{
	return lo + (hi - lo) * (rand() / (float)RAND_MAX);
}

// Adds a meteor somewhere in the upper sky, heading mostly downwards like a real one.
void spawnMeteor()
{
	if ((int)meteors.size() >= MAX_METEORS)
		meteors.erase(meteors.begin()); // replace the oldest

	// Appear near where the camera is looking (so a key press is visible), a bit above the view
	// centre, then fall down through the view.
	Meteor m;
	glm::vec3 forward = -glm::vec3(view[0][2], view[1][2], view[2][2]);
	float lookAzimuth = atan2f(forward.z, forward.x);
	float lookElevation = asinf(glm::clamp(forward.y, -1.0f, 1.0f));
	float azimuth = lookAzimuth + glm::radians(randomRange(-35.0f, 35.0f));
	float elevation = glm::clamp(lookElevation + glm::radians(randomRange(5.0f, 25.0f)), glm::radians(15.0f), glm::radians(80.0f));
	m.start = glm::vec3(cosf(elevation) * cosf(azimuth), sinf(elevation), cosf(elevation) * sinf(azimuth));

	// Pick a direction of travel in the sky at that point, tilted towards the horizon.
	glm::vec3 side = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), m.start));
	glm::vec3 down = glm::cross(m.start, side);
	if (down.y > 0.0f)
		down = -down;
	float slant = randomRange(-1.2f, 1.2f); // radians away from straight down
	m.tangent = glm::normalize(cosf(slant) * down + sinf(slant) * side);

	m.spawnTime = glfwGetTime();
	m.duration = randomRange(0.5f, 1.1f);
	m.arc = randomRange(0.25f, 0.5f);
	m.tail = randomRange(0.08f, 0.16f);
	m.fireball = randomRange(0.0f, 1.0f) < 0.12f ? 1.0f : 0.0f; // about 1 in 8 is a fireball
	m.brightness = m.fireball > 0.0f ? randomRange(1.8f, 2.6f) : randomRange(0.8f, 1.4f);
	meteors.push_back(m);
}

// Uploads the live meteors to the bound skybox program and drops the finished ones.
void updateMeteors(GLuint program, float visibility)
{
	double now = glfwGetTime();
	for (size_t i = 0; i < meteors.size();) {
		if (now - meteors[i].spawnTime > meteors[i].duration)
			meteors.erase(meteors.begin() + i);
		else
			i++;
	}

	glm::vec3 starts[MAX_METEORS], tangents[MAX_METEORS];
	glm::vec4 states[MAX_METEORS];
	int n = (int)meteors.size();
	for (int i = 0; i < n; i++) {
		const Meteor& m = meteors[i];
		float t = (float)(now - m.spawnTime) / m.duration; // 0..1
		float envelope = powf(sinf(glm::pi<float>() * t), 0.6f); // flare up, then burn out
		starts[i] = m.start;
		tangents[i] = m.tangent;
		states[i] = glm::vec4(m.arc * t, glm::min(m.tail, m.arc * t + 0.01f), m.brightness * envelope * visibility, m.fireball);
	}
	glUniform1i(glGetUniformLocation(program, "meteorCount"), n);
	if (n > 0) {
		glUniform3fv(glGetUniformLocation(program, "meteorStart"), n, glm::value_ptr(starts[0]));
		glUniform3fv(glGetUniformLocation(program, "meteorTangent"), n, glm::value_ptr(tangents[0]));
		glUniform4fv(glGetUniformLocation(program, "meteorState"), n, glm::value_ptr(states[0]));
	}
}

// (Re)creates the puddle reflection target at the window's size.
void ensureReflectionTarget()
{
	if (reflectionFBO != 0 && reflectionWidth == retina_width && reflectionHeight == retina_height)
		return;
	if (reflectionFBO == 0) {
		glGenFramebuffers(1, &reflectionFBO);
		glGenTextures(1, &reflectionTexture);
		glGenRenderbuffers(1, &reflectionDepth);
	}
	reflectionWidth = retina_width;
	reflectionHeight = retina_height;
	glBindTexture(GL_TEXTURE_2D, reflectionTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, reflectionWidth, reflectionHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindRenderbuffer(GL_RENDERBUFFER, reflectionDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, reflectionWidth, reflectionHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, reflectionFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectionTexture, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, reflectionDepth);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		fprintf(stderr, "puddle reflection framebuffer is incomplete\n");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

// Everything the scene shader needs to draw reflective puddles (it must be bound).
void uploadPuddleUniforms()
{
	GLuint p = myCustomShader.shaderProgram;
	glm::vec3 cameraPos = glm::vec3(glm::inverse(view)[3]);
	glUniform1f(glGetUniformLocation(p, "puddleWetness"), puddleWetness);
	glUniform1f(glGetUniformLocation(p, "puddleSurface"), 0.0f);
	glUniform3fv(glGetUniformLocation(p, "skyTint"), 1, glm::value_ptr(skyColor));
	glUniform3fv(glGetUniformLocation(p, "cameraPosWorld"), 1, glm::value_ptr(cameraPos));
	glUniform3fv(glGetUniformLocation(p, "moonDirWorld"), 1, glm::value_ptr(moonSkyDir));
	glUniform1f(glGetUniformLocation(p, "moonVisibility"), glm::clamp(nightFactor * 1.2f, 0.0f, 1.0f));
	glUniform3fv(glGetUniformLocation(p, "cloudLitColor"), 1, glm::value_ptr(cloudLitColor));
	glUniform3fv(glGetUniformLocation(p, "cloudShadowColor"), 1, glm::value_ptr(cloudShadowColor));
	glUniform1f(glGetUniformLocation(p, "time"), (float)glfwGetTime());
	glActiveTexture(GL_TEXTURE0 + SKYBOX_TEXTURE_UNIT);
	glBindTexture(GL_TEXTURE_CUBE_MAP, mySkyBox.GetTextureId());
	glActiveTexture(GL_TEXTURE0 + REFLECTION_TEXTURE_UNIT);
	glBindTexture(GL_TEXTURE_2D, reflectionTexture);
	glActiveTexture(GL_TEXTURE0);
	glUniform1f(glGetUniformLocation(p, "puddlePlaneY"), PUDDLE_PLANE_Y);
	glUniform2f(glGetUniformLocation(p, "screenSize"), (float)retina_width, (float)retina_height);
	glUniform1f(glGetUniformLocation(p, "hasReflection"), puddleWetness > 0.0f ? 1.0f : 0.0f);
}

// Snowflakes drift down around the camera, lit by the current light.
void drawSnow()
{
	if (snowAmount <= 0.0f)
		return;
	glm::vec3 cameraPos = glm::vec3(glm::inverse(view)[3]);
	glm::vec3 velocity = glm::vec3(WIND_VELOCITY.x * 0.6f, -1.4f, WIND_VELOCITY.y * 0.6f);

	GLuint p = snowShader.shaderProgram;
	snowShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(p, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(p, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniform3fv(glGetUniformLocation(p, "cameraPosWorld"), 1, glm::value_ptr(cameraPos));
	glUniform1f(glGetUniformLocation(p, "time"), (float)glfwGetTime());
	glUniform3fv(glGetUniformLocation(p, "snowVelocity"), 1, glm::value_ptr(velocity));
	glUniform3fv(glGetUniformLocation(p, "snowBoxSize"), 1, glm::value_ptr(glm::vec3(36.0f, 24.0f, 36.0f)));
	glUniform1f(glGetUniformLocation(p, "pixelsPerUnitAtOne"), projection[1][1] * 0.5f * (float)retina_height);
	glUniform3fv(glGetUniformLocation(p, "snowColor"), 1, glm::value_ptr(snowColor));
	snow.Draw(snowShader, snowAmount);
}

// Sand grains blown almost horizontally past the camera (rain streaks with a sideways velocity).
void drawSand()
{
	if (sandAmount <= 0.0f)
		return;
	glm::vec3 cameraPos = glm::vec3(glm::inverse(view)[3]);
	glm::vec3 velocity = glm::vec3(WIND_VELOCITY.x * 7.0f, -1.5f, WIND_VELOCITY.y * 7.0f);

	GLuint p = rainShader.shaderProgram;
	rainShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(p, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(p, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniform3fv(glGetUniformLocation(p, "cameraPosWorld"), 1, glm::value_ptr(cameraPos));
	glUniform1f(glGetUniformLocation(p, "time"), (float)glfwGetTime());
	glUniform3fv(glGetUniformLocation(p, "rainVelocity"), 1, glm::value_ptr(velocity));
	glUniform3fv(glGetUniformLocation(p, "rainBoxSize"), 1, glm::value_ptr(glm::vec3(30.0f, 14.0f, 30.0f)));
	glUniform1f(glGetUniformLocation(p, "streakLength"), 0.35f);
	glm::vec3 grainColor = dustColor * 0.6f; // grains are darker than the glowing dust around them
	glUniform3fv(glGetUniformLocation(p, "rainColor"), 1, glm::value_ptr(grainColor));
	glUniform1f(glGetUniformLocation(p, "rainAlpha"), 0.5f);
	sand.Draw(rainShader, sandAmount);
}

// The tornado funnel and the debris whirling around its foot.
void drawTornado()
{
	if (tornadoAmount <= 0.0f)
		return;
	glm::vec3 cameraPos = glm::vec3(glm::inverse(view)[3]);
	float t = (float)glfwGetTime();
	float extent = glm::smoothstep(0.0f, 0.75f, tornadoAmount);  // touches down after the funnel forms
	float width = glm::smoothstep(0.3f, 1.0f, tornadoAmount);    // a thin rope as it forms and dies
	glm::vec3 dustLight = ambientNow * 2.2f + directNow * 0.6f;

	gps::Shader* shaders[2] = { &tornadoShader, &tornadoDebrisShader };
	for (int i = 0; i < 2; i++) {
		GLuint p = shaders[i]->shaderProgram;
		shaders[i]->useShaderProgram();
		glUniformMatrix4fv(glGetUniformLocation(p, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(p, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniform3fv(glGetUniformLocation(p, "tornadoBase"), 1, glm::value_ptr(tornadoBase));
		glUniform1f(glGetUniformLocation(p, "funnelHeight"), TORNADO_FUNNEL_HEIGHT);
		glUniform1f(glGetUniformLocation(p, "tornadoTime"), t);
		glUniform1f(glGetUniformLocation(p, "tornadoWidth"), width);
		glUniform1f(glGetUniformLocation(p, "visibility"), glm::smoothstep(0.0f, 0.3f, tornadoAmount));
	}

	GLuint f = tornadoShader.shaderProgram;
	tornadoShader.useShaderProgram();
	glUniform3fv(glGetUniformLocation(f, "cameraPosWorld"), 1, glm::value_ptr(cameraPos));
	glUniform3fv(glGetUniformLocation(f, "lightDirWorld"), 1, glm::value_ptr(glm::normalize(currentLightDir)));
	glUniform3fv(glGetUniformLocation(f, "lightColor"), 1, glm::value_ptr(directNow));
	glUniform3fv(glGetUniformLocation(f, "ambientLight"), 1, glm::value_ptr(ambientNow));
	glUniform3fv(glGetUniformLocation(f, "fogColor"), 1, glm::value_ptr(fogColorNow));
	glUniform1f(glGetUniformLocation(f, "extent"), extent);
	glUniform1f(glGetUniformLocation(f, "shellScale"), 1.0f);
	tornado.DrawFunnel(tornadoShader);
	glUniform1f(glGetUniformLocation(f, "shellScale"), 1.45f);
	tornado.DrawFunnel(tornadoShader);

	GLuint d = tornadoDebrisShader.shaderProgram;
	tornadoDebrisShader.useShaderProgram();
	glUniform1f(glGetUniformLocation(d, "pixelsPerUnitAtOne"), projection[1][1] * 0.5f * (float)retina_height);
	glUniform3fv(glGetUniformLocation(d, "dustLight"), 1, glm::value_ptr(glm::min(dustLight, glm::vec3(1.2f))));
	// Debris only once the funnel has reached the ground.
	glUniform1f(glGetUniformLocation(d, "visibility"), glm::smoothstep(0.6f, 0.9f, tornadoAmount));
	tornado.DrawDebris(tornadoDebrisShader);
}

// Fireflies come out after dusk and hide when it rains.
void drawFireflies()
{
	float visibility = glm::smoothstep(0.5f, 0.9f, nightFactor) * (1.0f - overcast);
	if (visibility <= 0.01f)
		return;
	GLuint p = firefliesShader.shaderProgram;
	firefliesShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(p, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(p, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniform3fv(glGetUniformLocation(p, "areaMin"), 1, glm::value_ptr(fireflies.AreaMin()));
	glUniform3fv(glGetUniformLocation(p, "areaSize"), 1, glm::value_ptr(fireflies.AreaSize()));
	glUniform1f(glGetUniformLocation(p, "time"), (float)glfwGetTime());
	glUniform1f(glGetUniformLocation(p, "pixelsPerUnitAtOne"), projection[1][1] * 0.5f * (float)retina_height);
	fireflies.Draw(firefliesShader, visibility);
}

// A bolt somewhere ahead of the camera (uses the previous frame's view, which is fine).
void strikeLightning()
{
	glm::mat4 cameraToWorld = glm::inverse(view);
	glm::vec3 cameraPos = glm::vec3(cameraToWorld[3]);
	glm::vec3 forward = -glm::vec3(cameraToWorld[2]);
	lightning.Strike(cameraPos, forward, glfwGetTime());
}

// Rain streaks around the camera, drawn last because they are transparent.
void drawRain()
{
	if (rainAmount <= 0.0f)
		return;
	glm::vec3 cameraPos = glm::vec3(glm::inverse(view)[3]);
	glm::vec3 velocity = glm::vec3(WIND_VELOCITY.x * 1.5f, -16.0f, WIND_VELOCITY.y * 1.5f);

	GLuint p = rainShader.shaderProgram;
	rainShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(p, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(p, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniform3fv(glGetUniformLocation(p, "cameraPosWorld"), 1, glm::value_ptr(cameraPos));
	glUniform1f(glGetUniformLocation(p, "time"), (float)glfwGetTime());
	glUniform3fv(glGetUniformLocation(p, "rainVelocity"), 1, glm::value_ptr(velocity));
	glUniform3fv(glGetUniformLocation(p, "rainBoxSize"), 1, glm::value_ptr(glm::vec3(40.0f, 30.0f, 40.0f)));
	glUniform1f(glGetUniformLocation(p, "streakLength"), 0.6f);
	glUniform3fv(glGetUniformLocation(p, "rainColor"), 1, glm::value_ptr(rainColor));
	glUniform1f(glGetUniformLocation(p, "rainAlpha"), 0.35f);
	rain.Draw(rainShader, rainAmount);
}

// Cloud uniforms shared by every program that includes clouds.glsl.
void setCloudUniforms(GLuint program)
{
	glUniform2fv(glGetUniformLocation(program, "cloudOffset"), 1, glm::value_ptr(cloudOffset));
	// Rain and snow bring full overcast regardless of the chosen cover.
	glUniform1f(glGetUniformLocation(program, "cloudCoverage"), glm::mix(cloudCoverage, 1.0f, overcast));
}

static glm::vec3 lerp3(const glm::vec3& a, const glm::vec3& b, float t)
{
	return a + (b - a) * glm::clamp(t, 0.0f, 1.0f);
}

// Uploads the campfire light. Needs the current view matrix and the house's model matrix,
// because the fire is part of the house model.
// Uploads the fire's eye-space position for the current view.
void updateFirePosition(const glm::mat4& houseModel)
{
	glm::vec3 posEye = glm::vec3(view * houseModel * glm::vec4(FIRE_POS_MODEL + fireJitter, 1.0f));
	glUniform3fv(fireLightPosEyeLoc, 1, glm::value_ptr(posEye));
}

void updateFireLight(const glm::mat4& houseModel)
{
	float t = (float)glfwGetTime();
	// Layered sines give an irregular flicker without needing a noise function.
	float flicker = 1.0f + 0.12f * sinf(t * 11.0f) + 0.08f * sinf(t * 23.0f + 1.3f) + 0.05f * sinf(t * 47.0f + 0.7f);
	fireJitter = glm::vec3(0.03f * sinf(t * 7.0f), 0.05f * sinf(t * 13.0f + 2.0f), 0.03f * sinf(t * 5.0f + 1.0f));

	// Rain puts the fire out; it comes back only after the rain is completely gone.
	if (rainAmount > 0.2f)
		fireStrength -= frameDt * rainAmount / FIRE_DOUSE_SECONDS;
	else if (rainAmount <= 0.0f)
		fireStrength += frameDt / FIRE_RELIGHT_SECONDS;
	fireStrength = glm::clamp(fireStrength, 0.0f, 1.0f);

	// A fire that is being doused sputters: deep irregular dips, strongest when half out.
	float dousing = rainAmount * 4.0f * fireStrength * (1.0f - fireStrength); // 0..1, peaks at 0.5
	float sputter = 0.5f + 0.5f * sinf(t * 13.0f + 3.0f * sinf(t * 1.7f));
	flicker *= 1.0f - 0.7f * dousing * sputter;
	// Gusts in a sandstorm or near a tornado whip the flames about.
	float gusts = glm::max(sandAmount, tornadoShake);
	flicker *= 1.0f + gusts * (0.25f * sinf(t * 9.0f + 2.0f * sinf(t * 2.3f)) - 0.1f);

	// The fire burns all the time but only stands out once the sun is low.
	float intensity = fireOn ? flicker * fireStrength * (0.35f + 2.65f * nightFactor) : 0.0f;
	updateFirePosition(houseModel);
	// A dying fire is redder: only embers left.
	glm::vec3 color = glm::vec3(1.0f, (0.55f + 0.05f * sinf(t * 17.0f)) * (0.45f + 0.55f * fireStrength), 0.2f * fireStrength);

	glUniform3fv(fireColorLoc, 1, glm::value_ptr(color));
	glUniform1f(fireIntensityLoc, intensity);
}

// Advances the clock and uploads sun/moon direction, light colors, fog and sky tint.
void updateDayNight()
{
	if (dayNightRunning) {
		timeOfDay += frameDt / DAY_LENGTH_SECONDS;
		if (timeOfDay >= 1.0f) {
			timeOfDay -= 1.0f;
			dayCount++;
		}
	}

	// The sun moves on a circle in the XY plane (offset in Z so lookAt never sees a vertical
	// direction); the moon is opposite. K/L still rotate the whole arc around Y.
	float theta = timeOfDay * 2.0f * glm::pi<float>();
	float elevation = sinf(theta); // -1..1, > 0 means the sun is up
	glm::vec3 sunDir = glm::vec3(2.0f * cosf(theta), 2.0f * elevation, 1.0f);
	glm::vec3 moonDir = glm::vec3(-sunDir.x, -sunDir.y, sunDir.z);
	glm::vec3 dir = elevation >= 0.0f ? sunDir : moonDir;
	glm::mat4 azimuth = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	currentLightDir = glm::vec3(azimuth * glm::vec4(dir, 1.0f));
	// sunDir is the fixed vector (2, 0, 1) rotated by theta around Z, then by the azimuth around Y.
	// The inverse of that rotation maps a world direction into the star sphere's frame.
	glm::mat4 skyTurn = azimuth * glm::rotate(glm::mat4(1.0f), theta, glm::vec3(0.0f, 0.0f, 1.0f));
	sunSkyDir = glm::normalize(glm::vec3(azimuth * glm::vec4(sunDir, 0.0f)));
	moonSkyDir = glm::normalize(glm::vec3(azimuth * glm::vec4(moonDir, 0.0f)));

	const glm::vec3 sunsetColor(1.0f, 0.55f, 0.3f);
	const glm::vec3 noonColor(1.0f, 0.97f, 0.9f);
	// Moonlight: cool blue, about a third as strong as midday light on screen (stylised; real
	// moonlight is far dimmer, but the eye's night vision makes it read blue and bright).
	const glm::vec3 moonColor(0.32f, 0.42f, 0.78f);
	glm::vec3 directColor;
	if (elevation >= 0.0f)
		directColor = lerp3(sunsetColor, noonColor, elevation / 0.4f) * glm::smoothstep(0.0f, 0.15f, elevation);
	else
		directColor = moonColor * glm::smoothstep(0.0f, 0.25f, -elevation);
	moonGlow = glm::smoothstep(0.0f, 0.25f, -elevation);

	// 1 at day, 0 at night, with a short twilight around the horizon.
	float daylight = glm::smoothstep(-0.15f, 0.2f, elevation);
	nightFactor = 1.0f - daylight;
	updatePlanets(glm::mat3(skyTurn), daylight);
	float twilight = 1.0f - glm::smoothstep(0.0f, 0.3f, fabsf(elevation));
	// Night ambient is bluer and a little brighter while the moon is up (sky scattering its light).
	glm::vec3 nightAmbient = lerp3(glm::vec3(0.03f, 0.035f, 0.06f), glm::vec3(0.05f, 0.07f, 0.14f), moonGlow);
	glm::vec3 ambient = lerp3(nightAmbient, glm::vec3(0.2f), daylight);
	skyColor = lerp3(glm::vec3(0.08f, 0.1f, 0.22f), glm::vec3(1.0f), daylight);
	skyColor = lerp3(skyColor, skyColor * glm::vec3(1.0f, 0.6f, 0.45f), twilight);

	// Rain: the sun or moon is mostly blocked, its halo disappears and the sky turns grey.
	float rainTarget = raining ? 1.0f : 0.0f;
	float rainStep = frameDt / RAIN_FADE_SECONDS;
	rainAmount += glm::clamp(rainTarget - rainAmount, -rainStep, rainStep);
	directColor *= 1.0f - 0.65f * rainAmount;
	moonGlow *= 1.0f - rainAmount;
	float skyGrey = glm::dot(skyColor, glm::vec3(0.3f, 0.5f, 0.2f)) * 0.6f;
	skyColor = lerp3(skyColor, glm::vec3(skyGrey), rainAmount * 0.8f);
	rainColor = glm::min(ambient * 1.5f + directColor * 0.4f + glm::vec3(0.1f), glm::vec3(1.0f));

	// Snow: a softer, brighter overcast than rain. Falling snow lights up a bit (it scatters light).
	float snowTarget = snowing ? 1.0f : 0.0f;
	snowAmount += glm::clamp(snowTarget - snowAmount, -rainStep, rainStep);
	float sandTarget = sandstorm ? 1.0f : 0.0f;
	float sandStep = frameDt / SAND_FADE_SECONDS;
	sandAmount += glm::clamp(sandTarget - sandAmount, -sandStep, sandStep);
	float tornadoStep = frameDt / TORNADO_FORM_SECONDS;
	tornadoAmount += glm::clamp((tornadoOn ? 1.0f : 0.0f) - tornadoAmount, -tornadoStep, tornadoStep);
	overcast = glm::max(glm::max(rainAmount, snowAmount), glm::max(sandAmount, tornadoAmount));
	directColor *= 1.0f - 0.5f * snowAmount;
	moonGlow *= 1.0f - snowAmount;
	float snowGrey = glm::dot(skyColor, glm::vec3(0.3f, 0.5f, 0.2f)) * 0.85f;
	skyColor = lerp3(skyColor, glm::vec3(snowGrey) * glm::vec3(0.95f, 0.97f, 1.0f), snowAmount * 0.8f);
	ambient += glm::vec3(0.04f, 0.045f, 0.05f) * snowAmount * daylight;
	snowColor = glm::min(ambient * 2.5f + directColor * 0.8f + glm::vec3(0.12f), glm::vec3(1.0f));

	// Sandstorm: the dust takes the colour of the light passing through it (ochre by day, rust
	// at sunset, dark brown at night). Sunlight is dimmed and reddened, the rest turns to haze.
	glm::vec3 dustLight = ambient * 2.2f + directColor * 0.75f;
	dustColor = glm::min(dustLight * glm::vec3(0.78f, 0.5f, 0.27f) + glm::vec3(0.03f, 0.02f, 0.01f), glm::vec3(0.85f, 0.6f, 0.35f));
	directColor = lerp3(directColor, directColor * glm::vec3(0.4f, 0.27f, 0.15f), sandAmount);
	ambient = lerp3(ambient, glm::max(ambient, dustColor * 0.45f), sandAmount);
	moonGlow *= 1.0f - sandAmount;
	skyColor = lerp3(skyColor, dustColor, sandAmount * 0.6f);
	dustOffset += WIND_VELOCITY * frameDt * (2.0f + 4.0f * sandAmount);
	puddleWetness -= frameDt * sandAmount / 20.0f; // the hot wind dries them

	// Tornado weather: a dark supercell sky with a sickly green tint, and dim light.
	directColor *= 1.0f - 0.45f * tornadoAmount;
	moonGlow *= 1.0f - tornadoAmount;
	float stormGrey = glm::dot(skyColor, glm::vec3(0.3f, 0.5f, 0.2f)) * 0.45f;
	skyColor = lerp3(skyColor, glm::vec3(stormGrey) * glm::vec3(0.82f, 1.0f, 0.85f), tornadoAmount * 0.85f);

	// It wanders on a slow looping path over the canyon floor that brings it through the camp.
	float tt = (float)(glfwGetTime() - tornadoSpawnTime) * 0.08f + tornadoPhase;
	tornadoBase = glm::vec3(15.0f * sinf(tt), -0.5f, 12.0f * sinf(tt * 0.73f + 1.3f));
	glm::vec3 cameraPosNow = myCamera.getCameraPosition();
	float tornadoDistance = glm::length(glm::vec2(cameraPosNow.x - tornadoBase.x, cameraPosNow.z - tornadoBase.z));
	tornadoShake = tornadoAmount * (1.0f - glm::smoothstep(8.0f, 45.0f, tornadoDistance));

	if (snowAmount > 0.2f)
		snowCover += frameDt * snowAmount / SNOW_COVER_SECONDS;
	else
		snowCover -= frameDt * (0.03f + 0.97f * daylight) / SNOW_MELT_SECONDS + frameDt * rainAmount / 20.0f;
	snowCover = glm::clamp(snowCover, 0.0f, 1.0f);

	if (rainAmount > 0.2f)
		puddleWetness += frameDt * rainAmount / PUDDLE_FILL_SECONDS;
	else if (rainAmount <= 0.0f)
		puddleWetness -= frameDt / PUDDLE_DRY_SECONDS;
	puddleWetness = glm::clamp(puddleWetness, 0.0f, 1.0f);

	// Lightning while it rains hard. Each strike flashes the whole scene and lights the sky.
	double now = glfwGetTime();
	if (rainAmount > 0.8f) {
		if (nextStrikeTime == 0.0)
			nextStrikeTime = now + randomRange(2.0f, 6.0f); // first one soon after the storm builds
		else if (now >= nextStrikeTime) {
			strikeLightning();
			nextStrikeTime = now + randomRange(4.0f, 12.0f);
		}
	}
	else
		nextStrikeTime = 0.0;
	float sinceRain = (float)(now - rainStoppedTime);
	float afterRain = glm::smoothstep(1.0f, 4.0f, sinceRain) *
		(1.0f - glm::smoothstep(RAINBOW_HOLD_SECONDS, RAINBOW_HOLD_SECONDS + 20.0f, sinceRain));
	float sunHeight = sunSkyDir.y; // sin of the sun's elevation; the bow (48 deg) needs it below ~0.74
	float lowSun = glm::smoothstep(0.0f, 0.08f, sunHeight) * (1.0f - glm::smoothstep(0.6f, 0.74f, sunHeight));
	rainbowStrength = 0.55f * afterRain * lowSun * daylight;

	float flash = lightning.Flash(now) * glm::min(rainAmount * 1.5f, 1.0f);
	ambient += glm::vec3(0.45f, 0.5f, 0.65f) * flash;
	skyColor += glm::vec3(0.6f, 0.65f, 0.85f) * flash;
	rainColor = glm::min(rainColor + glm::vec3(0.5f) * flash, glm::vec3(1.0f));

	glm::vec3 fogColor = lerp3(skyColor * 0.5f, dustColor, sandAmount);

	// Clouds take the colour of whatever lights them: white by day, orange at sunset,
	// blue-grey under the moon, near-black silhouettes on a moonless night.
	cloudOffset += WIND_VELOCITY * frameDt * (1.0f + 5.0f * sandAmount + 2.0f * tornadoAmount); // a storm wind
	cloudLitColor = glm::min(ambient * 2.0f + directColor * 0.9f, glm::vec3(1.0f)) * (1.0f - 0.45f * rainAmount - 0.15f * snowAmount);
	cloudShadowColor = (ambient * 1.5f + skyColor * 0.1f) * (1.0f - 0.3f * rainAmount);
	cloudLitColor += glm::vec3(0.5f, 0.55f, 0.75f) * flash; // clouds light up from inside
	cloudShadowColor += glm::vec3(0.3f, 0.33f, 0.45f) * flash;

	myCustomShader.useShaderProgram();
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(currentLightDir));
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(directColor));
	glUniform3fv(ambientLightLoc, 1, glm::value_ptr(ambient));
	glUniform3fv(fogColorLoc, 1, glm::value_ptr(fogColor));
	ambientNow = ambient;
	directNow = directColor;
	fogColorNow = fogColor;
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "rainAmount"), rainAmount);
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "snowAmount"), snowAmount);
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "snowCover"), snowCover);
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "sandAmount"), sandAmount);
	glUniform2fv(glGetUniformLocation(myCustomShader.shaderProgram, "dustOffset"), 1, glm::value_ptr(dustOffset));

	glClearColor(fogColor.r, fogColor.g, fogColor.b, 1.0f);
}

// Draws every model with myCustomShader (bound) using the current global view.
void drawSceneObjects(bool withTerrain)
{
	//create model matrix for nanosuit
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	//send model matrix data to shader	
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//compute normal matrix
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	//myModel.Draw(myCustomShader);

	////create model matrix for ground
	//model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	////send model matrix data to shader
	//glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	////create normal matrix
	//normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	////send normal matrix data to shader
	//glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	//ground.Draw(myCustomShader);

	//create model matrix for tents
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	//send model matrix data to shader
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	//compute normal matrix
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	tents.Draw(myCustomShader);

	//create model matrix for grass
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	//send model matrix data to shader
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	//compute normal matrix
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	// Puddles only form on the terrain, not on roofs or the tank. The terrain is left out of the
	// puddle reflection: it lies under the mirror plane anyway.
	if (withTerrain) {
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "puddleSurface"), 1.0f);
		grass.Draw(myCustomShader);
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "puddleSurface"), 0.0f);
	}


	

	//create model matrix for house
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	//send model matrix data to shader
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	//compute normal matrix
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	house.Draw(myCustomShader);

	
	

	//create model matrix for ufo
	model = glm::rotate(glm::mat4(1.0f), glm::radians(birdAngle), glm::vec3(0, 1, 0));	
	//send model matrix data to shader
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	//compute normal matrix
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	bird.Draw(myCustomShader);
	
	//create model matrix for tank
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	model = glm::translate(model, glm::vec3(0, distanceRocket, 0));
	//send model matrix data to shader
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	//compute normal matrix
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	if (distanceRocket >= 2 && birdAngle >= 330 && birdAngle <= 350) {
		invisibilistyt = true;
	} 

	if (invisibilistyt == true) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
		tank.Draw(myCustomShader);
		glDisable(GL_BLEND);
	}
	else
	{
		tank.Draw(myCustomShader);
	}
	
	//send model matrix data to shader
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	//compute normal matrix
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	//tank.Draw(myCustomShader);

	
	

	

	//create model matrix for farmer
	model = tankModelMatrix();
	//send model matrix data to shader
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	//compute normal matrix
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	farmer.Draw(myCustomShader);

	//create model matrix for wood
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	//send model matrix data to shader
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	//compute normal matrix
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	wood.Draw(myCustomShader);
}

// Puddle reflections: the models drawn mirrored below the camp floor into reflectionFBO.
// The puddle shader samples it at its own screen position; where nothing was drawn
// (alpha 0) it falls back to the sky/cloud/moon reflection.
void renderPuddleReflection(const glm::mat4& houseModel)
{
	ensureReflectionTarget();
	GLuint p = myCustomShader.shaderProgram;
	glm::mat4 cameraView = view;

	// Mirror about the plane y = PUDDLE_PLANE_Y.
	glm::mat4 mirror = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, PUDDLE_PLANE_Y, 0.0f)) *
		glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, 1.0f)) *
		glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -PUDDLE_PLANE_Y, 0.0f));
	view = cameraView * mirror;
	glUniformMatrix4fv(glGetUniformLocation(p, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix3fv(lightDirMatrixLoc, 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::inverseTranspose(view))));
	glm::vec3 fireEye = glm::vec3(view * houseModel * glm::vec4(FIRE_POS_MODEL + fireJitter, 1.0f));
	glUniform3fv(fireLightPosEyeLoc, 1, glm::value_ptr(fireEye));
	// Keep only what is above the floor (a small margin hides the seam where objects touch it).
	glUniform4f(glGetUniformLocation(p, "clipPlane"), 0.0f, 1.0f, 0.0f, -(PUDDLE_PLANE_Y - 0.02f));
	glEnable(GL_CLIP_DISTANCE0);

	glBindFramebuffer(GL_FRAMEBUFFER, reflectionFBO);
	glViewport(0, 0, reflectionWidth, reflectionHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawSceneObjects(false);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glDisable(GL_CLIP_DISTANCE0);

	// Back to the real camera.
	view = cameraView;
	glUniformMatrix4fv(glGetUniformLocation(p, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix3fv(lightDirMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightDirMatrix));
	updateFirePosition(houseModel);
}

void renderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	double now = glfwGetTime();
	frameDt = (float)glm::min(now - lastFrameTime, 0.1); // clamp so a stall doesn't teleport anything
	lastFrameTime = now;

	processMovement();
	updateDayNight();

	//render the scene to the depth buffer (first pass)

	depthMapShader.useShaderProgram();

	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));

	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	//create model matrix for nanosuit
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));



	//create model matrix for ground
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));



	//create model matrix for barn
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));
	tents.Draw(depthMapShader);

	//create model matrix for grass
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));
	grass.Draw(depthMapShader);





	//create model matrix for house
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));
	house.Draw(depthMapShader);



	

	//create model matrix for ufo
	model = glm::rotate(glm::mat4(1.0f), glm::radians(birdAngle), glm::vec3(0, 1, 0));
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));
	bird.Draw(depthMapShader);

	//create model matrix for tank
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));
	if (distanceRocket >= 100 ) {
		invisibilistyt = true;
	}

	if (invisibilistyt == true) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
		tank.Draw(depthMapShader);
		glDisable(GL_BLEND);
	}
	else
	{
		tank.Draw(depthMapShader);
	}

	//create model matrix for goat2
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));
	//goat2.Draw(depthMapShader);

	//create model matrix for goat3
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));
	//goat3.Draw(depthMapShader);

	//create model matrix for the tank (farmer.obj)
	model = tankModelMatrix();
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));
	farmer.Draw(depthMapShader);

	//create model matrix for wood
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));
	wood.Draw(depthMapShader);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//render the scene (second pass)

	myCustomShader.useShaderProgram();

	if (startFog) {
		glUniform3fv(fogLoc, 1, glm::value_ptr(glm::vec3(1.0f, 0.0f, 0.0f)));
	}
	else {
		glUniform3fv(fogLoc, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, 0.0f)));
	}

	//send lightSpace matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));

	//send view matrix to shader
	view = myCamera.getViewMatrix();
	if (tornadoShake > 0.0f) {
		// The ground rumbles and the wind buffets the view when the tornado is close.
		float t = (float)now;
		float yaw = (sinf(t * 31.0f) * 0.6f + sinf(t * 17.3f + 1.0f) * 0.4f) * 0.8f * tornadoShake;
		float pitch = (sinf(t * 27.0f + 2.0f) * 0.6f + sinf(t * 13.7f) * 0.4f) * 0.6f * tornadoShake;
		view = glm::rotate(glm::mat4(1.0f), glm::radians(pitch), glm::vec3(1, 0, 0)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(yaw), glm::vec3(0, 1, 0)) * view;
	}
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "view"),
		1,
		GL_FALSE,
		glm::value_ptr(view));

	//compute light direction transformation matrix
	lightDirMatrix = glm::mat3(glm::inverseTranspose(view));
	//send lightDir matrix data to shader
	glUniformMatrix3fv(lightDirMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightDirMatrix));

	// lamp toggle (H/J); must be sent while this program is bound
	glUniform1i(lightsOnLoc, lightsOn);
	setCloudUniforms(myCustomShader.shaderProgram);

	// same model matrix the house is drawn with below
	glm::mat4 houseModel = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	updateFireLight(houseModel);

	// Shadows are needed by the reflection pass too.
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "puddleSurface"), 0.0f);
	if (puddleWetness > 0.0f)
		renderPuddleReflection(houseModel);
	uploadPuddleUniforms(); // binds the reflection for sampling, now that it is drawn

	glViewport(0, 0, retina_width, retina_height);
	myCustomShader.useShaderProgram();

	//bind the depth map
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);

	drawSceneObjects(true);

	skyboxShader.useShaderProgram();
	glUniform3fv(glGetUniformLocation(skyboxShader.shaderProgram, "skyTint"), 1, glm::value_ptr(skyColor));
	glUniform1f(glGetUniformLocation(skyboxShader.shaderProgram, "starVisibility"), glm::smoothstep(0.3f, 0.9f, nightFactor) * (1.0f - overcast));
	glUniformMatrix3fv(glGetUniformLocation(skyboxShader.shaderProgram, "starRotation"), 1, GL_FALSE, glm::value_ptr(starRotation));
	glUniform1f(glGetUniformLocation(skyboxShader.shaderProgram, "time"), (float)glfwGetTime());
	glUniform3fv(glGetUniformLocation(skyboxShader.shaderProgram, "planetDir"), gps::PLANET_COUNT, glm::value_ptr(planetDirs[0]));
	glUniform3fv(glGetUniformLocation(skyboxShader.shaderProgram, "planetColor"), gps::PLANET_COUNT, glm::value_ptr(planetColors[0]));
	glUniform1fv(glGetUniformLocation(skyboxShader.shaderProgram, "planetBrightness"), gps::PLANET_COUNT, planetBrightness);
	// Meteors are a night sight; by day only a faint trace shows (bright fireballs can be seen in daylight).
	glUniform3fv(glGetUniformLocation(skyboxShader.shaderProgram, "sunDirWorld"), 1, glm::value_ptr(sunSkyDir));
	glUniform1f(glGetUniformLocation(skyboxShader.shaderProgram, "rainbowStrength"), rainbowStrength);
	glUniform1f(glGetUniformLocation(skyboxShader.shaderProgram, "sandAmount"), sandAmount);
	glUniform3fv(glGetUniformLocation(skyboxShader.shaderProgram, "dustColor"), 1, glm::value_ptr(dustColor));
	glUniform2fv(glGetUniformLocation(skyboxShader.shaderProgram, "dustOffset"), 1, glm::value_ptr(dustOffset));
	updateMeteors(skyboxShader.shaderProgram, glm::mix(0.3f, 1.0f, glm::smoothstep(0.3f, 0.9f, nightFactor)));
	glUniform3fv(glGetUniformLocation(skyboxShader.shaderProgram, "moonDirWorld"), 1, glm::value_ptr(moonSkyDir));
	glUniform1f(glGetUniformLocation(skyboxShader.shaderProgram, "moonGlow"), moonGlow * glm::clamp(nightFactor * 1.2f, 0.0f, 1.0f));
	setCloudUniforms(skyboxShader.shaderProgram);
	glm::vec3 cameraPos = glm::vec3(glm::inverse(view)[3]);
	glm::vec3 lightDirWorld = glm::normalize(currentLightDir);
	glUniform3fv(glGetUniformLocation(skyboxShader.shaderProgram, "cameraPosWorld"), 1, glm::value_ptr(cameraPos));
	glUniform3fv(glGetUniformLocation(skyboxShader.shaderProgram, "lightDirWorld"), 1, glm::value_ptr(lightDirWorld));
	glUniform3fv(glGetUniformLocation(skyboxShader.shaderProgram, "cloudLitColor"), 1, glm::value_ptr(cloudLitColor));
	glUniform3fv(glGetUniformLocation(skyboxShader.shaderProgram, "cloudShadowColor"), 1, glm::value_ptr(cloudShadowColor));
	mySkyBox.Draw(skyboxShader, view, projection);
	drawMoon();
	lightning.Draw(lightningShader, view, projection, glm::vec3(glm::inverse(view)[3]), glfwGetTime());
	drawTornado();
	drawFireflies();
	drawRain();
	drawSnow();
	drawSand();

}

int main(int argc, const char * argv[]) {

	initOpenGLWindow();
	initOpenGLState();
	initFBOs();
	initModels();
	initShaders();
	initUniforms();
	glCheckError();

	std::vector<const GLchar*> faces;
	

	faces.push_back("textures/skybox/sahara_ft.tga");
	faces.push_back("textures/skybox/sahara_bk.tga");
	faces.push_back("textures/skybox/sahara_up.tga");
	faces.push_back("textures/skybox/sahara_dn.tga");
	faces.push_back("textures/skybox/sahara_rt.tga");
	faces.push_back("textures/skybox/sahara_lf.tga");

	mySkyBox.Load(faces);
	skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
	skyboxShader.useShaderProgram();
	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "view"), 1, GL_FALSE,
		glm::value_ptr(view));
	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "projection"), 1, GL_FALSE,
		glm::value_ptr(projection));

	GLFWcursor* cursor = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
	glfwSetCursor(glWindow, cursor);

	while (!glfwWindowShouldClose(glWindow)) {
		renderScene();
		glfwSetCursorPosCallback(glWindow, mouseCallback);

		glfwPollEvents();
		glfwSwapBuffers(glWindow);
	}

	

	
	glfwTerminate();

	return 0;
}
