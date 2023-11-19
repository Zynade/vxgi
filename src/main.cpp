#include "camera.h"
#include "constants.h"
#include "scene.h"
#include "shader.h"
#include "utils.h"

#include <iostream>

void processInput(GLFWwindow *window);
void mouseCallback(GLFWwindow *window, double xposIn, double yposIn);
void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);

glm::mat4 modelT;

Camera camera(glm::vec3(100, 100, 100));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f;

void setupModelTransformation();

int main() {

  GLFWwindow *window = setupWindow(SCR_WIDTH, SCR_HEIGHT);

  Shader shader =
      Shader("shaders/voxelization.vert", "shaders/voxelization.frag",
             "shaders/voxelization.geom");
  shader.use();

  setupModelTransformation();
  shader.setUniform(uniformType::mat4x4, glm::value_ptr(modelT), "M");

  Scene scene = Scene();
  scene.loadObj("assets/crytek-sponza/", "assets/crytek-sponza/sponza.obj");

  // create texture for voxelization
  GLuint voxelTexture;
  std::vector<GLubyte> clearBuffer(4 * VOXEL_DIM * VOXEL_DIM * VOXEL_DIM, 0);

  glGenTextures(1, &voxelTexture);
  glBindTexture(GL_TEXTURE_3D, voxelTexture);
  glTexStorage3D(GL_TEXTURE_3D, 7, GL_RGBA8, VOXEL_DIM, VOXEL_DIM, VOXEL_DIM);
  glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, VOXEL_DIM, VOXEL_DIM, VOXEL_DIM, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, &clearBuffer[0]);

  // LOD settings for mipmapping.
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glGenerateMipmap(GL_TEXTURE_3D);
  glBindTexture(GL_TEXTURE_3D, voxelTexture);

  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  glBindImageTexture(0, voxelTexture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);
  int tx = 0;
  shader.setUniform(uniformType::i1, &tx, "voxelTexture");
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_3D, voxelTexture);
  glm::vec3 worldCenter = scene.getWorldCenter();
  float worldSizeHalf = 0.5f * scene.getWorldSize();
  shader.setUniform(uniformType::fv3, glm::value_ptr(worldCenter),
                    "worldCenter");
  shader.setUniform(uniformType::f1, &worldSizeHalf, "worldSizeHalf");
  glViewport(0, 0, VOXEL_DIM, VOXEL_DIM);
  scene.draw(shader, true);
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glfwSetCursorPosCallback(window, mouseCallback);
  glfwSetScrollCallback(window, scrollCallback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  Shader vis = Shader("shaders/vis.vert", "shaders/vis.frag");
  vis.use();
  vis.setUniform(uniformType::mat4x4, glm::value_ptr(modelT), "M");
  vis.setUniform(uniformType::fv3, glm::value_ptr(worldCenter), "worldCenter");
  vis.setUniform(uniformType::f1, &worldSizeHalf, "worldSizeHalf");
  tx = 0;
  vis.setUniform(uniformType::i1, &tx, "voxelTexture");

  glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
  while (!glfwWindowShouldClose(window)) {
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    processInput(window);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 viewT = camera.getViewMatrix();
    vis.setUniform(uniformType::mat4x4, glm::value_ptr(viewT), "V");

    glm::mat4 projectionT = glm::perspective(
        glm::radians(camera.zoom), (GLfloat)SCR_WIDTH / (GLfloat)SCR_HEIGHT,
        0.1f, 5000.0f);
    vis.setUniform(uniformType::mat4x4, glm::value_ptr(projectionT), "P");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, voxelTexture);
    scene.draw(vis, false);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}

void setupModelTransformation() {
  // Modelling transformations (Model -> World coordinates)
  modelT = glm::translate(
      glm::mat4(1.0f),
      glm::vec3(0.0, 0.0, 0.0)); // Model coordinates are the world coordinates
}

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    camera.processKeyboard(Camera_Movement::FORWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    camera.processKeyboard(Camera_Movement::BACKWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    camera.processKeyboard(Camera_Movement::LEFT, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    camera.processKeyboard(Camera_Movement::RIGHT, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    camera.processKeyboard(Camera_Movement::UP, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    camera.processKeyboard(Camera_Movement::DOWN, deltaTime);
}

void mouseCallback(GLFWwindow *window, double xposIn, double yposIn) {
  float xpos = static_cast<float>(xposIn);
  float ypos = static_cast<float>(yposIn);

  if (firstMouse) {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
  }

  float xoffset = xpos - lastX;
  float yoffset = lastY - ypos;

  lastX = xpos;
  lastY = ypos;

  camera.processMouseMovement(xoffset, yoffset);
}

void scrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
  camera.processMouseScroll(static_cast<float>(yoffset));
}
