#pragma once

#include <glm/glm.hpp>

class Shader;
class ComputeShader;

class ParticleEmitter
{
public:
	ParticleEmitter(glm::mat4 model, unsigned int particleCount);

	void Update(ComputeShader* particleShader);
	void Render(Shader* shader);

private:
	glm::mat4 model;
	unsigned int particleCount;
	unsigned int ssbo;
	unsigned int vao;

	double lastUpdate;

	struct Particle
	{
		glm::vec4 positionLife; //16
		glm::vec4 velocity;     //16 = 32
	};
};