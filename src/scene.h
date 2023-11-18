#ifndef SCENE_H
#define SCENE_H

#include "mesh.h"
#include "shader.h"
#include "tinyobjloader.h"
#include "utils.h"

#include <map>
#include <vector>

class Scene {
private:
  std::vector<Mesh> meshes;
  std::map<std::string, GLuint> textures;
  std::vector<tinyobj::material_t> materials;
  float gMinX, gMinY, gMinZ, gMaxX, gMaxY, gMaxZ;

public:
  Scene()
      : gMinX(FLT_MAX), gMinY(FLT_MAX), gMinZ(FLT_MAX), gMaxX(FLT_MIN),
        gMaxY(FLT_MIN), gMaxZ(FLT_MIN){};
  void loadObj(const char *textureDir, const char *filePath);
  void draw(Shader &shader, bool voxelize);
  glm::vec3 getWorldCenter();
  float getWorldSize();
};

#endif /* ifndef SCENE_H */