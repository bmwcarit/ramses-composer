#ifndef MATERIALMANAGER_H
#define MATERIALMANAGER_H
#include "MaterialData/materialData.h"
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

    void addCurUniform(Uniform uniform);
	bool deleteCurUniform(std::string name);
	std::vector<Uniform> getCurUniformArr();
	int curUniformArrSize();
	void curUniformClear();
	void curUniformAssign(std::vector<Uniform> arr);
	bool hasUniform(std::string name);

    void clearData();

    void traverseMaterialData();

private:
    MaterialManager();

private:
    std::map<std::string, MaterialData> materialDataMap_;
    std::map<std::string, Shader> shaderMap_;// vertex and fragment
    std::map<std::string, Bitmap> bitmapMap_;
	std::vector<Uniform> currentUniforms_;
};
}

#endif // MATERIALMANAGER_H
