#pragma once

constexpr float PI = 3.14159265359f;

constexpr unsigned int SCREEN_WIDTH = 800;
constexpr unsigned int SCREEN_HEIGHT = 600;

//max amount of point lights allowed
constexpr unsigned int MAX_POINT_LIGHTS = 4;

//camera
constexpr float DEFAULT_EXPOSURE = 0.5f;
constexpr float DEFAULT_NEAR = 0.1f;
constexpr float DEFAULT_FAR = 200.0f;

//Tone mapping methods
enum ToneFunction
{
	REINHARD = 0,
	EXPOSURE,
};

//directional light shadow frustum settings
constexpr float D_FRUSTUM_SIZE = 10.0f;
constexpr float D_NEAR_PLANE = 1.0f;
constexpr float D_FAR_PLANE = 10.5f;

//amount off passes of gaussian blur in bloom
constexpr unsigned int BLUR_PASSES = 5;

//sphere vertex generation
constexpr unsigned int SPHERE_X_SEGMENTS = 64;
constexpr unsigned int SPHERE_Y_SEGMENTS = 64;

//cascading shadowmap
constexpr unsigned int NUM_CASCADES = 4;