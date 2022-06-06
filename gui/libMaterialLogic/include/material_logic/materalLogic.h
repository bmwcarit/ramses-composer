#ifndef MATERALLOGIC_H
#define MATERALLOGIC_H

#include <QObject>
#include "core/CommandInterface.h"
#include "MaterialData/materialManager.h"
#include <map>

using namespace raco::guiData;
namespace raco::material_logic {

class MateralLogic : public QObject
{
    Q_OBJECT
public:
    explicit MateralLogic(QObject *parent = nullptr);
	void setResourcesHandleReMap(std::map<std::string, core::ValueHandle> map) {
		resourcesHandleReMap_ = map;
	}
	
	std::map<std::string, core::ValueHandle>& getResourcesHandleReMap() {
		return resourcesHandleReMap_;
	}

    void Analyzing();
    void initMaterialProperty(core::ValueHandle valueHandle, MaterialData &materialData, Shader &shader);
    void setOptionsProperty(core::ValueHandle valueHandle, MaterialData &materialData);
    void setUniformsProperty(core::ValueHandle valueHandle, MaterialData &materialData);
    void setTexturePorperty(core::ValueHandle valueHandle, MaterialData &materialData, TextureData &textureData);
    void setUniformsMultiElementProperty(core::ValueHandle valueHandle, MaterialData &materialData, UniformType type);

private:
	std::map<std::string, core::ValueHandle> resourcesHandleReMap_;
};

}

#endif // MATERALLOGIC_H
