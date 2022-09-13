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

bool MaterialManager::addNodeMaterial(std::string key, NodeMaterial nodeMaterial) {
	auto it = nodeMaterialDataMap_.find(key);
	if (it != nodeMaterialDataMap_.end()) {
		return false;
	}
	nodeMaterialDataMap_.emplace(key, nodeMaterial);
	return true;
}

bool MaterialManager::deleteNodeMateial(std::string key) {
	auto it = nodeMaterialDataMap_.find(key);
	if (it == nodeMaterialDataMap_.end()) {
		return false;
	}
	nodeMaterialDataMap_.erase(it);
	return true;
}

bool MaterialManager::getNodeMaterial(std::string key, NodeMaterial &nodeMaterial) {
	auto it = nodeMaterialDataMap_.find(key);
	if (it == nodeMaterialDataMap_.end()) {
		return false;
	}
	nodeMaterial = it->second;
	return true;
}

std::map<std::string, NodeMaterial> MaterialManager::getNodeMaterialMap() {
	return nodeMaterialDataMap_;
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

bool MaterialManager::addTexture(std::string name, TextureData texture) {
	auto it = textureMap_.find(name);
	if (it != textureMap_.end()) {
		return false;
	}
	textureMap_.emplace(name, texture);
	return true;
}

bool MaterialManager::deleteTexture(std::string name) {
	auto it = textureMap_.find(name);
	if (it == textureMap_.end()) {
		return false;
	}
	textureMap_.erase(it);
	return true;
}

bool MaterialManager::getTexture(std::string name, TextureData &texture) {
	auto it = textureMap_.find(name);
	if (it == textureMap_.end()) {
		return false;
	}
	texture = it->second;
	return true;
}

std::map<std::string, TextureData> MaterialManager::getTextureDataMap() {
	return textureMap_;
}

void MaterialManager::addCurUniform(Uniform uniform) {
	currentUniforms_.push_back(uniform);
}

bool MaterialManager::deleteCurUniform(std::string name) {
	for (auto it = currentUniforms_.begin(); it != currentUniforms_.end(); ++it) {
		if (it->getName() == name) {
			currentUniforms_.erase(it);
			return true;
		}
    }
	return false;
}

std::vector<Uniform> MaterialManager::getCurUniformArr() {
	return currentUniforms_;
}

int MaterialManager::curUniformArrSize() {
	return currentUniforms_.size();
}

void MaterialManager::curUniformClear() {
	currentUniforms_.clear();
}

void MaterialManager::curUniformAssign(std::vector<Uniform> arr) {
	currentUniforms_.clear();
	currentUniforms_.swap(arr);
}

bool MaterialManager::hasUniform(std::string name) {
	for (auto &it : currentUniforms_) {
		if (it.getName() == name) {
			return true;
        }
    }
	return false;
}

void MaterialManager::clearData() {
	materialDataMap_.clear();
	nodeMaterialDataMap_.clear();
	shaderMap_.clear();
	bitmapMap_.clear();
	textureMap_.clear();
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
