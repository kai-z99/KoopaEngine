#include "../include/ParticleEmitter.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <vector>

#include "helpers.h"
#include "Shader.h"
#include "ComputeShader.h"

ParticleEmitter::ParticleEmitter(glm::mat4 model, unsigned int particleCount)
{
    this->model = model;
    this->particleCount = particleCount;

    this->lastUpdate = glfwGetTime();

    std::vector<Particle> particles = std::vector<Particle>(particleCount);

    //PARTICLES
    for (auto& p : particles)
    {
        p.positionLife = glm::vec4(0.0f, 0.0f, 0.0f, 3.0f * RandomFloat01());
        p.velocity = glm::vec4(0.0f);
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

    glBindVertexArray(0);

}

void ParticleEmitter::Update(ComputeShader* particleShader)
{
    particleShader->use();
    double now = glfwGetTime();
    float dt = float(now - this->lastUpdate);
    this->lastUpdate = now;

    glUniform1f(glGetUniformLocation(particleShader->ID, "dt"), dt);

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