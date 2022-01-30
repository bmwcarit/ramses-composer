/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "utils/FileUtils.h"

#include "utils/u8path.h"
#include "utils/stdfilesystem.h"

#include "log_system/log.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace raco::utils::file {

std::string read(const u8path& path) {
	if (path.existsFile()) {
		
		std::ifstream in{path.internalPath(), std::ifstream::in};
		std::stringstream ss{};
		ss << in.rdbuf();
		in.close();
		return ss.str();
	} else {
		if (!path.empty()) {
			LOG_WARNING("UTILS", "file not found: {}", path.string());
		}
		return {};
	}
}

std::vector<unsigned char> readBinary(const u8path& path) {
	if (path.existsFile()) {
		std::ifstream in{path.internalPath(), std::ifstream::in | std::ifstream::binary};
		std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(in), {});
		in.close();
		return buffer;
	} else {
		if (!path.empty()) {
			LOG_WARNING("UTILS", "file not found: {}", path.string());
		}
		return {};
	}
}

void write(const u8path& path, const std::string& content) {
	if (!path.parent_path().existsDirectory()) {
		std::filesystem::create_directory(path.parent_path());
	}
	std::ofstream out{path.internalPath(), std::ifstream::out};
	out << content;
	out.close();
}

}  // namespace raco::utils::file
