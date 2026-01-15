#include <glad/glad.h>  // 必须放在 GLFW 之前
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <map>
#include "Shader.h"
#include "Camera.h"
#include "Chunk.h"
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// 摄像机对象（调整位置以便看到整个区块）
Camera camera(glm::vec3(20.0f, 15.0f, 25.0f));

// 时间相关变量
float deltaTime = 0.0f; // 当前帧与上一帧的时间差
float lastFrame = 0.0f; // 上一帧的时间

// 鼠标相关变量
float lastX = 400.0f;
float lastY = 300.0f;
bool firstMouse = true;

// 窗口大小变化时的回调函数
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// 鼠标移动回调函数
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    // 检测Shift键是否按下（冲刺）
    bool sprinting = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ||
                     (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime, sprinting);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime, sprinting);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime, sprinting);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime, sprinting);
}

int main() {
    // 初始化 GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 创建窗口对象
    GLFWwindow* window = glfwCreateWindow(800, 600, "Test", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    // 设置鼠标输入模式：隐藏光标并捕获它
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    // 初始化 GLAD (加载 OpenGL 函数指针)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // 创建着色器程序
    Shader ourShader("../src/shader.vs", "../src/shader.fs");

    // 定义渲染范围（区块数量）
    const int RENDER_DISTANCE = 8; // 渲染距离，生成 (2*8+1)^2 = 289 个区块

    // 创建区块容器：使用 map 存储每个区块（按坐标索引）
    std::map<std::pair<int, int>, Chunk*> chunks;
    
    // 预生成所有区块
    for (int cx = -RENDER_DISTANCE; cx <= RENDER_DISTANCE; cx++) {
        for (int cz = -RENDER_DISTANCE; cz <= RENDER_DISTANCE; cz++) {
            Chunk* chunk = new Chunk();
            chunk->initData(cx, cz);  // 使用区块坐标生成地形
            chunk->updateMesh();
            chunks[{cx, cz}] = chunk;
        }
    }
    std::cout << "Generated " << chunks.size() << " chunks with Perlin noise terrain" << std::endl;

    // 加载纹理图集 (Texture Atlas)
    unsigned int textureAtlas;
    glGenTextures(1, &textureAtlas);
    glBindTexture(GL_TEXTURE_2D, textureAtlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load("../assets/texture_atlas.png", &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture atlas" << std::endl;
    }
    stbi_image_free(data);

    // 设置着色器中的纹理单元
    ourShader.use();
    glUniform1i(glGetUniformLocation(ourShader.ID, "textureAtlas"), 0);

    // 启用深度测试
    glEnable(GL_DEPTH_TEST);

    // 渲染循环
    while (!glfwWindowShouldClose(window)) {

        // 计算deltaTime
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        glClearColor(0.5f, 0.7f, 1.0f, 1.0f); // 天空蓝色背景
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // 使用Camera类获取视图矩阵
        glm::mat4 view = camera.GetViewMatrix();
        
        // 创建透视投影矩阵（增加远裁剪面以看到更远的区块）
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 800.0f / 600.0f, 0.1f, 500.0f);
        
        // 绑定纹理图集（仅需绑定一次）
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureAtlas);
        
        // 设置着色器
        ourShader.use();
        unsigned int viewLoc = glGetUniformLocation(ourShader.ID, "view");
        unsigned int projectionLoc = glGetUniformLocation(ourShader.ID, "projection");
        unsigned int modelLoc = glGetUniformLocation(ourShader.ID, "model");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        // 绘制多个区块，形成无限延伸的效果
        for (int cx = -RENDER_DISTANCE; cx <= RENDER_DISTANCE; cx++) {
            for (int cz = -RENDER_DISTANCE; cz <= RENDER_DISTANCE; cz++) {
                // 创建模型矩阵：平移到对应的区块位置
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(cx * 16.0f, 0.0f, cz * 16.0f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                
                // 渲染对应坐标的区块
                auto it = chunks.find({cx, cz});
                if (it != chunks.end()) {
                    it->second->render();
                }
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

    }

    // 释放区块资源
    for (auto& pair : chunks) {
        delete pair.second;
    }
    chunks.clear();

    glfwTerminate();
    return 0;
}