#include "material_logic/materalLogic.h"
#include "MaterialData/materialManager.h"
#include "user_types/Material.h"
#include "PropertyData/PropertyType.h"

namespace raco::material_logic { 
MateralLogic::MateralLogic(QObject *parent)
    : QObject{parent} {

}

void MateralLogic::setPtxName(Shader &shader) {
	std::map<std::string, Shader> shaderMap = MaterialManager::GetInstance().getShaderDataMap();
	for (auto &it : shaderMap) {
		if (it.second.getFragmentShader() == shader.getFragmentShader()
			&& it.second.getVertexShader() == shader.getVertexShader()){
			shader.setPtxShaderName(it.second.getPtxShaderName());
			return;
        }
    }
	shader.setPtxShaderName(shader.getName());
}

void setIsDefaultMaterial(std::string id, MaterialData& materialData) {
	std::map<std::string, MaterialData> materialMap = MaterialManager::GetInstance().getMaterialDataMap();
	for (auto &it : materialMap) {
		if (it.second.getObjectName() == materialData.getObjectName()) {
			materialData.setDefaultID(it.first);
			return;
		}
	}
	materialData.setDefaultID(id);
}


void MateralLogic::AnalyzingTexture() {
	for (const auto &it : textureResourcesHandleReMap_) {
		MaterialData materialData;
        TextureData textureData;
		core::ValueHandle valueHandle = it.second;
		setTexturePorperty(valueHandle ,materialData, textureData);
		textureData.setBitmapRef(textureData.getName());
		textureData.setUniformName("");
		MaterialManager::GetInstance().addTexture(textureData.getName(), textureData);
	}
}


bool getDefaultMaterialData(raco::core::ValueHandle &valueHandle) {
	valueHandle = valueHandle[0];
	if (valueHandle.type() == core::PrimitiveType::Ref) {
		valueHandle = valueHandle.asRef();
		if (valueHandle != NULL) {
			return true;
		}
	}
	return false;
}
// Analyzing default Material
void MateralLogic::AnalyzingMaterialData() {
	for (const auto &it : materialResourcesHandleReMap_) {
		MaterialData materialData;
		Shader shader;
		core::ValueHandle valueHandle = it.second;
		if (getDefaultMaterialData(valueHandle)) {
			if (valueHandle.hasProperty("options")) {
				initMaterialProperty(valueHandle, materialData, shader);
				raco::user_types::Material *material = dynamic_cast<raco::user_types::Material *>(valueHandle.rootObject().get());
				raco::core::PropertyInterfaceList attritesArr = material->attributes();
				for (auto &it : attritesArr) {
					raco::guiData::Attribute attribute;
					attribute.name = it.name;
					attribute.type = static_cast<raco::guiData::VertexAttribDataType>(it.type);
					materialData.addUsedAttribute(attribute);
				}

				MaterialManager::GetInstance().addMaterialData(it.first, materialData);
				setPtxName(shader);
				MaterialManager::GetInstance().addShader(shader.getName(), shader);
			}
		}
	}
	MaterialManager::GetInstance().traverseMaterialData();
}

void MateralLogic::Analyzing() {
	// clear cache data
	MaterialManager::GetInstance().clearData();
	// analyzing default Material
	AnalyzingMaterialData();
	// analyzing node Material
	AnalyzingNodeMaterial();
}

void MateralLogic::initNodeMaterialProperty(core::ValueHandle valueHandle, NodeMaterial &nodeMaterial) {
	using PrimitiveType = core::PrimitiveType;	// "material" "private"   "options"  "uniforms"
	if (valueHandle != NULL) {
		for (int i{0}; i < valueHandle.size(); i++) {
			if (!valueHandle[i].isObject()) {
				raco::core::ValueHandle tempHandle = valueHandle[i];
				std::string propName = tempHandle.getPropName();
				if (QString::fromStdString(propName).compare("material") == 0) {
					if (tempHandle.type() == PrimitiveType::Ref) {
						tempHandle = tempHandle.asRef();
						if (tempHandle != NULL) {
							for (int j{0}; j < tempHandle.size(); ++j) {
								raco::core::ValueHandle value = tempHandle[j];
								std::string name = value.getPropName();
								if (QString::fromStdString(name).compare("objectName") == 0) {
									std::string name = value.asString();
									nodeMaterial.setObjectName(value.asString());
									break;
								}
							}
						}
					}
				}
				if (QString::fromStdString(propName).compare("private") == 0) {
					if (tempHandle.type() == PrimitiveType::Bool) {
						nodeMaterial.setIsPrivate(tempHandle.asBool());
					}
				}
				if (QString::fromStdString(propName).compare("options") == 0) {
					MaterialData materialData;
					setOptionsProperty(tempHandle, materialData);
					nodeMaterial.setRenderMode(materialData.getRenderMode());
				}
				if (QString::fromStdString(propName).compare("uniforms") == 0) {
					MaterialData materialData;
					setUniformsProperty(tempHandle, materialData);
					for (auto &un : materialData.getUniforms()) {
						nodeMaterial.addUniform(un);
					}
				}
				initNodeMaterialProperty(tempHandle, nodeMaterial);
			}
		}
	}
}


void MateralLogic::AnalyzingNodeMaterial() {
	for (const auto &it : materialResourcesHandleReMap_) {
		core::ValueHandle valueHandle = it.second;
		NodeMaterial nodeMaterial;
		initNodeMaterialProperty(valueHandle, nodeMaterial);
		MaterialManager::GetInstance().addNodeMaterial(it.first, nodeMaterial);
	}
}

void MateralLogic::initMaterialProperty(core::ValueHandle valueHandle, MaterialData &materialData, Shader &shader) {
    using PrimitiveType = core::PrimitiveType;
    for (int i{0}; i < valueHandle.size(); i++) {
        if (!valueHandle[i].isObject()) {
            raco::core::ValueHandle tempHandle = valueHandle[i];
			std::string propName = tempHandle.getPropName();
			if (QString::fromStdString(propName).compare("objectName") == 0) {
                if (tempHandle.type() == PrimitiveType::String) {
                    materialData.setObjectName(tempHandle.asString());
                    materialData.setShaderRef(tempHandle.asString());
                    shader.setName(tempHandle.asString());
                }
            }
			if (QString::fromStdString(propName).compare("uriVertex") == 0) {
                if (tempHandle.type() == PrimitiveType::String) {
                    shader.setVertexShader(tempHandle.asString());
                }
            }
            if (QString::fromStdString(propName).compare("uriFragment") == 0) {
                if (tempHandle.type() == PrimitiveType::String) {
                    shader.setFragmentShader(tempHandle.asString());
                }
            }
			if (QString::fromStdString(propName).compare("options") == 0) {
                setOptionsProperty(tempHandle, materialData);
            }
			if (QString::fromStdString(propName).compare("uniforms") == 0) {
                setUniformsProperty(tempHandle, materialData);
            }
            initMaterialProperty(tempHandle, materialData, shader);
        }
    }
}

void MateralLogic::setOptionsProperty(core::ValueHandle valueHandle, MaterialData &materialData) {
	Blending blending;
    ColorWrite colorWrite;
    RenderMode renderMode;
    using PrimitiveType = core::PrimitiveType;
    auto func = [&](raco::core::ValueHandle handle, std::string str, double &value)->bool {
        if (handle.hasProperty(str)) {
            raco::core::ValueHandle tempHandle = handle.get(str);
            if (tempHandle.type() == PrimitiveType::Double) {
                value = tempHandle.asDouble();
                return true;
            }
        }
        return false;
    };

    for (int i{0}; i < valueHandle.size(); i++) {
        if (!valueHandle[i].isObject()) {
            raco::core::ValueHandle tempHandle = valueHandle[i];
            std::string propName = tempHandle.getPropName();
            if (QString::fromStdString(propName).compare("blendOperationColor") == 0) {
                if (tempHandle.type() == PrimitiveType::Int) {
                    int value = tempHandle.asInt();
                    blending.setBlendOperationColor((BlendOperation)(value));
                }
            }
            if (QString::fromStdString(propName).compare("blendOperationAlpha") == 0) {
                if (tempHandle.type() == PrimitiveType::Int) {
                    int value = tempHandle.asInt();
                    blending.setBlendOperationAlpha(static_cast<BlendOperation>(value));
                }
            }
            if (QString::fromStdString(propName).compare("blendFactorSrcColor") == 0) {
                if (tempHandle.type() == PrimitiveType::Int) {
                    int value = tempHandle.asInt();
                    blending.setSrcColorFactor((BlendFactor)(value));
                }
            }
            if (QString::fromStdString(propName).compare("blendFactorDestColor") == 0) {
                if (tempHandle.type() == PrimitiveType::Int) {
                    int value = tempHandle.asInt();
                    blending.setDesColorFactor((BlendFactor)(value));
                }
            }
            if (QString::fromStdString(propName).compare("blendFactorSrcAlpha") == 0) {
                if (tempHandle.type() == PrimitiveType::Int) {
                    int value = tempHandle.asInt();
                    blending.setSrcAlphaFactor((BlendFactor)(value));
                }
            }
            if (QString::fromStdString(propName).compare("blendFactorDestAlpha") == 0) {
                if (tempHandle.type() == PrimitiveType::Int) {
                    int value = tempHandle.asInt();
                    blending.setDesAlphaFactor((BlendFactor)(value));
                }
            }
            if (QString::fromStdString(propName).compare("blendColor") == 0) {
                double value{0.0};
                if (func(tempHandle, "x", value)) {
					colorWrite.setRed(value);
                }
                if (func(tempHandle, "y", value)) {
					colorWrite.setGreen(value);
                }
                if (func(tempHandle, "z", value)) {
					colorWrite.setBlue(value);
                }
                if (func(tempHandle, "w", value)) {
                    colorWrite.setAlpha(value);
                }
            }
            if (QString::fromStdString(propName).compare("depthwrite") == 0) {
                if (tempHandle.type() == PrimitiveType::Bool) {
                    renderMode.setDepthWrite(tempHandle.asBool());
                }
            }
            if (QString::fromStdString(propName).compare("depthFunction") == 0) {
                if (tempHandle.type() == PrimitiveType::Int) {
                    int value = tempHandle.asInt();
                    renderMode.setDepthCompare(static_cast<DepthCompare>(value));
                }
            }
            if (QString::fromStdString(propName).compare("cullmode") == 0) {
                if (tempHandle.type() == PrimitiveType::Int) {
                    int value = tempHandle.asInt();
                    renderMode.setCulling(static_cast<Culling>(value));
                }
            }
        }
    }
	renderMode.setBlending(blending);
    renderMode.setColorWrite(colorWrite);
    materialData.setRenderMode(renderMode);
}

void MateralLogic::setUniformsProperty(core::ValueHandle valueHandle, MaterialData &materialData) {
    using PrimitiveType = core::PrimitiveType;
    for (int i{0}; i < valueHandle.size(); i++) {
        Uniform tempUniform;
        raco::core::ValueHandle tempHandle = valueHandle[i];
		std::string property = tempHandle.getPropName();
        switch (tempHandle.type()) {
            case PrimitiveType::String: {
                tempUniform.setName(property);
                tempUniform.setType(UniformType::String);
                tempUniform.setValue(tempHandle.asString());
                materialData.addUniform(tempUniform);
                break;
            }
            case PrimitiveType::Bool: {
                tempUniform.setName(property);
                tempUniform.setType(UniformType::Bool);
                tempUniform.setValue(tempHandle.asBool());
                materialData.addUniform(tempUniform);
                break;
            }
            case PrimitiveType::Int: {
                tempUniform.setName(property);
                tempUniform.setType(UniformType::Int);
                tempUniform.setValue(tempHandle.asInt());
                materialData.addUniform(tempUniform);
                break;
            }
            case PrimitiveType::Double: {
                tempUniform.setName(property);
                tempUniform.setType(UniformType::Double);
                tempUniform.setValue(tempHandle.asDouble());
                materialData.addUniform(tempUniform);
                break;
            }
            case PrimitiveType::Ref: {
                TextureData textureData;
                textureData.setUniformName(property);
                setTexturePorperty(tempHandle.asRef(), materialData, textureData);

				if (textureData.getName().empty()) {
					textureData.setName("empty");
					textureData.setBitmapRef("empty");
				}
                materialData.addTexture(textureData);
				break;
            }
            case PrimitiveType::Table:
            case PrimitiveType::Struct: {
                auto typeDesc = &tempHandle.constValueRef()->asStruct().getTypeDescription();
                if (typeDesc == &core::Vec2f::typeDescription) {
                    setUniformsMultiElementProperty(tempHandle, materialData, UniformType::Vec2f);
                } else if (typeDesc == &core::Vec3f::typeDescription) {
                    setUniformsMultiElementProperty(tempHandle, materialData, UniformType::Vec3f);
                } else if (typeDesc == &core::Vec4f::typeDescription) {
                    setUniformsMultiElementProperty(tempHandle, materialData, UniformType::Vec4f);
                } else if (typeDesc == &core::Vec2i::typeDescription) {
                    setUniformsMultiElementProperty(tempHandle, materialData, UniformType::Vec2i);
                } else if (typeDesc == &core::Vec3i::typeDescription) {
                    setUniformsMultiElementProperty(tempHandle, materialData, UniformType::Vec3i);
                } else if (typeDesc == &core::Vec4i::typeDescription) {
                    setUniformsMultiElementProperty(tempHandle, materialData, UniformType::Vec4i);
                }
                break;
            }
            default: {
                break;
            }
        };
    }
}

void MateralLogic::setTexturePorperty(core::ValueHandle valueHandle, MaterialData &materialData, TextureData &textureData) {
    Bitmap bitMap;
    std::string bitMapKey;
    using PrimitiveType = core::PrimitiveType;
    if (valueHandle != NULL) {
		for (int i{0}; i < valueHandle.size(); i++) {
			if (!valueHandle[i].isObject()) {
				raco::core::ValueHandle tempHandle = valueHandle[i];
				std::string propName = tempHandle.getPropName();
				if (QString::fromStdString(propName).compare("objectName") == 0) {
					if (tempHandle.type() == PrimitiveType::String) {
						bitMapKey = tempHandle.asString();
						bitMap.setName(tempHandle.asString());
						textureData.setName(tempHandle.asString());
                        textureData.setBitmapRef(tempHandle.asString());
					}
				}
				if (QString::fromStdString(propName).compare("wrapUMode") == 0) {
					if (tempHandle.type() == PrimitiveType::Int) {
						textureData.setWrapModeU(static_cast<WrapMode>(tempHandle.asInt()));
					}
				}
				if (QString::fromStdString(propName).compare("wrapVMode") == 0) {
					if (tempHandle.type() == PrimitiveType::Int) {
						textureData.setWrapModeV(static_cast<WrapMode>(tempHandle.asInt()));
					}
				}
				if (QString::fromStdString(propName).compare("minSamplingMethod") == 0) {
					if (tempHandle.type() == PrimitiveType::Int) {
						textureData.setMinFilter(static_cast<Filter>(tempHandle.asInt()));
					}
				}
				if (QString::fromStdString(propName).compare("magSamplingMethod") == 0) {
					if (tempHandle.type() == PrimitiveType::Int) {
						textureData.setMagFilter(static_cast<Filter>(tempHandle.asInt()));
					}
				}
				if (QString::fromStdString(propName).compare("anisotropy") == 0) {
					if (tempHandle.type() == PrimitiveType::Int) {
						textureData.setAnisotropicSamples(static_cast<Filter>(tempHandle.asInt()));
					}
				}
				if (QString::fromStdString(propName).compare("uri") == 0) {
					if (tempHandle.type() == PrimitiveType::String) {
                        bitMap.setResource(tempHandle.asString());
					}
				}
				if (QString::fromStdString(propName).compare("flipTexture") == 0) {
					// TODO
				}
				if (QString::fromStdString(propName).compare("generateMipmaps") == 0) {
					if (tempHandle.type() == PrimitiveType::Bool) {
						bitMap.setGenerateMipmaps(tempHandle.asBool());
					}
				}
			}
        }
    }
    MaterialManager::GetInstance().addBitmap(bitMapKey, bitMap);
}

bool vec2UniformValue(std::vector<std::any> valueVec, std::any& uniformValue, UniformType type) {
	bool result = false;
	if (valueVec.size() <= 1) {
		return result;
    }
	std::any value = valueVec.at(0);
	switch (type) {
		case raco::guiData::Vec2f: {
			Vec2 unValue;
			if (value.type() == typeid(double) && valueVec.size() == 2) {
				unValue.x = std::any_cast<double>(valueVec.at(0));
				unValue.y = std::any_cast<double>(valueVec.at(1));
				uniformValue = unValue;
				result = true;
			}
			break;
		}
		case raco::guiData::Vec3f: {
			Vec3 unValue;
			if (value.type() == typeid(double) && valueVec.size() == 3) {
				unValue.x = std::any_cast<double>(valueVec.at(0));
				unValue.y = std::any_cast<double>(valueVec.at(1));
				unValue.z = std::any_cast<double>(valueVec.at(2));
				uniformValue = unValue;
				result = true;
			}
			break;
		}
		case raco::guiData::Vec4f:{
			Vec4 unValue;
			if (value.type() == typeid(double) && valueVec.size() == 4) {
				unValue.x = std::any_cast<double>(valueVec.at(0));
				unValue.y = std::any_cast<double>(valueVec.at(1));
				unValue.z = std::any_cast<double>(valueVec.at(2));
				unValue.w = std::any_cast<double>(valueVec.at(3));
				uniformValue = unValue;
				result = true;
			}
			break;
	    }
		case raco::guiData::Vec2i: {
			Vec2int unValue;
			if (value.type() == typeid(int) && valueVec.size() == 2) {
				unValue.x = std::any_cast<int>(valueVec.at(0));
				unValue.y = std::any_cast<int>(valueVec.at(1));
				uniformValue = unValue;
				result = true;
			}
			break;
		}
		case raco::guiData::Vec3i: {
			Vec3int unValue;
			if (value.type() == typeid(int) && valueVec.size() == 3) {
				unValue.x = std::any_cast<int>(valueVec.at(0));
				unValue.y = std::any_cast<int>(valueVec.at(1));
				unValue.z = std::any_cast<int>(valueVec.at(2));
				uniformValue = unValue;
				result = true;
			}
			break;
		}
		case raco::guiData::Vec4i: {
			Vec4int unValue;
			if (value.type() == typeid(int) && valueVec.size() == 4) {
				unValue.x = std::any_cast<int>(valueVec.at(0));
				unValue.y = std::any_cast<int>(valueVec.at(1));
				unValue.z = std::any_cast<int>(valueVec.at(2));
				unValue.w = std::any_cast<int>(valueVec.at(3));
				uniformValue = unValue;
				result = true;
			}
			break;
		}
		default:
			break;
	}

	return result;
}

void MateralLogic::setUniformsMultiElementProperty(core::ValueHandle valueHandle, MaterialData &materialData, UniformType type) {
    using PrimitiveType = core::PrimitiveType;
    Uniform tempUniform;
    std::string property = valueHandle.getPropName();
    tempUniform.setName(property);
    tempUniform.setType(type);

    std::vector<std::any> valueVec;
    for (int i{0}; i < valueHandle.size(); i++) {
        raco::core::ValueHandle tempHandle = valueHandle[i];
        switch (tempHandle.type()) {
            case PrimitiveType::String: {
                break;
            }
            case PrimitiveType::Bool: {
                break;
            }
            case PrimitiveType::Int: {
                valueVec.push_back(tempHandle.asInt());
                break;
            }
            case PrimitiveType::Double: {
                valueVec.push_back(tempHandle.asDouble());
                break;
            }
            default: {
                break;
            }
        };
    }
	std::any uniformValue;
    if (vec2UniformValue(valueVec, uniformValue, type)) {
		tempUniform.setValue(uniformValue);
		materialData.addUniform(tempUniform);
    }
}
}
