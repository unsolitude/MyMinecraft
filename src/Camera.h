#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// 定义摄像机移动方向
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// 默认摄像机参数
const float YAW         = -90.0f;
const float PITCH       =  0.0f;
const float SPEED       =  10.0f;  // 提高基础速度
const float SPRINT_MULTIPLIER = 2.5f; // 冲刺倍率
const float SENSITIVITY =  0.1f;
const float ZOOM        =  45.0f;

// 摄像机类
class Camera
{
public:
    // 摄像机属性
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    
    // 欧拉角
    float Yaw;
    float Pitch;
    
    // 摄像机选项
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    // 构造函数（使用向量）
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), 
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), 
           float yaw = YAW, 
           float pitch = PITCH) 
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)), 
          MovementSpeed(SPEED), 
          MouseSensitivity(SENSITIVITY), 
          Zoom(ZOOM)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // 构造函数（使用标量）
    Camera(float posX, float posY, float posZ, 
           float upX, float upY, float upZ, 
           float yaw, float pitch) 
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)), 
          MovementSpeed(SPEED), 
          MouseSensitivity(SENSITIVITY), 
          Zoom(ZOOM)
    {
        Position = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // 返回使用欧拉角和LookAt矩阵计算的视图矩阵
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // 处理键盘输入
    void ProcessKeyboard(Camera_Movement direction, float deltaTime, bool sprinting = false)
    {
        float speed = sprinting ? MovementSpeed * SPRINT_MULTIPLIER : MovementSpeed;
        float velocity = speed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
    }

    // 处理鼠标移动输入
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;

        // 确保俯仰角不会超出范围
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // 使用更新后的欧拉角更新前、右和上向量
        updateCameraVectors();
    }

    // 处理鼠标滚轮输入（可选，用于缩放）
    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f;
    }

private:
    // 根据摄像机的欧拉角计算前向量
    void updateCameraVectors()
    {
        // 计算新的前向量
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        
        // 重新计算右向量和上向量
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};

#endif
