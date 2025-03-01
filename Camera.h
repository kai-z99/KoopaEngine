#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum Camera_Movement
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN,
};

enum Camera_Mode
{
    FLY,
    FPS,
};

// Default camera values
const float YAW = -90.0f; //start facing -z. If you do euler angles calcs for yaw = -90, pitch = 0, you get front = (0, 0, -1) as desired.                                
const float PITCH = 0.0f;                  //If you do it with yaw = pitch = 0, you get front = (1, 0, 0), facing the x axis.
const float SPEED = 5.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

class Camera
{
public:
    glm::vec3 position; //(in world space)

    //unit vectors----
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    //----------------

    glm::vec3 worldUp; //where is up in world space

    float yaw;
    float pitch;

    float moveSpeed;
    float sensitivity;
    float zoom;

    Camera_Mode mode;

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
        float yaw = YAW,
        float pitch = PITCH,
        float speed = SPEED,
        float sense = SENSITIVITY,
        float zoom = ZOOM)
    {
        this->position = position;
        this->worldUp = up;
        this->yaw = yaw;
        this->pitch = pitch;
        this->moveSpeed = speed;
        this->sensitivity = sense;
        this->zoom = zoom;

        //direction unit vectors ARE CALCULATED IN UpdateCameraVectors()))
        //this->front = front; 
        //this->up = up; 
        //this->right = glm::normalize(glm::cross(this->front, this->worldUp));

        this->mode = FLY;

        // Update direction unit vectors based on the initial yaw and pitch
        this->UpdateCameraVectors();
    }

    glm::mat4 GetViewMatrix() const
    {
        return glm::lookAt(this->position, this->position + this->front, this->up);
    }

    glm::mat4 GetKaisViewMatrix() //note: try creating lookAt(position, target, up) seperatly. Direction = position - target!!! (opposite of front so no need to negate like in this func :)). Then right and up calced from that.
    {
        glm::mat4 alpha =
        {
            glm::vec4(right.x , right.y , right.z , 0.0f),
            glm::vec4(up.x    , up.y    , up.z    , 0.0f),
            glm::vec4(-front.x, -front.y, -front.z, 0.0f), // supposed to be "direction" which points opposite to where its facing. So add negatives.
            glm::vec4(0.0f    , 0.0f    , 0.0f    , 1.0f),

        };
        alpha = glm::transpose(alpha);

        glm::mat4 beta =
        {
            glm::vec4(1.0f, 0.0f, 0.0f, -position.x),
            glm::vec4(0.0f, 1.0f, 0.0f, -position.y),
            glm::vec4(0.0f, 0.0f, 1.0f, -position.z),
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        };
        beta = glm::transpose(beta);

        return alpha * beta;
    }




    //change camera position
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = this->moveSpeed * deltaTime;

        if (direction == FORWARD)
            this->position += this->front * velocity;
        if (direction == BACKWARD)
            this->position -= this->front * velocity;
        if (direction == LEFT)
            this->position -= this->right * velocity;
        if (direction == RIGHT)
            this->position += this->right * velocity;
        if (direction == UP)
            this->position.y += velocity;
        if (direction == DOWN)
            this->position.y -= velocity;

        //If there was terrain, call a function float h = GetTerrainHeight(position.x, position.z)
        //then set position.y = h. (simple case) or position.y = h + eyeOffset....so on
        if (this->mode == FPS) this->position.y = 0;
    }

    //Change the euler angles, therefore changing the front, up, and right vectors.
    void ProcessMouseMove(float xOffset, float yOffset, GLboolean constrainPitch = true)
    {
        xOffset *= this->sensitivity;
        yOffset *= this->sensitivity;

        this->yaw += xOffset;
        this->pitch += yOffset;

        //cant look up and then around ur back
        if (constrainPitch)
        {
            if (this->pitch > 89.0f)
                this->pitch = 89.0f;
            if (this->pitch < -89.0f)
                this->pitch = -89.0f;
        }

        this->UpdateCameraVectors();
    }

    //change zoom
    void ProcessMouseScroll(float yoffset)
    {
        this->zoom -= (float)yoffset;

        if (this->zoom < 1.0f)
            this->zoom = 1.0f;
        if (this->zoom > 45.0f)
            this->zoom = 45.0f;
    }

    void SetMode(Camera_Mode mode)
    {
        this->mode = mode;
    }

private:
    // Updates the camera unit vectors AKA THE UNIT VECTORS: FRONT, UP, RIGHT) based on euler angles.
    void UpdateCameraVectors()
    {
        //FRONT
        glm::vec3 f;

        f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        f.y = sin(glm::radians(pitch));
        f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        this->front = glm::normalize(f); //unit vector

        //RIGHT
        //                   index (forward) x middle(up)    = thumb (right)
        this->right = glm::normalize(glm::cross(this->front, this->worldUp)); //unit vector

        //UP
        //                   index (right) x middle(forward)    = thumb (up)
        this->up = glm::normalize(glm::cross(this->right, this->front)); //unit vector
    }
};