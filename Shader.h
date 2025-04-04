#pragma once

#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
    unsigned int ID;

    void compileShaders(const char* vShaderCode, const char* fShaderCode, const char* gShaderCode, 
        const char* tesConShaderCode, const char* tesEvalShaderCode)
    {
        unsigned int vertex, fragment, geometry, tesControl, tesEval;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        // fragment shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        if (gShaderCode)
        {
            geometry = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(geometry, 1, &gShaderCode, NULL);
            glCompileShader(geometry);
            checkCompileErrors(geometry, "GEOMETRY");
        }

        if (tesConShaderCode)
        {
            tesControl = glCreateShader(GL_TESS_CONTROL_SHADER);
            glShaderSource(tesControl, 1, &tesConShaderCode, NULL);
            glCompileShader(tesControl);
            checkCompileErrors(tesControl, "TESS_CONTROL");
        }

        // Tessellation Evaluation shader (if provided)
        if (tesEvalShaderCode)
        {
            tesEval = glCreateShader(GL_TESS_EVALUATION_SHADER);
            glShaderSource(tesEval, 1, &tesEvalShaderCode, NULL);
            glCompileShader(tesEval);
            checkCompileErrors(tesEval, "TESS_EVALUATION");
        }

        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        if (gShaderCode) glAttachShader(ID, geometry);
        if (tesConShaderCode) glAttachShader(ID, tesControl);
        if (tesEvalShaderCode) glAttachShader(ID, tesEval);

        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if (gShaderCode) glDeleteShader(geometry);
        if (tesConShaderCode) glDeleteShader(tesControl);
        if (tesEvalShaderCode) glDeleteShader(tesEval);
    }

    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr,
        const char* tesConPath = nullptr, const char* tesEvalPath = nullptr)
    {
        compileShaders(vertexPath, fragmentPath, geometryPath, tesConPath, tesEvalPath);
    }
    // activate the shader
    // ------------------------------------------------------------------------
    void use()
    {
        glUseProgram(ID);
    }
    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string& name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }
    // ------------------------------------------------------------------------
    void setInt(const std::string& name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setFloat(const std::string& name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

private:
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void checkCompileErrors(unsigned int shader, std::string type)
    {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};
