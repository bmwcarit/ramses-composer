#include "NodeData/nodeDataEx.h"

namespace raco::guiData {

CurveBinding::CurveBinding() {
}

CurveBinding::~CurveBinding() {
    bindingMap_.clear();
}

std::map < std::string, std::map<std::string, std::string>> &CurveBinding::bindingMap() {
    return bindingMap_;
}

bool CurveBinding::insertAnimation(std::string &sampleProp, std::map<std::string, std::string> &bindingData) {
    auto ret = bindingMap_.emplace(sampleProp, bindingData);
    if (ret.second == false) {
        std::cout << "element [" << sampleProp << "] already existed";
        return false;
    }
    return true ;
}


bool CurveBinding::deleteAnimation(std::string &sampleProp) {
    auto iter = bindingMap_.find(sampleProp);
    if(iter == bindingMap_.end()){
        std::cout << "can't find [" << sampleProp << "] !\n";
        return false;
    }
    bindingMap_.erase(iter);
    return true;
}

bool CurveBinding::insertBindingDataItem(const std::string &sampleProp, const std::string &property, const std::string &curve) {
    auto iter = bindingMap_.find(sampleProp);
    if (iter == bindingMap_.end()) {
        std::map<std::string, std::string> bindingDataMap;
        bindingDataMap.emplace(property, curve);
        bindingMap_.emplace(sampleProp, bindingDataMap);
		return true;
    }
    iter->second.emplace(property, curve);
    return true;
}

bool CurveBinding::deleteBindingDataItem(const std::string& sampleProp, const std::string& property, const std::string& curve) {
    auto iter = bindingMap_.find(sampleProp);
    if (iter == bindingMap_.end()) {
        return false;
    }
    auto propIt = iter->second.find(property);
    if (propIt == iter->second.end()) {
        return false;
    }
    iter->second.erase(propIt);
    return true;
}

bool CurveBinding::getPropCurve(std::string sampleProp, std::map<std::string, std::string> &bindingData) {
    auto iter = bindingMap_.find(sampleProp);
    if(iter == bindingMap_.end()) {
        return false;
    }
    bindingData =iter->second;
    return true;
}

void CurveBinding::traverseCurveBinding() {
    for(auto& it : bindingMap_){
        std::cout << "sampleProp_ :"<< it.first << " \n bindingData: \n";
        for(auto &data: it.second)
            std::cout << "Property: "  << data.first << " Curve: " << data.second << std::endl;
    }
}


//////////////////////////////////////////////////////////
CustomProp::CustomProp()
{
}

CustomProp::~CustomProp()
{
    typeMap_.clear();
}

std::map<std::string, std::any>& CustomProp::customTypeMapRef()
{
    return typeMap_;
}

bool CustomProp::insertType(const std::string& name, const std::any& anyData)
{
	auto it = typeMap_.find(name);
	if (it != typeMap_.end()) {
		modifyData(name, anyData);
		return true;
	}
    auto ret = typeMap_.emplace(name, anyData);
    if (ret.second == false) {
        std::cout << "element [" << name << "] already existed " << std::endl;
        return false;
    }
    return true ;
}

bool CustomProp::deleteType(const std::string &name)
{
    auto iter = typeMap_.find(name);
    if(iter == typeMap_.end()){
        std::cout << "can't find [" << name << "] !\n";
        return false;
    }
    typeMap_.erase(iter);
    return true;
}

bool CustomProp::modifyData(const std::string &name, const std::any &value)
{
    auto iter = typeMap_.find(name);
    if(iter == typeMap_.end()){
        std::cout << "can't find [" << name << "] !\n";
        return false;
    }
    iter->second = value;
    return true;
}

bool CustomProp::hasType(const std::string &typeName) {
    auto iter = typeMap_.find(typeName);
    if(iter == typeMap_.end()){
        return false;
    }
    return true;
}

NodeExtend::NodeExtend()
{

}
}
