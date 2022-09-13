#include "MaterialData/materialData.h"
#include <vector>

namespace raco::guiData {

MaterialData::MaterialData() : name_(""), shaderRef_(""), defaultID_("") {

}

NodeMaterial::NodeMaterial() : name_(""), isPrivate_(false) {

}

}
