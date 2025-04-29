#pragma once

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <meshoptimizer/meshoptimizer.h>

#include "Shader.h"
#include "Definitions.h"
#include "Constants.h"

#include <string>
#include <vector>
#include <optional>
using namespace std;

#define MAX_BONE_INFLUENCE 4

struct Vertex {
    // position
    glm::vec3 Position;
    // normal
    glm::vec3 Normal;
    // texCoords
    glm::vec2 TexCoords;
    // tangent
    glm::vec3 Tangent;
    // bitangent
    glm::vec3 Bitangent;
    //bone indexes which will influence this vertex
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    //weights from each bone
    float m_Weights[MAX_BONE_INFLUENCE];
};

struct Texture {
    unsigned int id;
    string type;
    string path;
};

class ModelMesh {
public:
    // mesh Data
    vector<Vertex>       vertices;
    vector<unsigned int> indices;
    vector<Texture>      textures;
    unsigned int VAO;
    AABB aabb;
    std::optional<MeshData> lodMeshData;

    // constructor
    ModelMesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;

        this->calculateAABB();
            
        // now that we have all the required data, set the vertex buffers and its attribute pointers.
        setupMesh();
    }

    Material GetMaterial()
    {
        Material m = Material();
        m.baseColor = {0,0.25,0};
        m.baseSpecular = 0.3f;

        

        for (const Texture& t : this->textures)
        {
            const std::string& type = t.type;

            if (type == "texture_diffuse")
            {
                m.useDiffuseMap = true;
                m.diffuse = t.id;
                GLint bits;
                glGetTextureLevelParameteriv(t.id, 0, GL_TEXTURE_ALPHA_SIZE, &bits);
                m.hasAlpha = bits > 0;
            }
            else if (type == "texture_normal")
            {
                m.useNormalMap = true;
                m.normal = t.id;
            }
            else if (type == "texture_specular")
            {
                m.useSpecularMap = true;
                m.specular = t.id;
            }
        }

        

        return m;
    }

    MeshData GetMeshData()
    {
        MeshData m = MeshData();

        m.VAO = this->VAO;
        m.indexCount = static_cast<unsigned int>(this->indices.size());
        m.aabb = this->aabb;

        return m;
    }

    void CreateLOD(float triangleRatio, float targetError, unsigned int options)
    {
        if (this->lodMeshData.has_value())
        {
            return;
        }
        else
        {
            if (this->vertices.size() <= MINIMUM_VERTEX_COUNT_FOR_LOD || triangleRatio >= 1.f)
            {
                //std::cout << "too little vertices or ratio > 1.\n";
                return;
            }
            
            //SIMPLIFY INDEX BUFFER---------------------------------------------------------
            const size_t ic = this->indices.size();
            const size_t vc = this->vertices.size();

            const size_t targetIndexCount = std::max<size_t>(3, static_cast<size_t>(ic * triangleRatio) / 3 * 3);

            //std::cout << "Target index count: " << targetIndexCount;

            std::vector<unsigned int> lodIndices(ic); //worst case size is og size
            /*
            size_t written = meshopt_simplify( //return size of new indices
                lodIndices.data(),
                this->indices.data(),
                ic,
                &this->vertices[0].Position.x, //expects float* not Vertex* ()
                vc,
                sizeof(Vertex),
                targetIndexCount,
                targetError,
                options,
                nullptr
                );
            */

            size_t written = meshopt_simplifySloppy( // Use the sloppy version
                lodIndices.data(),
                this->indices.data(),
                ic,
                &this->vertices[0].Position.x,
                vc,
                sizeof(Vertex),
                targetIndexCount, // Target index count (error is ignored)
                targetError, // This parameter is not used by sloppy version
                nullptr // result error pointer (optional, maybe useful?)
            );

            lodIndices.resize(written);
            lodIndices.shrink_to_fit();

            //REMAP/SHIRNK VERTEX BUFFER---------------------------------------------------------
            std::vector<unsigned int> remap(vc); //remap[oldVertexID] = newVertexID
            size_t lodVertexCount = meshopt_generateVertexRemap(
                remap.data(),
                lodIndices.data(),
                written,
                this->vertices.data(), //Vertex is correct because we supply the stride
                vc,
                sizeof(Vertex)         
            );

            std::vector<Vertex> lodVertices(lodVertexCount); 
            meshopt_remapVertexBuffer(lodVertices.data(), vertices.data(), vc,
                sizeof(Vertex), remap.data());
            meshopt_remapIndexBuffer(lodIndices.data(), lodIndices.data(), written,
                remap.data());

            //BUILD--------------------
            unsigned int lodVAO, lodVBO, lodEBO;
            glGenVertexArrays(1, &lodVAO);
            glGenBuffers(1, &lodVBO);
            glGenBuffers(1, &lodEBO);

            glBindVertexArray(lodVAO);

            glBindBuffer(GL_ARRAY_BUFFER, lodVBO);
            glBufferData(GL_ARRAY_BUFFER, lodVertices.size() * sizeof(Vertex), lodVertices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lodEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, lodIndices.size() * sizeof(unsigned int), lodIndices.data(), GL_STATIC_DRAW);
            
            glEnableVertexAttribArray(0); //pos
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

            glEnableVertexAttribArray(1); //norm
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

            glEnableVertexAttribArray(2); //tex
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

            glEnableVertexAttribArray(3); //tan
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));

            glEnableVertexAttribArray(4); //btan
            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

            glEnableVertexAttribArray(5);
            glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));

            glEnableVertexAttribArray(6);
            glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));

            glBindVertexArray(0);

            //CREATE MESHDATA----------------
            MeshData m = MeshData();
            m.VAO = lodVAO;
            m.indexCount = static_cast<unsigned int>(lodIndices.size());
            m.aabb = this->aabb;
            
           // std::cout << "SUCCESS: old indexCount: " << ic << " New indexCount: " << m.indexCount << '\n';

            this->lodMeshData = m;
        }
    }

private:
    //cache
    

    // render data 
    unsigned int VBO, EBO;

    // initializes all the buffer objects/arrays
    void setupMesh()
    {
        // create buffers/arrays
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        // load data into vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        //index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // set the vertex attribute pointers
        // vertex Positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // vertex texture coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        // vertex tangent
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        // vertex bitangent
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
        // ids
        glEnableVertexAttribArray(5);
        glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));

        // weights
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));
        glBindVertexArray(0);
    }

    void calculateAABB()
    {
        if (this->vertices.empty())
        {
            aabb.min = Vec3(0.0f, 0.0f, 0.0f);
            aabb.max = Vec3(0.0f, 0.0f, 0.0f);
            return;
        }

        const glm::vec3& firstPos = vertices[0].Position;
        aabb.min = Vec3(firstPos.x, firstPos.y, firstPos.z);
        aabb.max = Vec3(firstPos.x, firstPos.y, firstPos.z);

        for (size_t i = 1; i < vertices.size(); ++i) {
            const glm::vec3& currentPosGLM = vertices[i].Position;

            Vec3 currentPos = Vec3(currentPosGLM.x, currentPosGLM.y, currentPosGLM.z);
            aabb.expand(currentPos); 
        }
    }
};

//REMOVE THIS, WE WANT TO DRAW FROM DrawCall objects
// render the mesh
/*
void Draw(Shader& shader)
{
    // bind appropriate textures
    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;
    unsigned int normalNr = 1;
    unsigned int heightNr = 1;
    for (unsigned int i = 0; i < textures.size(); i++)
    {
        glActiveTexture(GL_TEXTURE5 + i); // active proper texture unit before binding

        if (GL_TEXTURE5 + i >= GL_TEXTURE9)
        {
            std::cout << "WARNING: TOO MANY TEXTURES IN MODEL!\n";
            break;
        }

        // retrieve texture number (the N in diffuse_textureN)
        string number;
        string name = textures[i].type;
        if (name == "texture_diffuse") //assume all models have diffuse map
            number = std::to_string(diffuseNr++);
        else if (name == "texture_specular")
            number = std::to_string(specularNr++); // transfer unsigned int to string
        else if (name == "texture_normal")
        {
            number = std::to_string(normalNr++); // transfer unsigned int to string
        }
        else if (name == "texture_height")
            number = std::to_string(heightNr++); // transfer unsigned int to string

        // now set the sampler to the correct texture unit
        glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), 5 + i);
        // and finally bind the texture
        glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }

    // draw mesh
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // always good practice to set everything back to defaults once configured.
    glActiveTexture(GL_TEXTURE0);
}
*/
