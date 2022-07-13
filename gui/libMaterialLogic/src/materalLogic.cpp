#include "material_logic/materalLogic.h"
#include "MaterialData/materialManager.h"

namespace raco::material_logic { 
MateralLogic::MateralLogic(QObject *parent)
    : QObject{parent} {

}

void MateralLogic::Analyzing() {
    for (const auto &it : resourcesHandleReMap_) {
        MaterialData materialData;
        Shader shader;
		core::ValueHandle valueHandle = it.second;
        if (valueHandle.hasProperty("options")) {
            initMaterialProperty(valueHandle, materialData, shader);
            MaterialManager::GetInstance().addMaterialData(it.first, materialData);
            MaterialManager::GetInstance().addShader(shader.getName(), shader);
        }
    }
	MaterialManager::GetInstance().traverseMaterialData();
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
                    blending.setBlendOperationColor(static_cast<BlendOperation>(value));
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
                    blending.setSrcColorFactor(static_cast<BlendFactor>(value));
                }
            }
            if (QString::fromStdString(propName).compare("blendFactorDestColor") == 0) {
                if (tempHandle.type() == PrimitiveType::Int) {
                    int value = tempHandle.asInt();
                    blending.setDesColorFactor(static_cast<BlendFactor>(value));
                }
            }
            if (QString::fromStdString(propName).compare("blendFactorSrcAlpha") == 0) {
                if (tempHandle.type() == PrimitiveType::Int) {
                    int value = tempHandle.asInt();
                    blending.setSrcAlphaFactor(static_cast<BlendFactor>(value));
                }
            }
            if (QString::fromStdString(propName).compare("blendFactorDestAlpha") == 0) {
                if (tempHandle.type() == PrimitiveType::Int) {
                    int value = tempHandle.asInt();
                    blending.setDesAlphaFactor(static_cast<BlendFactor>(value));
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
                    // 和ptx中Culling取值对应，0代表的是4
					if (0 == value) {
						value = 4;
                    }
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
                if (!textureData.getName().empty()) {
                    materialData.addTexture(textureData);
                }
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
    tempUniform.setValue(valueVec);
    materialData.addUniform(tempUniform);
}
}
