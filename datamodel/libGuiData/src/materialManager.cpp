#include "MaterialData/materialManager.h"

namespace raco::guiData {
MaterialManager &MaterialManager::GetInstance() {
    static MaterialManager Instance;
    return Instance;
}

MaterialManager::MaterialManager() {

}

bool MaterialManager::addMaterialData(std::string key, MaterialData materialData) {
    auto it = materialDataMap_.find(key);
    if (it != materialDataMap_.end()) {
        return false;
    }
    materialDataMap_.emplace(key, materialData);
    return true;
}

bool MaterialManager::deleteMateialData(std::string key) {
    auto it = materialDataMap_.find(key);
    if (it == materialDataMap_.end()) {
        return false;
    }
    materialDataMap_.erase(it);
    return true;
}

bool MaterialManager::getMaterialData(std::string key, MaterialData &materialData) {
    auto it = materialDataMap_.find(key);
    if (it == materialDataMap_.end()) {
        return false;
    }
    materialData = it->second;
    return true;
}

std::map<std::string, MaterialData> MaterialManager::getMaterialDataMap() {
    return materialDataMap_;
}

bool MaterialManager::addShader(std::string name, Shader shader) {
    auto it = shaderMap_.find(name);
    if (it != shaderMap_.end()) {
        return false;
    }
    shaderMap_.emplace(name, shader);
    return true;
}

bool MaterialManager::deleteShader(std::string name, Shader shader) {
    auto it = shaderMap_.find(name);
    if (it == shaderMap_.end()) {
        return false;
    }
    shaderMap_.erase(it);
    return true;
}

bool MaterialManager::getShader(std::string name, Shader &shader) {
    auto it = shaderMap_.find(name);
    if (it == shaderMap_.end()) {
        return false;
    }
    shader = it->second;
    return true;
}

std::map<std::string, Shader> MaterialManager::getShaderDataMap() {
    return shaderMap_;
}

bool MaterialManager::addBitmap(std::string name, Bitmap bitmap) {
    auto it = bitmapMap_.find(name);
    if (it != bitmapMap_.end()) {
        return false;
    }
    bitmapMap_.emplace(name, bitmap);
    return true;
}

bool MaterialManager::deleteBitmap(std::string name, Bitmap bitmap) {
    auto it = bitmapMap_.find(name);
    if (it == bitmapMap_.end()) {
        return false;
    }
    bitmapMap_.erase(it);
    return true;
}

bool MaterialManager::getBitmap(std::string name, Bitmap &bitmap) {
    auto it = bitmapMap_.find(name);
    if (it == bitmapMap_.end()) {
        return false;
    }
    bitmap = it->second;
    return true;
}

std::map<std::string, Bitmap> MaterialManager::getBitmapDataMap() {
    return bitmapMap_;
}

void MaterialManager::traverseMaterialData() {
    qDebug() << "start <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";
    qDebug() << "material size:" << materialDataMap_.size();
    for (const auto &it : materialDataMap_) {
        qDebug() << "material key:" << QString::fromStdString(it.first);
        MaterialData data = it.second;
        data.traverseMaterial();
    }
    qDebug() << "shader size:" << shaderMap_.size();
    for (const auto &it : shaderMap_) {
        qDebug() << "shader key:" << QString::fromStdString(it.first);
        Shader shader = it.second;
        shader.traverseShader();
    }
    qDebug() << "bitmap size:" << bitmapMap_.size();
    for (const auto &it : bitmapMap_) {
        qDebug() << "bitmap key:" << QString::fromStdString(it.first);
        Bitmap map = it.second;
        map.traverseBitmap();
    }
    qDebug() << "end <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";
}
}
