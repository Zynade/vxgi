#include "scene.h"
#include "stb_image.h"

#include <iostream>

void Scene::loadTextureFromFile(const char *texturePath,
                                const char *textureName) {
  GLuint textureId;
  int w, h;
  int comp;

  unsigned char *image = stbi_load(texturePath, &w, &h, &comp, STBI_default);
  if (!image) {
    std::cout << "Unable to load texture: " << textureName << std::endl;
    exit(1);
  }

  std::cout << "Loaded texture: " << textureName << ", w = " << w
            << ", h = " << h << ", comp = " << comp << std::endl;

  glGenTextures(1, &textureId);
  glBindTexture(GL_TEXTURE_2D, textureId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  if (comp == 1) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE,
                 image);
  } else if (comp == 3) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 image);
  } else if (comp == 4) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 image);
  } else {
    std::cout << "Comp invalid: " << comp << std::endl;
    exit(1);
  }
  glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);
  stbi_image_free(image);
  textures.insert(std::make_pair(textureName, textureId));
}

void Scene::loadObj(const char *textureDir, const char *filePath,
                    int isDynamic) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> tmaterials;

  std::string warn, err;
  bool success = tinyobj::LoadObj(&attrib, &shapes, &tmaterials, &warn, &err,
                                  filePath, textureDir);

  if (!warn.empty()) {
    std::cout << "WARN: " << warn << std::endl;
  }

  if (!err.empty()) {
    std::cout << err << std::endl;
  }

  if (!success) {
    std::cout << "Failed to load: " << filePath << std::endl;
    exit(1);
  }

  stbi_set_flip_vertically_on_load(true);
  for (size_t m = 0; m < tmaterials.size(); m++) {
    Material material;
    tinyobj::material_t *mp = &tmaterials[m];

    if (mp->diffuse_texname.length() > 0) {
      std::string textureName = mp->diffuse_texname;
      if (textures.find(textureName) == textures.end()) {
        std::string texturePath = textureDir + textureName;
        loadTextureFromFile(texturePath.c_str(), textureName.c_str());
      }
      material.diffuseMap = textures[textureName];
    }

    if (mp->specular_texname.length() > 0) {
      std::string textureName = mp->specular_texname;
      if (textures.find(textureName) == textures.end()) {
        std::string texturePath = textureDir + textureName;
        loadTextureFromFile(texturePath.c_str(), textureName.c_str());
      }
      material.specularMap = textures[textureName];
    }

    if (mp->bump_texname.length() > 0) {
      std::string textureName = mp->bump_texname;
      if (textures.find(textureName) == textures.end()) {
        std::string texturePath = textureDir + textureName;
        loadTextureFromFile(texturePath.c_str(), textureName.c_str());
      }
      material.normalMap = textures[textureName];
    }

    material.kd = glm::vec3(mp->diffuse[0], mp->diffuse[1], mp->diffuse[2]);
    material.ks = glm::vec3(mp->specular[0], mp->specular[1], mp->specular[2]);
    material.shininess = mp->shininess;

    if (mp->name == "floor") {
      floorIdx = m;
    }
    if (mp->name == "fabric_g") {
      curtainIdx = m;
    }

    materials.push_back(material);
  }

  for (size_t s = 0; s < shapes.size(); s++) {
    Mesh mesh;
    std::vector<GLfloat> buffer;
    Material material = materials[shapes[s].mesh.material_ids[0]];
    float scaleFactor = 1.0;

    if (isDynamic) {
      Material material = Material();
      material.kd = glm::vec3(0.67, 0.84, 0.90);
      materials.push_back(material);
      scaleFactor = 40.0;
    }

    for (size_t f = 0; f < shapes[s].mesh.indices.size() / 3; f++) {
      tinyobj::index_t idx0 = shapes[s].mesh.indices[3 * f + 0];
      tinyobj::index_t idx1 = shapes[s].mesh.indices[3 * f + 1];
      tinyobj::index_t idx2 = shapes[s].mesh.indices[3 * f + 2];

      GLfloat v[3][3];
      for (int k = 0; k < 3; k++) {
        int f0 = idx0.vertex_index;
        int f1 = idx1.vertex_index;
        int f2 = idx2.vertex_index;

        v[0][k] = attrib.vertices[3 * f0 + k];
        v[1][k] = attrib.vertices[3 * f1 + k];
        v[2][k] = attrib.vertices[3 * f2 + k];
      }

      GLfloat n[3][3];
      int nf0 = idx0.normal_index;
      int nf1 = idx1.normal_index;
      int nf2 = idx2.normal_index;

      for (int k = 0; k < 3; k++) {
        n[0][k] = attrib.normals[3 * nf0 + k];
        n[1][k] = attrib.normals[3 * nf1 + k];
        n[2][k] = attrib.normals[3 * nf2 + k];
      }

      GLfloat tc[3][2];
      if (attrib.texcoords.size() > 0) {
        tc[0][0] = attrib.texcoords[2 * idx0.texcoord_index];
        tc[0][1] = attrib.texcoords[2 * idx0.texcoord_index + 1];
        tc[1][0] = attrib.texcoords[2 * idx1.texcoord_index];
        tc[1][1] = attrib.texcoords[2 * idx1.texcoord_index + 1];
        tc[2][0] = attrib.texcoords[2 * idx2.texcoord_index];
        tc[2][1] = attrib.texcoords[2 * idx2.texcoord_index + 1];
      } else {
        tc[0][0] = 0.0f;
        tc[0][1] = 0.0f;
        tc[1][0] = 0.0f;
        tc[1][1] = 0.0f;
        tc[2][0] = 0.0f;
        tc[2][1] = 0.0f;
      }

      glm::vec3 tangent;
      glm::vec3 bitangent;
      if (material.normalMap > 0) {
        computeTangentAndBitangent(v[0][0], v[0][1], v[0][2], v[1][0], v[1][1],
                                   v[1][2], v[2][0], v[2][1], v[2][2], tc[0][0],
                                   tc[0][1], tc[1][0], tc[1][1], tc[2][0],
                                   tc[2][1], tangent, bitangent);
      }

      for (int k = 0; k < 3; k++) {
        buffer.push_back(v[k][0] * scaleFactor);
        buffer.push_back(v[k][1] * scaleFactor);
        buffer.push_back(v[k][2] * scaleFactor);
        buffer.push_back(n[k][0]);
        buffer.push_back(n[k][1]);
        buffer.push_back(n[k][2]);
        buffer.push_back(tc[k][0]);
        buffer.push_back(tc[k][1]);

        if (material.normalMap > 0) {
          buffer.push_back(tangent.x);
          buffer.push_back(tangent.y);
          buffer.push_back(tangent.z);
          buffer.push_back(bitangent.x);
          buffer.push_back(bitangent.y);
          buffer.push_back(bitangent.z);
        }

        gMinX = std::min(gMinX, v[k][0]);
        gMinY = std::min(gMinY, v[k][1]);
        gMinZ = std::min(gMinZ, v[k][2]);

        gMaxX = std::max(gMaxX, v[k][0]);
        gMaxY = std::max(gMaxY, v[k][1]);
        gMaxZ = std::max(gMaxZ, v[k][2]);
      }
    }

    mesh.vao = 0;
    mesh.vbo = 0;
    mesh.materialId = shapes[s].mesh.material_ids[0];
    mesh.isDynamic = isDynamic;
    if (mesh.isDynamic) {
      mesh.materialId = materials.size() - 1;
      dynamicIdx = mesh.materialId;
    }

    GLuint stride = 8;
    if (material.normalMap > 0) {
      stride += 6;
    }

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * buffer.size(), &buffer[0],
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat),
                          (void *)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat),
                          (void *)(3 * sizeof(GLfloat)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat),
                          (void *)(6 * sizeof(GLfloat)));

    if (material.normalMap > 0) {
      glEnableVertexAttribArray(3);
      glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat),
                            (void *)(8 * sizeof(GLfloat)));

      glEnableVertexAttribArray(4);
      glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat),
                            (void *)(11 * sizeof(GLfloat)));
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    mesh.numTriangles = buffer.size() / (stride) / 3;

    meshes.push_back(mesh);
  }
}

void Scene::draw(Shader &shader, int textureUnit) {
  shader.use();

  for (size_t i = 0; i < meshes.size(); i++) {
    Mesh mesh = meshes[i];
    Material material = materials[mesh.materialId];
    glBindVertexArray(mesh.vao);

    int zero = 0, one = 1, mapUnit = textureUnit;
    shader.setUniform(uniformType::fv3, glm::value_ptr(material.kd), "kd");
    shader.setUniform(uniformType::fv3, glm::value_ptr(material.ks), "ks");
    shader.setUniform(uniformType::fv3, glm::value_ptr(material.ke), "ke");
    shader.setUniform(uniformType::f1, &(material.shininess), "shininess");
    shader.setUniform(uniformType::i1, &zero, "hasDiffuseMap");
    shader.setUniform(uniformType::i1, &zero, "hasSpecularMap");
    shader.setUniform(uniformType::i1, &zero, "hasNormalMap");

    if (material.diffuseMap > 0) {
      shader.setUniform(uniformType::i1, &one, "hasDiffuseMap");
    }
    glActiveTexture(GL_TEXTURE0 + textureUnit + 0);
    glBindTexture(GL_TEXTURE_2D, material.diffuseMap);
    mapUnit = textureUnit + 0;
    shader.setUniform(uniformType::i1, &mapUnit, "diffuseMap");

    if (material.specularMap > 0) {
      shader.setUniform(uniformType::i1, &one, "hasSpecularMap");
    }
    glActiveTexture(GL_TEXTURE0 + textureUnit + 1);
    glBindTexture(GL_TEXTURE_2D, material.specularMap);
    mapUnit = textureUnit + 1;
    shader.setUniform(uniformType::i1, &mapUnit, "specularMap");

    if (material.normalMap > 0) {
      shader.setUniform(uniformType::i1, &one, "hasNormalMap");
    }
    glActiveTexture(GL_TEXTURE0 + textureUnit + 2);
    glBindTexture(GL_TEXTURE_2D, material.normalMap);
    mapUnit = textureUnit + 2;
    shader.setUniform(uniformType::i1, &mapUnit, "normalMap");

    if (mesh.isDynamic) {
      // Adjust its model matrix
      glm::mat4 modelT = glm::mat4(1.0f);
      // get dragon position
      modelT = glm::translate(modelT, glm::vec3(dynamicMeshPosition));
      shader.setUniform(uniformType::mat4x4, glm::value_ptr(modelT), "M");
    }

    glDrawArrays(GL_TRIANGLES, 0, 3 * mesh.numTriangles);

    glActiveTexture(GL_TEXTURE0 + textureUnit + 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0 + textureUnit + 1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0 + textureUnit + 2);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindVertexArray(0);
  }
}

glm::vec3 Scene::getWorldCenter() {
  glm::vec3 gMin = glm::vec3(gMinX, gMinY, gMinZ);
  glm::vec3 gMax = glm::vec3(gMaxX, gMaxY, gMaxZ);

  return 0.5f * (gMin + gMax);
}

float Scene::getWorldSize() {
  glm::vec3 gMin = glm::vec3(gMinX, gMinY, gMinZ);
  glm::vec3 gMax = glm::vec3(gMaxX, gMaxY, gMaxZ);

  glm::vec3 diag = (gMax - gMin);

  float size = std::max(diag.x, std::max(diag.y, diag.z));
  return size;
}

std::vector<glm::vec3> Scene::getAABB() {
  using v = glm::vec3;
  return {
      v(gMinX, gMinY, gMinZ), v(gMinX, gMinY, gMaxZ), v(gMinX, gMaxY, gMinZ),
      v(gMinX, gMaxY, gMaxZ), v(gMaxX, gMinY, gMinZ), v(gMaxX, gMinY, gMaxZ),
      v(gMaxX, gMaxY, gMinZ), v(gMaxX, gMaxY, gMaxZ),
  };
}

void Scene::computeTangentAndBitangent(
    GLfloat pos1x, GLfloat pos1y, GLfloat pos1z, GLfloat pos2x, GLfloat pos2y,
    GLfloat pos2z, GLfloat pos3x, GLfloat pos3y, GLfloat pos3z, GLfloat uv1x,
    GLfloat uv1y, GLfloat uv2x, GLfloat uv2y, GLfloat uv3x, GLfloat uv3y,
    glm::vec3 &tangent, glm::vec3 &bitangent) {
  // Create vectors for the edges of the triangle in both position and UV space
  glm::vec3 edge1(pos2x - pos1x, pos2y - pos1y, pos2z - pos1z);
  glm::vec3 edge2(pos3x - pos2x, pos3y - pos2y, pos3z - pos2z);

  glm::vec2 deltaUV1(uv2x - uv1x, uv2y - uv1y);
  glm::vec2 deltaUV2(uv3x - uv2x, uv3y - uv2y);

  // Compute the tangent and bitangent
  float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

  tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
  tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
  tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
  tangent = glm::normalize(tangent);

  bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
  bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
  bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
  bitangent = glm::normalize(bitangent);
}
