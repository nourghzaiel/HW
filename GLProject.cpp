#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ctime>
#include <iostream>
#include <vector>

using namespace std;

// ---- SHADERS ----
const char* vertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main(){ gl_Position = projection * view * model * vec4(aPos,1.0); }
)";
const char* fragmentShaderSrc = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 color;
void main(){ FragColor = vec4(color,1.0); }
)";

// ---- CUBE DATA FOR CLOCK BODY ----
float cubeVertices[] = {
    -0.5,-0.5,-0.5, 0.5,-0.5,-0.5, 0.5,0.5,-0.5, 0.5,0.5,-0.5, -0.5,0.5,-0.5, -0.5,-0.5,-0.5,
    -0.5,-0.5,0.5, 0.5,-0.5,0.5, 0.5,0.5,0.5, 0.5,0.5,0.5, -0.5,0.5,0.5, -0.5,-0.5,0.5,
    -0.5,0.5,0.5, -0.5,0.5,-0.5, -0.5,-0.5,-0.5, -0.5,-0.5,-0.5, -0.5,-0.5,0.5, -0.5,0.5,0.5,
    0.5,0.5,0.5, 0.5,0.5,-0.5, 0.5,-0.5,-0.5, 0.5,-0.5,-0.5, 0.5,-0.5,0.5, 0.5,0.5,0.5,
    -0.5,-0.5,-0.5, 0.5,-0.5,-0.5, 0.5,-0.5,0.5, 0.5,-0.5,0.5, -0.5,-0.5,0.5, -0.5,-0.5,-0.5,
    -0.5,0.5,-0.5, 0.5,0.5,-0.5, 0.5,0.5,0.5, 0.5,0.5,0.5, -0.5,0.5,0.5, -0.5,0.5,-0.5
};

// ---- CIRCLE DATA FOR DIAL ----
const int circleSegments = 100;
float circleVertices[3 * (circleSegments + 2)];
void generateCircle(float radius) {
    circleVertices[0] = 0; circleVertices[1] = 0; circleVertices[2] = 0;
    for (int i = 0; i <= circleSegments; i++) {
        float a = i * 2.0f * 3.14159265f / circleSegments;
        circleVertices[3 * (i + 1) + 0] = cos(a) * radius;
        circleVertices[3 * (i + 1) + 1] = sin(a) * radius;
        circleVertices[3 * (i + 1) + 2] = 0;
    }
}

// ---- HOUR MARKERS ----
vector<glm::vec3> hourMarkers;
void generateHourMarkers() {
    for (int i = 0; i < 12; i++) {
        float angle = i * 30.0f * 3.14159265f / 180.0f;
        float x = cos(angle) * 0.9f;
        float y = sin(angle) * 0.9f;
        hourMarkers.push_back(glm::vec3(x, y, 0.01f));
    }
}

// ---- VAO BUILDER ----
unsigned int buildVAO(float* vertices, int size) {
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    return VAO;
}

// ---- CAMERA CONTROL ----
float camDistance = 15.0f;
float camYaw = 0.0f, camPitch = 20.0f;
double lastX = 400, lastY = 300;
bool firstMouse = true, leftPressed = false, rightPressed = false;

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) leftPressed = (action == GLFW_PRESS);
    if (button == GLFW_MOUSE_BUTTON_RIGHT) rightPressed = (action == GLFW_PRESS);
}
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float dx = xpos - lastX;
    float dy = ypos - lastY;
    if (leftPressed) {
        camYaw += dx * 0.3f;
        camPitch -= dy * 0.3f;
        if (camPitch > 89.0f) camPitch = 89.0f;
        if (camPitch < -89.0f) camPitch = -89.0f;
    }
    lastX = xpos; lastY = ypos;
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camDistance -= yoffset;
    if (camDistance < 3.0f) camDistance = 3.0f;
    if (camDistance > 50.0f) camDistance = 50.0f;
}

// ---- MAIN ----
int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "3D Clock", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_DEPTH_TEST);

    // ---- SHADERS ----
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSrc, NULL); glCompileShader(vs);
    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSrc, NULL); glCompileShader(fs);
    unsigned int shader = glCreateProgram();
    glAttachShader(shader, vs); glAttachShader(shader, fs);
    glLinkProgram(shader); glUseProgram(shader);

    // ---- VAOs ----
    unsigned int cubeVAO = buildVAO(cubeVertices, sizeof(cubeVertices));
    generateCircle(1.0f);
    unsigned int circleVAO = buildVAO(circleVertices, sizeof(circleVertices));
    generateHourMarkers();

    // ---- CAMERA CALLBACKS ----
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // ---- PROJECTION ----
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, &projection[0][0]);

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ---- CAMERA VIEW ----
        glm::vec3 camPos;
        camPos.x = camDistance * cos(glm::radians(camYaw)) * cos(glm::radians(camPitch));
        camPos.y = camDistance * sin(glm::radians(camPitch));
        camPos.z = camDistance * sin(glm::radians(camYaw)) * cos(glm::radians(camPitch));
        glm::mat4 view = glm::lookAt(camPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, &view[0][0]);

        // ---- TIME ----
        time_t now = time(0); tm ltm; localtime_s(&ltm, &now);
        float hr = ltm.tm_hour % 12 + ltm.tm_min / 60.0f;
        float min = ltm.tm_min + ltm.tm_sec / 60.0f;
        float sec = ltm.tm_sec;
        float t = glfwGetTime();
        float swing = sin(t * 2.0f) * 0.8f;

        // ---- CLOCK BODY LONG ----
        glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(3.5f, 6.0f, 0.8f));
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &model[0][0]);
        glUniform3f(glGetUniformLocation(shader, "color"), 0.6f, 0.3f, 0.1f);
        glBindVertexArray(cubeVAO); glDrawArrays(GL_TRIANGLES, 0, 36);

        // ---- CLOCK DIAL CENTER (SMALLER) ----
        model = glm::translate(glm::mat4(1.0f), glm::vec3(0, 1.5f, 0.45f));
        model = glm::scale(model, glm::vec3(1.2f, 1.2f, 1.0f)); // الحجم أصغر
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &model[0][0]);
        glUniform3f(glGetUniformLocation(shader, "color"), 0.95f, 0.95f, 0.95f);
        glBindVertexArray(circleVAO); glDrawArrays(GL_TRIANGLE_FAN, 0, circleSegments + 2);

        // ---- Hour Markers ----
        for (auto& h : hourMarkers) {
            model = glm::translate(glm::mat4(1.0f), glm::vec3(h.x, h.y + 1.5f, h.z + 0.45f));
            model = glm::scale(model, glm::vec3(0.05f, 0.2f, 0.05f));
            glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &model[0][0]);
            glUniform3f(glGetUniformLocation(shader, "color"), 0, 0, 0);
            glBindVertexArray(cubeVAO); glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // ---- HOUR HAND ----
        model = glm::translate(glm::mat4(1.0f), glm::vec3(0, 1.5f, 0.5f));
        model = glm::rotate(model, -hr * glm::radians(30.0f), glm::vec3(0, 0, 1));
        model = glm::translate(model, glm::vec3(0, -0.4f, 0));
        model = glm::scale(model, glm::vec3(0.1f, 0.8f, 0.05f));
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &model[0][0]);
        glUniform3f(glGetUniformLocation(shader, "color"), 0, 0, 0);
        glBindVertexArray(cubeVAO); glDrawArrays(GL_TRIANGLES, 0, 36);

        // ---- MINUTE HAND ----
        model = glm::translate(glm::mat4(1.0f), glm::vec3(0, 1.5f, 0.51f));
        model = glm::rotate(model, -min * glm::radians(6.0f), glm::vec3(0, 0, 1));
        model = glm::translate(model, glm::vec3(0, -0.55f, 0));
        model = glm::scale(model, glm::vec3(0.07f, 1.1f, 0.05f));
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &model[0][0]);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // ---- SECOND HAND ----
        model = glm::translate(glm::mat4(1.0f), glm::vec3(0, 1.5f, 0.52f));
        model = glm::rotate(model, -sec * glm::radians(6.0f), glm::vec3(0, 0, 1));
        model = glm::translate(model, glm::vec3(0, -0.65f, 0));
        model = glm::scale(model, glm::vec3(0.03f, 1.3f, 0.05f));
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &model[0][0]);
        glUniform3f(glGetUniformLocation(shader, "color"), 1, 0, 0);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // ---- PENDULUM ROD LONG ----
        model = glm::translate(glm::mat4(1.0f), glm::vec3(swing, -3.0f, 0.3f));
        model = glm::scale(model, glm::vec3(0.08f, 4.0f, 0.08f));
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &model[0][0]);
        glUniform3f(glGetUniformLocation(shader, "color"), 0.8f, 0.7f, 0.1f);
        glBindVertexArray(cubeVAO); glDrawArrays(GL_TRIANGLES, 0, 36);

        // ---- PENDULUM BALL ----
        model = glm::translate(glm::mat4(1.0f), glm::vec3(swing, -5.0f, 0.3f));
        model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.2f));
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &model[0][0]);
        glUniform3f(glGetUniformLocation(shader, "color"), 0.4f, 0.4f, 0.4f);
        glBindVertexArray(circleVAO); glDrawArrays(GL_TRIANGLE_FAN, 0, circleSegments + 2);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
