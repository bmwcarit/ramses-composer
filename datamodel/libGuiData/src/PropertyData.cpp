#include "PropertyData/PropertyData.h"

namespace raco::guiData {
PropertyDataManager::PropertyDataManager() {

}

PropertyDataManager &PropertyDataManager::GetInstance() {
    static PropertyDataManager Instance;
    return Instance;
}

PropertyDataManager::~PropertyDataManager() {

}

std::map<std::string, EPropertyType> PropertyDataManager::getCustomPropertyTypeMap() {
    return customPropertyTypeMap_;
}

bool PropertyDataManager::insertCustomProItem(const std::string &name, const EPropertyType &type) {
    auto it = customPropertyTypeMap_.find(name);
    if (it == customPropertyTypeMap_.end()) {
        customPropertyTypeMap_.emplace(name, type);
        return true;
    }
    return false;
}

bool PropertyDataManager::deleteCustomProItem(const std::string &name) {
    auto it = customPropertyTypeMap_.find(name);
    if (it != customPropertyTypeMap_.end()) {
        customPropertyTypeMap_.erase(it);
        return true;
    }
    return false;
}

bool PropertyDataManager::modifyCustomProItem(const std::string &name, const EPropertyType &type) {
    auto it = customPropertyTypeMap_.find(name);
    if (it != customPropertyTypeMap_.end()) {
        it->second = type;
        return true;
    }
    return false;
}

bool PropertyDataManager::getCustomProItemType(const std::string &name, EPropertyType &type) {
    auto it = customPropertyTypeMap_.find(name);
    if (it != customPropertyTypeMap_.end()) {
        type = it->second;
        return true;
    }
    return false;
}

std::map<std::string, EPropertyType> PropertyDataManager::getSystemPropertyTypeMap() {
    return systemPropertyTypeMap_;
}

bool PropertyDataManager::insertSystemProItem(const std::string &name, const EPropertyType &type) {
    auto it = systemPropertyTypeMap_.find(name);
    if (it != systemPropertyTypeMap_.end()) {
        systemPropertyTypeMap_.emplace(name, type);
        return true;
    }
    return false;
}

bool PropertyDataManager::deleteSystemProItem(const std::string &name) {
    auto it = systemPropertyTypeMap_.find(name);
    if (it != systemPropertyTypeMap_.end()) {
        systemPropertyTypeMap_.erase(it);
        return true;
    }
    return false;
}

std::map<std::string, EPropertyType> PropertyDataManager::getAnimationPropertyTypeMap() {
    return animationPropertyTypeMap_;
}

bool PropertyDataManager::insertAnimationProItem(const std::string &name, const EPropertyType &type) {
    auto it = animationPropertyTypeMap_.find(name);
    if (it != animationPropertyTypeMap_.end()) {
        animationPropertyTypeMap_.emplace(name, type);
        return true;
    }
    return false;
}

bool PropertyDataManager::deleteAnimationProItem(const std::string &name) {
    auto it = animationPropertyTypeMap_.find(name);
    if (it != animationPropertyTypeMap_.end()) {
        animationPropertyTypeMap_.erase(it);
        return true;
    }
    return false;
}
}
