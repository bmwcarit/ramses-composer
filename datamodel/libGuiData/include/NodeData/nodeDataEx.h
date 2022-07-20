#ifndef NODEDATA_H
#define NODEDATA_H


#include <string>
#include <map>
#include <any>
#include <list>
#include <iostream>
#include "PropertyData/PropertyData.h"

namespace raco::guiData {


class CurveBinding
{
public:
    CurveBinding();
    ~CurveBinding();
	std::map<std::string, std::map<std::string, std::string>>& bindingMap();
	bool insertAnimation(std::string& sampleProp, std::map<std::string, std::string>& bindingData);
	bool deleteAnimation(std::string& sampleProp);
    bool insertBindingDataItem(const std::string& sampleProp, const std::string& property, const std::string& curve);
    bool deleteBindingDataItem(const std::string& sampleProp, const std::string& property, const std::string& curve);

	bool getPropCurve(std::string sampleProp, std::map<std::string, std::string>& bindingData);
    void traverseCurveBinding();
	size_t getBindingMapSize() { return bindingMap_.size(); }
private:
    std::map<std::string, std::map<std::string, std::string>>bindingMap_;
    //     < sampleProp_ ,  <property_, curve_> >
    //       animationName
};

class CustomProp
{
public:
    CustomProp();
    ~CustomProp();
    std::map<std::string,std::any>& customTypeMapRef();
    bool insertType(const std::string& name,const std::any& anyData);
    bool deleteType(const std::string& name);
    bool modifyData(const std::string& typeName ,const std::any& value);
	bool hasType(const std::string& typeName);
	int getCustomMapSize() {
		return typeMap_.size();
	}
    void traverseCustomProp() {
        for(auto& it : typeMap_){
            std::cout << "key :"<< it.first << " Item.anyData: " <<  std::any_cast<double>(it.second)
                      << std::endl;
        }
    }

private:
    std::map<std::string, std::any> typeMap_;
};

/////////////////////////////////////////////
class MeshProp{
public:
    MeshProp():
        name_(""),material_("") {}
    inline void modifyName(const std::string name) {
        name_ = name;
    }
    inline void modifyMaterial(const std::string material) {
        material_ = material;
    }
    inline const std::string& modifyName() {
        return name_;
    }
    inline const std::string& modifyMaterial() {
        return material_;
    }
    inline void displayMeshProp() {
        std::cout << "Name : " << name_ << "  Material: " << material_ << std::endl;
    }
private:
    std::string name_;      // 路径
    std::string material_;  // 材质名称
};

//////////////////////////////////////////////////////

class NodeExtend
{
public:
    NodeExtend();
    // Curve属性
    CurveBinding& curveBindingRef() {  // sampleproperty 索引curvebaning列表
        return curveBinding_;
    }

    void setCurveBinding(CurveBinding& cb) {
        curveBinding_ = cb;
    }

    // Custom属性
    CustomProp& customPropRef() {
        return customProp_;
    }

    void setCustomProp(CustomProp& cp) {
        customProp_ = std::move(cp);
    }

    // Mesh属性
    MeshProp& meshPropRef()  {
        return meshProp_;
    }
    void setMeshProp(MeshProp& mp) {
        meshProp_ = std::move(mp);
    }
	int getCustomPropSize() {
		return customProp_.getCustomMapSize();
	}
	size_t getCurveBindingSize() {
		return curveBinding_.getBindingMapSize();
	}
    void traverseNodeExtend(){
        std::cout << "\n ============ curveBinding_ ============== " << std::endl;
        curveBinding_.traverseCurveBinding();
        std::cout << "\n ============ customProp_ ============== " << std::endl;
        customProp_.traverseCustomProp();
        std::cout << "\n ============ meshProp_ ============== " << std::endl;
        meshProp_.displayMeshProp();
    }

private:
    CurveBinding curveBinding_;
    CustomProp customProp_;
    MeshProp meshProp_;
};
}

#endif // NODEDATA_H
