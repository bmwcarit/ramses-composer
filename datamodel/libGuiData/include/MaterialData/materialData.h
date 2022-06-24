#ifndef MATERIALDATA_H
#define MATERIALDATA_H
#include <string>
#include <any>
#include <vector>
#include <iostream>
#include <QDebug>

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
    M_TEWinding_CounterClockWise   // 逆时针
};

enum Culling {  // 对应Cull Mode
					  //    TEFace_None = 0,
    CU_Disabled = 0,
	CU_Front,
	CU_Back,
	CU_FrontAndBack
};

enum DepthCompare {  // Depth Function
    // TECompareFunction_Always = 0,  // 总是有比较函数
    DC_Disabled = 0,
	DC_GreaterThan,		// >
	DC_GreaterOrEqualTo,  // >=
	DC_LessThan,		  // <
	DC_LessThanOrEqualTo,  // <=
	DC_Equal,			   // =
	DC_NotEqual,		   // !=
	DC_True,			   // 真
	DC_False			   // 假
};

enum Filter {	// 已补充
    Linear = 0,
    NearestMipMapNearest,
    NearestMipMapLinear,
    LinearMipMapNearest,
    LinearMipMapLinear
};

enum WrapMode {  // 已补充
    Clamp = 0,
    Repeat,
    Mirror
};

enum BlendOperation {	// 已补充
    BO_Disabled = 0,
    BO_Add,
    BO_Subtract,
    BO_ReverseSub,
    BO_Min,
    BO_Max
};

enum BlendFactor {	 // 已补充
    Zero = 0,
    One,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor
};

class Blending {   // ok 和Options对应
public:
	Blending() : blendOperationColor_(BO_Disabled), blendOperationAlpha_(BO_Disabled), srcColorFactor_(Zero)
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

class ColorWrite {  // Blend Color
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
    std::string name_;    // objectName
    std::string bitmapRef_;  // ok
    Filter minFilter_;    // ok
    Filter magFilter_;    // ok
    int anisotropicSamples_;  // ???
    WrapMode wrapModeU_;        // ok
    WrapMode wrapModeV_;        // ok
    std::string uniformName_;   // ok
    std::string bitmapsRef_;    // 和bitmap关联
};

class Uniform {
public:
    Uniform() : name_(""), value_(0) {}

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
    UniformType type_;  // 可能是三维向量也可能是一个float等
};


class RenderMode {
public:
	RenderMode() : winding_(M_TEWinding_CounterClockWise), culling_(CU_Disabled), depthCompareFunction_(DC_Disabled), depthWrite_(false) {}

    void setWindingType(WindingType type) {
        winding_ = type;
    }
    WindingType getWindingType() {
        return winding_;
    }

    Blending getBlending() {
        return blending_;
    }

    ColorWrite getColorWrite() {
        return colorWrite_;
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
    WindingType winding_;   // ?????????
    Culling culling_;   // ok
    Blending blending_;     // ok  和Options对应

    DepthCompare depthCompareFunction_; // ok
    bool depthWrite_;        // ok
    ColorWrite colorWrite_; // ok  BlendColor
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

private:
    std::string name_;               // ObjectName
    RenderMode renderMode_;          // ok

    std::string shaderRef_;          // 和shader关联
    std::vector<TextureData> textures_;
    std::vector<Uniform> uniforms_;  // 不一定
};
}

#endif // MATERIALDATA_H
