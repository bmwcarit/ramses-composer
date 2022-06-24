#ifndef MATERIALMANAGER_H
#define MATERIALMANAGER_H
#include "materialData.h"
#include <list>
#include <map>
#include <vector>

namespace raco::guiData {
class MaterialManager
{
public:
    static MaterialManager& GetInstance();
    ~MaterialManager() {}
    MaterialManager(const MaterialManager&) = delete;
    MaterialManager& operator=(const MaterialManager&) = delete;

    bool addMaterialData(std::string key, MaterialData materialData);
    bool deleteMateialData(std::string key);
    bool getMaterialData(std::string key, MaterialData &materialData);
    std::map<std::string, MaterialData> getMaterialDataMap();

    bool addShader(std::string name, Shader shader);
    bool deleteShader(std::string name, Shader shader);
    bool getShader(std::string name, Shader &shader);
    std::map<std::string, Shader> getShaderDataMap();

    bool addBitmap(std::string name, Bitmap bitmap);
    bool deleteBitmap(std::string name, Bitmap bitmap);
    bool getBitmap(std::string name, Bitmap &bitmap);
    std::map<std::string, Bitmap> getBitmapDataMap();

    void traverseMaterialData();

private:
    MaterialManager();

private:
    std::map<std::string, MaterialData> materialDataMap_;
    std::map<std::string, Shader> shaderMap_;   // vertex和fragment
    std::map<std::string, Bitmap> bitmapMap_;   // 贴图
};
}

#endif // MATERIALMANAGER_H
