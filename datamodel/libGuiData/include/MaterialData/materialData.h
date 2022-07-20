#ifndef MATERIALDATA_H
#define MATERIALDATA_H
#include <string>
#include <any>
#include <vector>
#include <iostream>
#include <QDebug>

#include "MeshData/MeshDataManager.h"

namespace raco::guiData {

class Shader;
class Bitmap;


enum UniformType {
    Null = -1,
    Bool,
    Int,
    Double,
    String,
    Ref,
    Table,
    Vec2f,
    Vec3f,
    Vec4f,
    Vec2i,
    Vec3i,
    Vec4i,
    Struct
};

enum WindingType{
	M_TEWinding_ClockWise,
	M_TEWinding_CounterClockWise
};

enum Culling {
	CU_Front,
	CU_Back,
	CU_FrontAndBack,
	CU_None
};
// Depth Function
enum DepthCompare {
    DC_Disabled,
	DC_GreaterThan,
	DC_GreaterOrEqualTo,
	DC_LessThan,
	DC_LessThanOrEqualTo,
	DC_Equal,
	DC_NotEqual,
	DC_True,
	DC_False

};

enum Filter {
	Nearest,
    Linear,
    NearestMipMapNearest,
    NearestMipMapLinear,
    LinearMipMapNearest,
    LinearMipMapLinear
};

enum WrapMode {
    Clamp,
    Repeat,
    Mirror
};

enum BlendOperation {
    BO_None,
    BO_Add,
    BO_Subtract,
    BO_ReverseSub,
    BO_Min,
    BO_Max
};

enum BlendFactor {
	Zero,
	One,
	SrcAlpha,
	OneMinusSrcAlpha,
	DstAlpha,
	OneMinusDstAlpha,
	SrcColor,
	OneMinusSrcColor,
	DstColor,
	OneMinusDstColor,
	ConstColor,
	OneMinusConstColor,
	ConstAlpha,
	OneMinusConstAlpha,
	AlphaSaturated
};

class Blending {
public:
	Blending() : blendOperationColor_(BO_None), blendOperationAlpha_(BO_None), srcColorFactor_(Zero)
        , srcAlphaFactor_(Zero), desColorFactor_(Zero), desAlphaFactor_(Zero) {}

    void setBlendOperationColor(BlendOperation operation) {
        blendOperationColor_ = operation;
    }
    BlendOperation getBlendOperationColor() {
        return blendOperationColor_;
    }

    void setBlendOperationAlpha(BlendOperation operation) {
        blendOperationAlpha_ = operation;
    }
    BlendOperation getBlendOperationAlpha() {
        return blendOperationAlpha_;
    }

    void setSrcColorFactor(BlendFactor factor) {
        srcColorFactor_ = factor;
    }
    BlendFactor getSrcColorFactor() {
        return srcColorFactor_;
    }

    void setSrcAlphaFactor(BlendFactor factor) {
        srcAlphaFactor_ = factor;
    }
    BlendFactor getSrcAlphaFactor() {
        return srcAlphaFactor_;
    }

    void setDesColorFactor(BlendFactor factor) {
        desColorFactor_ = factor;
    }
    BlendFactor getDesColorFactor() {
        return desColorFactor_;
    }

    void setDesAlphaFactor(BlendFactor factor) {
        desAlphaFactor_ = factor;
    }
    BlendFactor getDesAlphaFactor() {
        return desAlphaFactor_;
    }

    void traverseBlending() {
        qDebug() << "blendOperationColor_:" << (int)blendOperationColor_;
		qDebug() << "blendOperationAlpha_:" << (int)blendOperationAlpha_;
		qDebug() << "srcColorFactor_:" << (int)srcColorFactor_;
		qDebug() << "srcAlphaFactor_:" << (int)srcAlphaFactor_;
		qDebug() << "desColorFactor_:" << (int)desColorFactor_;
		qDebug() << "desAlphaFactor_:" << (int)desAlphaFactor_;
    }

private:
    BlendOperation blendOperationColor_;
    BlendOperation blendOperationAlpha_;
    BlendFactor srcColorFactor_;
    BlendFactor srcAlphaFactor_;
    BlendFactor desColorFactor_;
    BlendFactor desAlphaFactor_;
};
// Blend Color
class ColorWrite {
public:
	ColorWrite() : red_(0), green_(0), blue_(0), alpha_ (0){}

    void setRed(int red) {
        red_ = red;
    }
    int getRed() {
        return red_;
    }

    void setGreen(int green) {
        green_ = green;
    }
    int getGreen() {
        return green_;
    }

    void setBlue(int blue) {
        blue_ = blue;
    }
    int getBlue() {
        return blue_;
    }

    void setAlpha(int alpha) {
        alpha_ = alpha;
    }
    int getAlpha() {
        return alpha_;
    }

    void traverseColorWrite() {
        qDebug() << "red:" << red_;
        qDebug() << "green_:" << green_;
        qDebug() << "blue_:" << blue_;
        qDebug() << "alpha_:" << alpha_;
    }

private:
    int red_;
    int green_;
    int blue_;
    int alpha_;
};


class Shader {
public:
	Shader() : name_(""), vertexShader_(""), fragmentShader_("") {}

    void setName(std::string name) {
        name_ = name;
    }
    std::string getName() {
        return name_;
    }

    void setPtxShaderName(std::string ptxName) {
		ptxShaderName_ = ptxName;
	}
	std::string getPtxShaderName() {
		return ptxShaderName_;
	}

    void setVertexShader(std::string vertexShader) {
        vertexShader_ = vertexShader;
    }
    std::string getVertexShader() {
        return vertexShader_;
    }
    void setFragmentShader(std::string fragmentShader) {
        fragmentShader_ = fragmentShader;
    }
    std::string getFragmentShader() {
        return fragmentShader_;
    }

    void traverseShader() {
        qDebug() << "shader name:" << QString::fromStdString(name_);
        qDebug() << "shader vertexShader_:" << QString::fromStdString(vertexShader_);
        qDebug() << "shader fragmentShader_:" << QString::fromStdString(fragmentShader_);
    }

private:
	std::string ptxShaderName_;
    std::string name_;
    std::string vertexShader_;
    std::string fragmentShader_;
};

class Bitmap {
public:
	Bitmap() : name_(""), resource_(""), generateMipmaps_(false) {}

    void setName(std::string name) {
        name_ = name;
    }
    std::string getName() {
        return name_;
    }
    void setResource(std::string resource) {
        resource_ = resource;
    }
    std::string getResource() {
        return resource_;
    }
    void setGenerateMipmaps(bool generateMipmaps) {
        generateMipmaps_ = generateMipmaps;
    }
    bool getGenerateMipmaps() {
        return generateMipmaps_;
    }

    void traverseBitmap() {
        qDebug() << "bitmap name:" << QString::fromStdString(name_);
        qDebug() << "bitmap resource_:" << QString::fromStdString(resource_);
        qDebug() << "bitmap generateMipmaps_:" << generateMipmaps_;
    }

private:
    std::string name_;
    std::string resource_;
    bool generateMipmaps_;
};

class TextureData {
public:
	TextureData() : name_(""), bitmapRef_(""), minFilter_(Linear), 
        magFilter_(Linear), anisotropicSamples_(0), wrapModeU_(Clamp), wrapModeV_(Clamp), uniformName_("") {}

    void setName(std::string name) {
        name_ = name;
    }
    std::string getName() {
        return name_;
    }
    void setBitmapRef(std::string bitmapRef) {
        bitmapRef_ = bitmapRef;
    }
    std::string getBitmapRef() {
        return bitmapRef_;
    }

    void setMinFilter(Filter minFilter) {
        minFilter_ = minFilter;
    }
    Filter getMinFilter() {
        return minFilter_;
    }

    void setMagFilter(Filter magFilter) {
        magFilter_ = magFilter;
    }
    Filter getMagFilter() {
        return magFilter_;
    }

    void setAnisotropicSamples(int samples) {
        anisotropicSamples_ = samples;
    }
    int getAnisotropicSamples() {
        return anisotropicSamples_;
    }

    void setWrapModeU(WrapMode mode) {
        wrapModeU_ = mode;
    }
    WrapMode getWrapModeU() {
        return wrapModeU_;
    }

    void setWrapModeV(WrapMode mode) {
        wrapModeV_ = mode;
    }
    WrapMode getWrapModeV() {
        return wrapModeV_;
    }

    void setUniformName(std::string name) {
        uniformName_ = name;
    }
    std::string getUniformName() {
        return uniformName_;
    }

    void setBitmapsRef(std::string bitmapsRef) {
        bitmapsRef_ = bitmapsRef;
    }
    std::string getBitmapsRef() {
        return bitmapsRef_;
    }

    void traverseTextureData() {
        qDebug() << "name_:" << QString::fromStdString(name_);
        qDebug() << "bitmapRef_:" << QString::fromStdString(bitmapRef_);
        qDebug() << "minFilter_:" << minFilter_;
        qDebug() << "magFilter_:" << magFilter_;
        qDebug() << "anisotropicSamples_:" << anisotropicSamples_;
        qDebug() << "wrapModeU_:" << wrapModeU_;
        qDebug() << "wrapModeV_:" << wrapModeV_;
        qDebug() << "uniformName_:" << QString::fromStdString(uniformName_);
    }

private:
    std::string name_;      // objectName
    std::string bitmapRef_;
    Filter minFilter_;
    Filter magFilter_;
    int anisotropicSamples_;
    WrapMode wrapModeU_;
    WrapMode wrapModeV_;
    std::string uniformName_;
    std::string bitmapsRef_;
};

class Uniform {
public:
	Uniform() : name_(""), value_(0), type_(UniformType::Bool) {}

    void setName(std::string name) {
        name_ = name;
    }
    std::string getName() {
        return name_;
    }
    void setValue(std::any value) {
        value_ = value;
    }
    std::any getValue() {
        return value_;
    }
    void setType(UniformType type) {
        type_ = type;
    }
    UniformType getType() {
        return type_;
    }

    void traverseUniform() {
        qDebug() << "uniform name_:" << QString::fromStdString(name_);
        qDebug() << "uniform type_:" << type_;
    }

private:
    std::string name_;
    std::any value_;
    UniformType type_;
};


class RenderMode {
public:
	RenderMode() : winding_(M_TEWinding_CounterClockWise), culling_(CU_None), depthCompareFunction_(DC_Disabled), depthWrite_(false) {}

    void setWindingType(WindingType type) {
        winding_ = type;
    }
    WindingType getWindingType() {
        return winding_;
    }

    void setBlending(Blending blending) {
        blending_ = blending;
    }
    Blending getBlending() {
        return blending_;
    }

    ColorWrite getColorWrite() {
        return colorWrite_;
    }

    void setColorWrite(ColorWrite& colorWrite) {
		colorWrite_ = colorWrite;
    }

    void setDepthWrite(bool bWrite) {
		depthWrite_ = bWrite;
    }
    bool getDepthWrite() {
		return depthWrite_;
    }

    void setDepthCompare(DepthCompare depthCompare) {
        depthCompareFunction_ = depthCompare;
    }
    DepthCompare getDepthCompare() {
        return depthCompareFunction_;
    }

    void setCulling(Culling culling) {
		int temp = culling;
		if (0 == temp) {
			qDebug() << culling;
        }
        culling_ = culling;
    }
    Culling getCulling() {
        return culling_;
    }

    void traverseRenderMode() {
        qDebug() << "WindingType:" << (int)winding_;
        qDebug() << "culling_:" << (int)culling_;
        blending_.traverseBlending();
        qDebug() << "depthCompareFunction_:" << (int)depthCompareFunction_;
		qDebug() << "depthWrite:" << depthWrite_;
        colorWrite_.traverseColorWrite();
    }

private:
    WindingType winding_;
    Culling culling_;
    Blending blending_;

    DepthCompare depthCompareFunction_;
    bool depthWrite_;
    ColorWrite colorWrite_;
 };


class MaterialData {
public:
	MaterialData();

    void setObjectName(std::string name) {
        name_ = name;
    }
    std::string getObjectName() {
        return name_;
    }

    std::string getDefaultID() {
		return defaultID_;
    }

    void setDefaultID(std::string Default) {
		defaultID_ = Default;
    }

    void setRenderMode(RenderMode renderMode) {
        renderMode_ = renderMode;
    }
    RenderMode getRenderMode() {
        return renderMode_;
    }

    void setShaderRef(std::string shaderRef) {
        shaderRef_ = shaderRef;
    }
    std::string getShaderRef() {
        return shaderRef_;
    }

    void addTexture(TextureData texture) {
        textures_.push_back(texture);
    }
    std::vector<TextureData> getTextures() {
        return textures_;
    }

    void addUniform(Uniform uniform) {
        uniforms_.push_back(uniform);
    }
    std::vector<Uniform> getUniforms() {
        return uniforms_;
    }

    void traverseMaterial() {
        qDebug() << "object name:" << QString::fromStdString(name_);
        renderMode_.traverseRenderMode();
        qDebug() << "shaderRef_:" << QString::fromStdString(shaderRef_);
        for (auto texture : textures_) {
            texture.traverseTextureData();
        }
        for (auto uniform : uniforms_) {
            uniform.traverseUniform();
        }
    }

    int getUsedAttributeSize() {
		return usedAttributes_.size();
	}

	void addUsedAttribute(Attribute attr) {
		usedAttributes_.push_back(attr);
	}

	std::vector<Attribute> getUsedAttributes() {
		return usedAttributes_;
	}

private:
	std::string defaultID_;
    std::string name_;
    RenderMode renderMode_;

    std::string shaderRef_;
    std::vector<TextureData> textures_;
    std::vector<Uniform> uniforms_;
	std::vector<Attribute> usedAttributes_;
};
}

#endif // MATERIALDATA_H
