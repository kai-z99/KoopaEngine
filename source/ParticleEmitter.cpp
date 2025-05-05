#include "../include/ParticleEmitter.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <vector>

#include "Shader.h"
#include "ComputeShader.h"

ParticleEmitter::ParticleEmitter(glm::mat4 model, unsigned int particleCount, double time)
{
    this->model = model;
    this->particleCount = particleCount;
    this->timeLeft = time;
    this->maxLife = 3.0f;

    std::vector<Particle> particles = std::vector<Particle>(particleCount);

    //TODO: Do this setup on the gpu woth compute shader.
    //PARTICLES
    for (auto& p : particles)
    {
        p.positionLife = glm::vec4(0.0f, 0.0f, 0.0f, maxLife * RandomFloat01());
        p.velocityActive = glm::vec4(0.0f, 0.0f, 0.0f, -1.0f);
        //rest will be overwritten in compute shader
    }

    glGenBuffers(1, &this->ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Particle) * this->particleCount, particles.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, this->ssbo); //binding = 1
    //unbind
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glGenVertexArrays(1, &this->vao);
    glBindVertexArray(this->vao);
    glBindBuffer(GL_ARRAY_BUFFER, this->ssbo); //just contains all particles

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, positionLife)); //location = 0
    glVertexAttribDivisor(0, 1); //1 move per instance
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)(offsetof(Particle, positionLife) + sizeof(float) * 3)); //location = 1
    glVertexAttribDivisor(1, 1); //1 move per instance
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)(offsetof(Particle, velocityActive) + sizeof(float) * 3)); //location = 2
    glVertexAttribDivisor(2, 1); //1 move per instance
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}
ParticleEmitter::~ParticleEmitter()
{
    glDeleteVertexArrays(1, &this->vao);
    glDeleteBuffers(1, &this->ssbo);
}


void ParticleEmitter::Update(ComputeShader* particleShader, float dt)
{
    particleShader->use();

    this->timeLeft -= dt;
    glUniform1f(glGetUniformLocation(particleShader->ID, "dt"), dt);
    glUniform1i(glGetUniformLocation(particleShader->ID, "doneEmitting"), this->timeLeft < 0);
    glUniform1f(glGetUniformLocation(particleShader->ID, "maxLife"), this->maxLife);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, this->ssbo); 
    unsigned int groups = (this->particleCount + 1023) / 1024; //ceil(groups / localsize);
    glDispatchCompute(groups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
}

void ParticleEmitter::Render(Shader* shader)
{
    shader->use();
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, false, glm::value_ptr(this->model));
    glBindVertexArray(this->vao);
    glDrawArraysInstanced(GL_POINTS, 0, 1, this->particleCount);
    glBindVertexArray(0);
}

bool ParticleEmitter::DoneEmitting()
{
    return this->timeLeft + (double)this->maxLife < 0;
}
