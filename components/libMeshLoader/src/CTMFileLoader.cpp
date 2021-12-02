/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "mesh_loader/CTMFileLoader.h"

#include "mesh_loader/CTMMesh.h"

#include <openctmpp.h>

namespace {

	std::string errorCodeToString(CTMenum errorCode) {
		switch (errorCode) {
		case CTM_INVALID_CONTEXT:
			return "The OpenCTM context was invalid";
		case CTM_INVALID_ARGUMENT:
			return "A function argument was invalid";
		case CTM_INVALID_OPERATION:
			return "The operation is not allowed";
		case CTM_INVALID_MESH:
			return "The mesh was invalid";
		case CTM_OUT_OF_MEMORY:
			return "Not enough memory to proceed";
		case CTM_FILE_ERROR:
			return "File I/O error";
		case CTM_BAD_FORMAT:
			return "File format error";
		case CTM_LZMA_ERROR:
			return "An error occured within the LZMA library.";
		case CTM_INTERNAL_ERROR:
			return "An internal error occured";
		case CTM_UNSUPPORTED_FORMAT_VERSION:
			return "Unsupported file format version";
		default:
			return "";
		}
	}

}

namespace raco::mesh_loader {

CTMFileLoader::CTMFileLoader(std::string absPath) : path_(absPath), valid_{false} {
}

void CTMFileLoader::reset() {
	importer_.reset();
	error_.clear();
	valid_ = false;
}

raco::core::MeshScenegraph* CTMFileLoader::getScenegraph(const std::string& absPath) {
	// Scenegraph import for CTM is unsupported as CTM does not contain any scenegraph.
	return nullptr;
}

int CTMFileLoader::getTotalMeshCount() {
	return 1;
}

std::shared_ptr<raco::core::MeshAnimationSamplerData> CTMFileLoader::getAnimationSamplerData(const std::string& absPath, int animIndex, int samplerIndex) {
	// CTM has no animations yet
	return {};
}

bool CTMFileLoader::loadFile() {
	if (!importer_) {
		importer_ = std::make_unique<CTMimporter>();
		try {
			importer_->Load(path_.c_str());
			valid_ = true;
		} catch (ctm_error const& e) {
			error_ = errorCodeToString(e.error_code());
			valid_ = false;
		};
	}
	return valid_;
}



raco::core::SharedMeshData CTMFileLoader::loadMesh(const raco::core::MeshDescriptor& descriptor) {
	if (loadFile()) {
		return std::make_shared<CTMMesh>(*importer_.get());
	}
	return raco::core::SharedMeshData();
}

std::string CTMFileLoader::getError() {
	return error_;
}

}  // namespace raco::mesh_loader