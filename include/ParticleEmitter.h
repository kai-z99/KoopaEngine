#pragma once

#include <glm/glm.hpp>

#include "helpers.h"

class Shader;
class ComputeShader;

class ParticleEmitter
{
public:
	ParticleEmitter(glm::mat4 model, unsigned int particleCount, double time);
	~ParticleEmitter();

	void Update(ComputeShader* particleShader, float dt);
	void Render(Shader* shader);

	bool DoneEmitting();
private:
	glm::mat4 model;
	unsigned int particleCount;
	unsigned int ssbo;
	unsigned int vao;
	double timeLeft;

	float maxLife;

	struct Particle
	{
		glm::vec4 positionLife;		  //16
		glm::vec4 velocityActive;     //16 = 32
	};
};