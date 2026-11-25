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
GLuint fireLightPosEyeLoc;
GLuint fireColorLoc;
GLuint fireIntensityLoc;
float nightFactor = 0.0f; // 0 at day, 1 at night

gps::Camera myCamera(glm::vec3(0.0f, 0.125f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f));
GLfloat cameraSpeed = 0.05f;

bool pressedKeys[1024];
GLfloat angle;
GLfloat angleF = 0;
GLfloat birdAngle;
GLfloat lightAngle;
GLfloat distanceRocket = 0;
GLfloat lampAmbient = 0;




gps::Model3D lightCube;
gps::Model3D tents;
gps::Model3D grass;


gps::Model3D house;

gps::Model3D bird;
gps::Model3D tank;

gps::Model3D farmer;
gps::Model3D wood;
gps::Shader myCustomShader;
gps::Shader lightShader;
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

	lightShader.useShaderProgram();

	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

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
	
	lightCube = gps::Model3D("objects/cube/cube.obj", "objects/cube/");
	tents = gps::Model3D("objects/tents/tents.obj", "objects/tents/");
	grass = gps::Model3D("objects/grass/GRASS.obj", "objects/grass/");
	
	house = gps::Model3D("objects/house/house1.obj", "objects/house/");

	bird = gps::Model3D("objects/vultur/vultur.obj", "objects/vultur/");
	tank = gps::Model3D("objects/goat1/goat1.obj", "objects/goat1/");

	farmer = gps::Model3D("objects/farmer/farmer.obj", "objects/farmer/");
	wood = gps::Model3D("objects/wood/wood.obj", "objects/wood/");
	moon = gps::Model3D("objects/moon/moon.obj", "objects/moon/");

	tankBounds = farmer.GetBounds();
	houseBounds = house.GetMeshBounds();
	tentsBounds = tents.GetMeshBounds();
	woodBounds = wood.GetMeshBounds();
	rocketBounds = tank.GetMeshBounds();
}

void initShaders()
{
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
	depthMapShader.loadShader("shaders/simpleDepthMap.vert", "shaders/simpleDepthMap.frag");
	moonShader.loadShader("shaders/moon.vert", "shaders/moon.frag");
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
	pointLightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "pointLightColor");
	glUniform3fv(pointLightColorLoc, 1, glm::value_ptr(glm::vec3(1.0f, 0.8f, 0.5f))); // warm lamp
	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));





}

bool invisibilistyt = false;

// Draws the moon far away along moonSkyDir, centred on the camera so it doesn't move as you walk.
// Drawn after the skybox with alpha blending, faded by nightFactor.
void drawMoon()
{
	float alpha = glm::clamp(nightFactor * 1.2f, 0.0f, 1.0f);
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
		planetBrightness[k] = brightness * twilightFade * glareFade;
	}
}

static glm::vec3 lerp3(const glm::vec3& a, const glm::vec3& b, float t)
{
	return a + (b - a) * glm::clamp(t, 0.0f, 1.0f);
}

// Uploads the campfire light. Needs the current view matrix and the house's model matrix,
// because the fire is part of the house model.
void updateFireLight(const glm::mat4& houseModel)
{
	float t = (float)glfwGetTime();
	// Layered sines give an irregular flicker without needing a noise function.
	float flicker = 1.0f + 0.12f * sinf(t * 11.0f) + 0.08f * sinf(t * 23.0f + 1.3f) + 0.05f * sinf(t * 47.0f + 0.7f);
	glm::vec3 jitter = glm::vec3(0.03f * sinf(t * 7.0f), 0.05f * sinf(t * 13.0f + 2.0f), 0.03f * sinf(t * 5.0f + 1.0f));

	// The fire burns all the time but only stands out once the sun is low.
	float intensity = fireOn ? flicker * (0.35f + 2.65f * nightFactor) : 0.0f;
	glm::vec3 posEye = glm::vec3(view * houseModel * glm::vec4(FIRE_POS_MODEL + jitter, 1.0f));
	glm::vec3 color = glm::vec3(1.0f, 0.55f + 0.05f * sinf(t * 17.0f), 0.2f);

	glUniform3fv(fireLightPosEyeLoc, 1, glm::value_ptr(posEye));
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
	const glm::vec3 moonColor(0.25f, 0.3f, 0.5f);
	glm::vec3 directColor;
	if (elevation >= 0.0f)
		directColor = lerp3(sunsetColor, noonColor, elevation / 0.4f) * glm::smoothstep(0.0f, 0.15f, elevation);
	else
		directColor = moonColor * glm::smoothstep(0.0f, 0.15f, -elevation);

	// 1 at day, 0 at night, with a short twilight around the horizon.
	float daylight = glm::smoothstep(-0.15f, 0.2f, elevation);
	nightFactor = 1.0f - daylight;
	updatePlanets(glm::mat3(skyTurn), daylight);
	float twilight = 1.0f - glm::smoothstep(0.0f, 0.3f, fabsf(elevation));
	glm::vec3 ambient = lerp3(glm::vec3(0.04f, 0.05f, 0.1f), glm::vec3(0.2f), daylight);
	skyColor = lerp3(glm::vec3(0.08f, 0.1f, 0.22f), glm::vec3(1.0f), daylight);
	skyColor = lerp3(skyColor, skyColor * glm::vec3(1.0f, 0.6f, 0.45f), twilight);
	glm::vec3 fogColor = skyColor * 0.5f;

	myCustomShader.useShaderProgram();
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(currentLightDir));
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(directColor));
	glUniform3fv(ambientLightLoc, 1, glm::value_ptr(ambient));
	glUniform3fv(fogColorLoc, 1, glm::value_ptr(fogColor));

	glClearColor(fogColor.r, fogColor.g, fogColor.b, 1.0f);
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

	// same model matrix the house is drawn with below
	updateFireLight(glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0)));

	glViewport(0, 0, retina_width, retina_height);
	myCustomShader.useShaderProgram();

	//bind the depth map
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);

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
	grass.Draw(myCustomShader);


	

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

	//draw a white cube around the light

	lightShader.useShaderProgram();

	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

	model = glm::translate(glm::mat4(1.0f), currentLightDir);
	model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	lightCube.Draw(lightShader);

	skyboxShader.useShaderProgram();
	glUniform3fv(glGetUniformLocation(skyboxShader.shaderProgram, "skyTint"), 1, glm::value_ptr(skyColor));
	glUniform1f(glGetUniformLocation(skyboxShader.shaderProgram, "starVisibility"), glm::smoothstep(0.3f, 0.9f, nightFactor));
	glUniformMatrix3fv(glGetUniformLocation(skyboxShader.shaderProgram, "starRotation"), 1, GL_FALSE, glm::value_ptr(starRotation));
	glUniform1f(glGetUniformLocation(skyboxShader.shaderProgram, "time"), (float)glfwGetTime());
	glUniform3fv(glGetUniformLocation(skyboxShader.shaderProgram, "planetDir"), gps::PLANET_COUNT, glm::value_ptr(planetDirs[0]));
	glUniform3fv(glGetUniformLocation(skyboxShader.shaderProgram, "planetColor"), gps::PLANET_COUNT, glm::value_ptr(planetColors[0]));
	glUniform1fv(glGetUniformLocation(skyboxShader.shaderProgram, "planetBrightness"), gps::PLANET_COUNT, planetBrightness);
	mySkyBox.Draw(skyboxShader, view, projection);
	drawMoon();

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
