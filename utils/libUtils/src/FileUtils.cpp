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

#include "utils/PathUtils.h"
#include "utils/stdfilesystem.h"

#include "log_system/log.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace raco::utils::file {

std::string read(const Path& path) {
	if (raco::utils::path::isExistingFile(path)) {
		std::ifstream in{path, std::ifstream::in};
		std::stringstream ss{};
		ss << in.rdbuf();
		in.close();
		return ss.str();
	} else {
		LOG_WARNING("UTILS", "file not found: {}", path);
		return {};
	}
}

std::vector<unsigned char> readBinary(const Path& path) {
	if (raco::utils::path::isExistingFile(path)) {
		std::ifstream in{path, std::ifstream::in | std::ifstream::binary};
		auto buffer = std::vector<unsigned char>{std::istream_iterator<unsigned char>(in), std::istream_iterator<unsigned char>()};
		in.close();
        return buffer;
	} else {
		LOG_WARNING("UTILS", "file not found: {}", path);
		return {};
	}
}

void write(const Path& path, const std::string& content) {
	std::filesystem::path p{path};
	if (!raco::utils::path::isExistingDirectory(p.parent_path().generic_string())) {
		std::filesystem::create_directory(p.parent_path());
	} 
	std::ofstream out{p, std::ifstream::out};
	out << content;
	out.close();
}

}  // namespace raco::utils::file
