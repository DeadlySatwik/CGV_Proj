#include <GL/glew.h>
#include <GLFW/glfw3.h>
// Include GLM and external headers
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

#include "Shader.h"
#include "Model.h"

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Car Structure
struct Car {
    int pathType; // 0 = straight underpass, 1 = curved bridge
    float t;      // progress 0.0 to 1.0
    float speed;
    glm::vec3 color;
    glm::vec3 position;
    float angle;

    void update(float deltaTime) {
        t += speed * deltaTime;
        if (t > 1.0f) t -= 1.0f;

        glm::vec3 oldPos = position;

        if (pathType == 0) {
            // Straight underpass: Right lane, moving +Z
            // The road visually covers X = -2.5 to X = 2.5 (width 5.0)
            // Right lane offset is approx +1.0 from center
            position = glm::vec3(1.2f, 0.55f, -18.0f * (1.0f - t) + 18.0f * t);
        } else if (pathType == 1) {
            // Straight underpass in opposite direction: Left lane, moving -Z
            position = glm::vec3(-1.2f, 0.55f, 18.0f * (1.0f - t) - 18.0f * t);
        } else if (pathType == 2) {
            // Bridge 1 (top bridge): explicit elevated left->right crossing over the top deck.
            float s = std::max(0.0f, std::min(1.0f, t));
    glm::vec3 p0(-20.0f, 0.70f, 10.5f); 
    glm::vec3 p1(-8.5f, 0.650f, 11.5f);  // Start point
    glm::vec3 p2(-9.0f, 8.60f, 40.0f);   // Control 1 (going up)
   
    glm::vec3 p3(9.0f, 8.60f, 35.0f);    // Control 2 (going down)
    glm::vec3 p4(10.0f, 0.70f, 14.0f); 
    
    
    float u = 1.0f - s;
    
    // Quartic Bezier Formula
    position = (u * u * u * u * p0) + 
               (4.0f * u * u * u * s * p1) + 
               (6.0f * u * u * s * s * p2) + 
               (4.0f * u * s * s * s * p3) + 
               (s * s * s * s * p4);
        } else if (pathType == 3) {
            // Bridge 2: curved bridge-shaped elevated path pushed toward the top edge.
            float sRaw = std::max(0.0f, std::min(1.0f, t));
            // Smoothstep easing reduces abrupt visual speed changes near the ramps.
            float s = sRaw * sRaw * (3.0f - 2.0f * sRaw);
            // Start a little lower on the right-side bridge entry, then move left while elevated.
            glm::vec3 p_start(17.0f, 0.7f, -12.5f); // Adjust these coordinates as needed
glm::vec3 p0(10.0f, 0.70f, -11.5f);
glm::vec3 p1(11.0f, 10.8f, -40.0f);
glm::vec3 p2(-11.0f, 10.8f, -25.5f);
glm::vec3 p3(-10.0f, 0.90f, -14.0f);

float u = 1.0f - s;

// 2. Use the Quartic (5-point) Bezier formula
position = (u * u * u * u * p_start) + 
           (4.0f * u * u * u * s * p0) + 
           (6.0f * u * u * s * s * p1) + 
           (4.0f * u * s * s * s * p2) + 
           (s * s * s * s * p3);
        } else if (pathType == 4) {
            // Curved bridge: Come from -X (right lane) and turn onto +Z (right lane)
            glm::vec3 p0(-18.0f, 0.55f, -1.2f);
            glm::vec3 p1(-1.2f, 2.5f, -1.2f); // Bridge peak
            glm::vec3 p2(1.2f, 0.55f, 18.0f);

            float u = 1.0f - t;
            position = u * u * p0 + 2.0f * u * t * p1 + t * t * p2;
        } else if (pathType == 5) {
            glm::vec3 p0(1.2f, 0.55f, -18.0f);
            glm::vec3 p1(1.2f, 2.5f, 1.2f);
            glm::vec3 p2(18.0f, 0.55f, -1.2f);
            float u = 1.0f - t;
            position = u * u * p0 + 2.0f * u * t * p1 + t * t * p2;
        }

        glm::vec3 dir = glm::normalize(position - oldPos);
        if (glm::length(dir) > 0.001f) {
            angle = atan2(dir.x, dir.z);
        }
    }
};

std::vector<Car> cars;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Increase this value to make the 3D model bigger
float modelScale = 50.0f;

void processInput(GLFWwindow *window) {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Scale controls
    if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        modelScale += 0.01f;
    if(glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        modelScale -= 0.01f;
        if(modelScale < 0.001f) modelScale = 0.001f;
    }
}

// Minimal placeholder drawing functions
void drawExtendedRoads(unsigned int shaderProgram, unsigned int vao) {
    // Set road color
    int objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    glUniform3f(objectColorLoc, 0.2f, 0.2f, 0.2f); // Dark gray road

    int modelLoc = glGetUniformLocation(shaderProgram, "model");

    // Road 1 (X axis)
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(100.0f, 0.1f, 5.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // Road 2 (Z axis)
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(5.0f, 0.1f, 100.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void drawCars(unsigned int shaderProgram, unsigned int vao) {
    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    int colorLoc = glGetUniformLocation(shaderProgram, "objectColor");

    for(const auto& car : cars) {
        // --- 1. Car Base Body ---
        glUniform3f(colorLoc, car.color.r, car.color.g, car.color.b);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, car.position);
        model = glm::rotate(model, car.angle, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 baseModel = glm::scale(model, glm::vec3(2.0f, 0.6f, 4.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(baseModel));

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // --- 2. Car Cabin (Top Area) ---
        glUniform3f(colorLoc, car.color.r * 0.8f, car.color.g * 0.8f, car.color.b * 0.8f); // Slightly darker tint for hood
        glm::mat4 cabinModel = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f));
        cabinModel = glm::scale(cabinModel, glm::vec3(1.6f, 0.5f, 2.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(cabinModel));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // --- 3. Four Wheels ---
        glUniform3f(colorLoc, 0.1f, 0.1f, 0.1f); // Dark rubber tires
        for (int i = 0; i < 4; i++) {
            float wx = (i % 2 == 0) ? -1.1f : 1.1f;
            float wz = (i < 2) ? 1.2f : -1.2f;
            glm::mat4 wheelModel = glm::translate(model, glm::vec3(wx, -0.2f, wz));
            wheelModel = glm::scale(wheelModel, glm::vec3(0.4f, 0.6f, 0.6f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wheelModel));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }
}

int main() {
    // 1. Initialize GLFW & Setup Window
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Traffic Simulator (No Traffic Lights)", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // 2. Setup exactly three cars, one straight and one on each bridge.
cars.push_back({0, 0.00f, 0.4f, glm::vec3(1.0f, 0.2f, 0.2f)}); // Lead red car
cars.push_back({0, 0.15f, 0.4f, glm::vec3(1.0f, 0.8f, 0.2f)}); // Following yellow car
cars.push_back({0, 0.30f, 0.4f, glm::vec3(0.8f, 0.2f, 1.0f)}); // Following purple car

// --- Path 1: Straight Underpass (-Z) ---
cars.push_back({1, 0.05f, 0.45f, glm::vec3(0.2f, 0.9f, 0.9f)}); // Cyan car heading opposite

// --- Path 2: Bridge 1 (Green trajectory) ---
// Note: Lower speed for bridges often looks better for curved paths
cars.push_back({2, 0.00f, 0.28f, glm::vec3(0.2f, 1.0f, 0.2f)}); // Lead green car
cars.push_back({2, 0.20f, 0.28f, glm::vec3(0.5f, 1.0f, 0.5f)}); // Following light green car

// --- Path 3: Bridge 2 (Blue trajectory) ---
cars.push_back({3, 0.00f, 0.32f, glm::vec3(0.2f, 0.4f, 1.0f)}); // Lead blue car
cars.push_back({3, 0.25f, 0.32f, glm::vec3(0.6f, 0.7f, 1.0f)}); // Following light blue car

    // 3. Define basic cube vertices for roads and cars
    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Set up shader program
    Shader shaderProgram("../shaders/vertex.glsl", "../shaders/fragment.glsl");

    // Load actual GLB model
    std::cout << "Loading 3D model... This might take a few seconds." << std::endl;
    Model trafficModel("../traffic_3d_model.glb");
    std::cout << "3D model loaded successfully!" << std::endl;

    glm::vec3 minBound(999999.0f), maxBound(-999999.0f);
    for (const auto& mesh : trafficModel.meshes) {
        for (const auto& vertex : mesh.vertices) {
            minBound.x = std::min(minBound.x, vertex.Position.x);
            minBound.y = std::min(minBound.y, vertex.Position.y);
            minBound.z = std::min(minBound.z, vertex.Position.z);
            maxBound.x = std::max(maxBound.x, vertex.Position.x);
            maxBound.y = std::max(maxBound.y, vertex.Position.y);
            maxBound.z = std::max(maxBound.z, vertex.Position.z);
        }
    }
    std::cout << "Model Bounding Box: Min(" << minBound.x << ", " << minBound.y << ", " << minBound.z << ") "
              << "Max(" << maxBound.x << ", " << maxBound.y << ", " << maxBound.z << ")" << std::endl;

    glm::vec3 center = (minBound + maxBound) / 2.0f;
    std::cout << "Model Center: " << center.x << ", " << center.y << ", " << center.z << std::endl;

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    // 4. Render Loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Update car logic
        for (auto& car : cars) {
            car.update(deltaTime);
        }

        glClearColor(0.5f, 0.7f, 0.9f, 1.0f); // Sky blue
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderProgram.use();

        // View/Projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);

        // Fixed camera so the frame does not rotate.
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 70.0f, 95.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        shaderProgram.setMat4("projection", projection);
        shaderProgram.setMat4("view", view);

        // Lighting settings
        shaderProgram.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
        shaderProgram.setVec3("lightPos", 0.0f, 100.0f, 120.0f);
        shaderProgram.setVec3("viewPos", 0.0f, 70.0f, 95.0f);

        // Render the base GLB model using Assimp
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        // Translate slightly up so it's not hidden under the road
        modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.5f, 0.0f));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(modelScale, modelScale, modelScale));	// Dynamically scale
        shaderProgram.setMat4("model", modelMatrix);
        shaderProgram.setVec3("objectColor", 0.6f, 0.6f, 0.6f); // Default light gray for the model
        trafficModel.Draw(shaderProgram);

        // Render extended roads
        // drawExtendedRoads(shaderProgram.ID, VAO);

        // Render moving cars
        drawCars(shaderProgram.ID, VAO);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return 0;
}
