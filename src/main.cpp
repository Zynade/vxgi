#include "camera.h"
#include "constants.h"
#include "scene.h"
#include "shader.h"
#include "shadowmap.h"
#include "utils.h"

#include <iostream>

void processInput(GLFWwindow *window);
void mouseCallback(GLFWwindow *window, double xposIn, double yposIn);
void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
void setupModelTransformation();
void renderQuad();

glm::mat4 modelT;

Camera camera(glm::vec3(100, 100, 100));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f;

int main() {

  GLFWwindow *window = setupWindow(SCR_WIDTH, SCR_HEIGHT);

  Scene scene = Scene();
  scene.loadObj("assets/crytek-sponza/", "assets/crytek-sponza/sponza.obj");
  setupModelTransformation();
  glm::vec3 lightPosition(200.0f, 2000.0f, 350.0f);

  ShadowMap shadowMap = ShadowMap("shaders/depthMap.vert",
                                  "shaders/depthMap.frag", lightPosition);
  shadowMap.generate(scene);

  /* VOXELIZATION */
  Shader voxelizeShader =
      Shader("shaders/voxelization.vert", "shaders/voxelization.frag",
             "shaders/voxelization.geom");
  voxelizeShader.use();

  voxelizeShader.setUniform(uniformType::mat4x4, glm::value_ptr(modelT), "M");

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
  glBindTexture(GL_TEXTURE_3D, 0);

  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  glBindImageTexture(0, voxelTexture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);
  int voxelTextureUnit = 0;
  voxelizeShader.setUniform(uniformType::i1, &voxelTextureUnit, "voxelTexture");
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_3D, voxelTexture);
  glm::vec3 worldCenter = scene.getWorldCenter();
  float worldSizeHalf = 0.5f * scene.getWorldSize();
  voxelizeShader.setUniform(uniformType::fv3, glm::value_ptr(worldCenter),
                            "worldCenter");
  voxelizeShader.setUniform(uniformType::f1, &worldSizeHalf, "worldSizeHalf");
  voxelizeShader.setUniform(uniformType::fv3, glm::value_ptr(lightPosition),
                            "lightPosition");
  voxelizeShader.setUniform(uniformType::mat4x4,
                            glm::value_ptr(shadowMap.getLightSpaceMatrix()),
                            "lightSpaceMatrix");
  int shadowMapUnit = 1;
  voxelizeShader.setUniform(uniformType::i1, &shadowMapUnit, "shadowMap");
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, shadowMap.getDepthMapTexture());
  glViewport(0, 0, VOXEL_DIM, VOXEL_DIM);
  scene.draw(voxelizeShader, 2);
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  glActiveTexture(GL_TEXTURE0);
  glGenerateMipmap(GL_TEXTURE_3D);
  glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glfwSetCursorPosCallback(window, mouseCallback);
  glfwSetScrollCallback(window, scrollCallback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  /* VOXELIZATION */

  // TODO: fetch from GPU
  // GLubyte *imageData = (GLubyte *)malloc(sizeof(GLubyte) * 4 * VOXEL_DIM *
  // VOXEL_DIM * VOXEL_DIM);
  // glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);

  // int w, h, d;
  // w = h = d = VOXEL_DIM;

  // int idx = 0;
  // glm::vec4 emitColor;
  // std::cout << "P3\n" << VOXEL_DIM << ' ' << VOXEL_DIM << "\n255\n";
  // for (int x = 255; x < 256; x++) {
  // for (int y = 0; y < h; y++) {
  // for (int z = 0; z < d; z++) {
  // idx = 4 * (x + y * w + z * w * w);
  // std::cout << (float)imageData[idx] << " ";
  // std::cout << (float)imageData[idx + 1] << " ";
  // std::cout << (float)imageData[idx + 2] << "\n";
  //}
  //}
  //}

  Shader vct = Shader("shaders/vct.vert", "shaders/vct.frag");
  vct.use();
  vct.setUniform(uniformType::mat4x4, glm::value_ptr(modelT), "M");
  vct.setUniform(uniformType::fv3, glm::value_ptr(lightPosition),
                 "lightPosition");
  vct.setUniform(uniformType::fv3, glm::value_ptr(worldCenter), "worldCenter");
  vct.setUniform(uniformType::f1, &worldSizeHalf, "worldSizeHalf");
  vct.setUniform(uniformType::mat4x4,
                 glm::value_ptr(shadowMap.getLightSpaceMatrix()),
                 "lightSpaceMatrix");

  // Shader vis = Shader("shaders/vis.vert", "shaders/vis.frag");
  // vis.use();
  // vis.setUniform(uniformType::mat4x4, glm::value_ptr(modelT), "M");
  // vis.setUniform(uniformType::fv3, glm::value_ptr(worldCenter),
  // "worldCenter"); vis.setUniform(uniformType::f1, &worldSizeHalf,
  // "worldSizeHalf");

  while (!glfwWindowShouldClose(window)) {
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    processInput(window);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    vct.use();
    glm::mat4 viewT = camera.getViewMatrix();
    vct.setUniform(uniformType::mat4x4, glm::value_ptr(viewT), "V");

    glm::mat4 projectionT = glm::perspective(
        glm::radians(camera.zoom), (GLfloat)SCR_WIDTH / (GLfloat)SCR_HEIGHT,
        0.1f, 5000.0f);
    vct.setUniform(uniformType::mat4x4, glm::value_ptr(projectionT), "P");
    glm::vec3 camPosition = camera.getPosition();
    vct.setUniform(uniformType::fv3, glm::value_ptr(camPosition),
                   "camPosition");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, voxelTexture);
    voxelTextureUnit = 0;
    vct.setUniform(uniformType::i1, &voxelTextureUnit, "voxelTexture");
    shadowMapUnit = 1;
    vct.setUniform(uniformType::i1, &shadowMapUnit, "shadowMap");
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowMap.getDepthMapTexture());
    scene.draw(vct, 2);

    // vis.use();
    // glm::mat4 viewT = camera.getViewMatrix();
    // vis.setUniform(uniformType::mat4x4, glm::value_ptr(viewT), "V");

    // glm::mat4 projectionT = glm::perspective(
    // glm::radians(camera.zoom), (GLfloat)SCR_WIDTH / (GLfloat)SCR_HEIGHT,
    // 0.1f, 5000.0f);
    // vis.setUniform(uniformType::mat4x4, glm::value_ptr(projectionT), "P");
    // voxelTextureUnit = 0;
    // vis.setUniform(uniformType::i1, &voxelTextureUnit, "voxelTexture");
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_3D, voxelTexture);
    // scene.draw(vis, 1);

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

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad() {
  if (quadVAO == 0) {
    float quadVertices[] = {
        // positions        // texture Coords
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,
    };
    // setup plane VAO
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)(3 * sizeof(float)));
  }
  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindVertexArray(0);
}
