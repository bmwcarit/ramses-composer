#include "MeshData/MeshDataManager.h"

namespace raco::guiData {
MeshData::MeshData() {

}

void MeshData::setMeshName(std::string name) {
    meshName_ = name;
}

std::string MeshData::getMeshName() {
    return meshName_;
}

void MeshData::setMeshUri(std::string uri) {
    meshUri_ = uri;
}

std::string MeshData::getMeshUri() {
    return meshUri_;
}

void MeshData::setNumTriangles(int num) {
    numTriangles_ = num;
}

int MeshData::getNumTriangles() {
    return numTriangles_;
}

void MeshData::setNumVertices(int num) {
    numVertices_ = num;
}

int MeshData::getNumVertices() {
    return numVertices_;
}

int MeshData::getAttributeSize() {
    return attributes_.size();
}

void MeshData::addAttribute(Attribute attr) {
    attributes_.push_back(attr);
}

std::vector<Attribute> MeshData::getAttributes() {
    return attributes_;
}

void MeshData::setIndices(std::vector<uint32_t> indices) {
    indexBuffer_ = indices;
}

std::vector<uint32_t> MeshData::getIndices() {
    return indexBuffer_;
}

MeshDataManager &MeshDataManager::GetInstance() {
    static MeshDataManager Instance;
    return Instance;
}

MeshDataManager::MeshDataManager() {

}

void MeshDataManager::clearMesh() {
    meshDataMap_.clear();
}

void MeshDataManager::addMeshData(std::string id, MeshData mesh) {
    meshDataMap_.emplace(id, mesh);
}

MeshData MeshDataManager::getMeshData(std::string id) {
    return meshDataMap_.at(id);
}

std::map<std::string, MeshData> MeshDataManager::getMeshDataMap() {
    return meshDataMap_;
}
}
