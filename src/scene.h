#ifndef SCENE_H
#define SCENE_H

#include "material.h"
#include "mesh.h"
#include "shader.h"
#include "tinyobjloader.h"
#include "utils.h"

#include <map>
#include <vector>

class Scene {
private:
  std::vector<Mesh> meshes;
  std::vector<Material> materials;
  std::map<std::string, GLuint> textures;
  float gMinX, gMinY, gMinZ, gMaxX, gMaxY, gMaxZ;

public:
  Scene()
      : gMinX(FLT_MAX), gMinY(FLT_MAX), gMinZ(FLT_MAX), gMaxX(FLT_MIN),
        gMaxY(FLT_MIN), gMaxZ(FLT_MIN){};
  void loadTextureFromFile(const char *texturePath, const char *textureName);
  void loadObj(const char *textureDir, const char *filePath);
  void draw(Shader &shader, int textureUnit);
  glm::vec3 getWorldCenter();
  float getWorldSize();
  void computeTangentAndBitangent(GLfloat pos1x, GLfloat pos1y, GLfloat pos1z,
                                  GLfloat pos2x, GLfloat pos2y, GLfloat pos2z,
                                  GLfloat pos3x, GLfloat pos3y, GLfloat pos3z,
                                  GLfloat uv1x, GLfloat uv1y, GLfloat uv2x,
                                  GLfloat uv2y, GLfloat uv3x, GLfloat uv3y,
                                  glm::vec3 &tangent, glm::vec3 &bitangent);
};

#endif /* ifndef SCENE_H */
