#pragma once

constexpr unsigned int SCREEN_WIDTH = 800;
constexpr unsigned int SCREEN_HEIGHT = 600;

//max amount of point lights allowed
constexpr unsigned int MAX_POINT_LIGHTS = 4;

//Starting exposure for the camera
constexpr float DEFAULT_EXPOSURE = 0.5f;

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