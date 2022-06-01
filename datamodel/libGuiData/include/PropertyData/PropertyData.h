#ifndef PROPERTYDATA_H
#define PROPERTYDATA_H

#include <list>
#include <mutex>
#include <string>
#include <any>
#include <map>

namespace raco::guiData {
enum EPropertyType {
    PROPERTY_TYPE_FLOAT,
    PROPERTY_TYPE_INT,
    PROPERTY_TYPE_BOOL,
    PROPERTY_TYPE_STRING,
    PROPERTY_TYPE_SAMPLETEXTURE,
    PROPERTY_TYPE_VEC2i,
    PROPERTY_TYPE_VEC3i,
    PROPERTY_TYPE_VEC4i,
    PROPERTY_TYPE_VEC2f,
    PROPERTY_TYPE_VEC3f,
    PROPERTY_TYPE_VEC4f,
    PROPERTY_TYPE_MAT2,
    PROPERTY_TYPE_MAT3,
    PROPERTY_TYPE_MAT4
};

class PropertyDataManager {
public:
    static PropertyDataManager& GetInstance();
    ~PropertyDataManager();
    PropertyDataManager(const PropertyDataManager&) = delete;
    PropertyDataManager& operator=(const PropertyDataManager&) = delete;

    std::map<std::string, EPropertyType> getCustomPropertyTypeMap();
    bool insertCustomProItem(const std::string &name, const EPropertyType& type);
    bool deleteCustomProItem(const std::string &name);
    bool modifyCustomProItem(const std::string &name, const EPropertyType& type);
    bool getCustomProItemType(const std::string &name, EPropertyType &type);

    std::map<std::string, EPropertyType> getSystemPropertyTypeMap();
    bool insertSystemProItem(const std::string &name, const EPropertyType& type);
    bool deleteSystemProItem(const std::string &name);

    std::map<std::string, EPropertyType> getAnimationPropertyTypeMap();
    bool insertAnimationProItem(const std::string &name, const EPropertyType& type);
    bool deleteAnimationProItem(const std::string &name);
private:
    PropertyDataManager();
private:
    std::map<std::string, EPropertyType> customPropertyTypeMap_;
    std::map<std::string, EPropertyType> systemPropertyTypeMap_;
    std::map<std::string, EPropertyType> animationPropertyTypeMap_;
};
}

#endif // PROPERTYDATA_H
