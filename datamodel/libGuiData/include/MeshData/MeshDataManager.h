#ifndef MESHDATAMANAGER_H
#define MESHDATAMANAGER_H

#include <list>
#include <mutex>
#include <string>
#include <any>
#include <map>
#include <vector>

namespace raco::guiData {

enum class VertexAttribDataType {
    VAT_Float = 0,
    VAT_Float2,
    VAT_Float3,
    VAT_Float4
};

struct Attribute {
    std::string name;
    VertexAttribDataType type;
    std::vector<float> data;
};

class MeshData {
public:
    MeshData();

    void setMeshName(std::string name);
    std::string getMeshName();

    void setMeshUri(std::string uri);
    std::string getMeshUri();

    void setNumTriangles(int num);
    int getNumTriangles();

    void setNumVertices(int num);
    int getNumVertices();

    int getAttributeSize();
    void addAttribute(Attribute attr);
    std::vector<Attribute> getAttributes();

    void setIndices(std::vector<uint32_t> indices);
    std::vector<uint32_t> getIndices();
private:
    std::string meshName_;
    std::string meshUri_;
    int numTriangles_{0};
    int numVertices_{0};
    std::vector<Attribute> attributes_;
    std::vector<uint32_t> indexBuffer_;
};

class MeshDataManager {
public:
    static MeshDataManager &GetInstance();
    MeshDataManager(const MeshDataManager&) = delete;
    MeshDataManager& operator=(const MeshDataManager&) = delete;

    void clearMesh();

    void addMeshData(std::string id, MeshData mesh);
	bool getMeshData(std::string id, MeshData& meshdata);
    std::map<std::string, MeshData> getMeshDataMap();
private:
    MeshDataManager();
private:
    std::map<std::string, MeshData> meshDataMap_; //key
};
}

#endif // MESHDATAMANAGER_H
