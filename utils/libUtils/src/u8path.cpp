/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "utils/u8path.h"

#include <fstream>
#include <algorithm>
#include <cassert>

namespace raco::utils {

u8path::u8path() : path_() {}

u8path::u8path(const path& path) : path_(path) {}

u8path::u8path(const std::string& path) : path_(std::filesystem::u8path(path)) {}

u8path::u8path(const char* path) : path_(std::filesystem::u8path(path)) {}

u8path& u8path::operator/=(const u8path& p) {
	path_ /= p;
	return *this;
}

u8path& u8path::append(const std::string& source) {
	path_ /= u8path(source);
	return *this;
}

u8path& u8path::concat(const std::string& source) {
	path_ += u8path(source);
	return *this;
}

u8path operator/(const u8path& lhs, const std::filesystem::path& rhs) {
	return u8path(lhs.path_ / rhs);
}

u8path operator/(const u8path& lhs, const std::string& rhs) {
	return lhs / u8path(rhs);
}

u8path operator/(const u8path& lhs, const char* rhs) {
	return lhs / u8path(rhs);
}

bool operator==(const u8path& lhs, const u8path& rhs) {
	return lhs.path_ == rhs.path_;
}

bool operator!=(const u8path& lhs, const u8path& rhs) {
	return lhs.path_ != rhs.path_;
}	

bool operator<(const u8path& lhs, const u8path& rhs) {
	return lhs.path_ < rhs.path_;
}

bool u8path::contains(const u8path& path) {
	return path.string().find(string()) == 0;
}

const std::filesystem::path& u8path::internalPath() const {
	return path_;
}

 std::string u8path::string() const {
	return path_.generic_u8string();
}

 std::wstring u8path::wstring() const {
	return path_.generic_wstring();
}

 bool u8path::empty() const {
	return path_.empty();
}

 bool u8path::exists(bool caseSensitive) const {
	std::error_code ec;
	auto status = std::filesystem::status(path_, ec);
	if (!ec) {
#ifdef _WIN32
		// enforce case-sensitive path name check for Windows systems to unify behavior with Linux builds
		if (std::filesystem::exists(status)) {
			if (caseSensitive) {
				return std::filesystem::canonical(path_, ec) == path_;
			}

			return true;
		}

		return false;
#else
		return std::filesystem::exists(status);
#endif
	}

	return false;
}

 bool u8path::existsDirectory() const {
	return exists() && std::filesystem::is_directory(path_);
}

 bool u8path::existsFile() const {
	return exists() && !is_directory(path_);
 }

 bool u8path::userHasReadAccess() const {
	 std::ifstream stream(path_);
	 return stream.good();
 }

 u8path u8path::normalized() const {
	 return path_.lexically_normal();
 }

 u8path u8path::normalizedRelativePath(const u8path& basePath) const {
	 assert(basePath.is_absolute());
	 if (is_relative()) {
		 return normalized();
	 } else {
		 // use proximate() call with std::error_code to prevent exceptions and return the input path instead.
		 std::error_code ec;

		 return std::filesystem::proximate(path_, basePath.path_, ec);	 
	 }
}

u8path u8path::normalizedAbsolutePath(const u8path& basePath) const {
	assert(basePath.is_absolute());
	if (is_absolute()) {
		return normalized();
	} else {
		return (basePath / path_).normalized();
	}
}

u8path u8path::rerootRelativePath(const u8path& oldBasePath, const u8path& newBasePath) {
	return normalizedAbsolutePath(oldBasePath).normalizedRelativePath(newBasePath);
}

bool u8path::areSharingSameRoot(const u8path& lhd, const u8path& rhd) {
	auto leftRoot = lhd.root_name().string();
	auto rightRoot = rhd.root_name().string();
	std::transform(leftRoot.begin(), leftRoot.end(), leftRoot.begin(), tolower);
	std::transform(rightRoot.begin(), rightRoot.end(), rightRoot.begin(), tolower);

	return leftRoot == rightRoot;
}

std::string u8path::sanitizePathString(const std::string& path) {
	const auto whitespaceCharacters = " \t\n\r\f\v";
	const auto trimmedStringLeft = path.find_first_not_of(whitespaceCharacters);
	const auto trimmedStringRight = path.find_last_not_of(whitespaceCharacters);

	if (trimmedStringLeft >= trimmedStringRight) {
		return {};
	}

	const auto trimmedString = path.substr(trimmedStringLeft, trimmedStringRight - trimmedStringLeft + 1);
	return u8path(trimmedString).normalized().string();
}

u8path u8path::current() {
	return std::filesystem::current_path();
}

 u8path u8path::parent_path() const {
	return path_.parent_path();
}

 u8path u8path::extension() const {
	return path_.extension();
}

 u8path u8path::stem() const {
	return path_.stem();
}

 u8path u8path::root_path() const {
	return path_.root_path();
}

 u8path u8path::root_name() const {
	return path_.root_name();
}

 u8path u8path::filename() const {
	return path_.filename();
}

 bool u8path::is_absolute() const {
	return path_.is_absolute();
}

 bool u8path::is_relative() const {
	return path_.is_relative();
}

 u8path& u8path::replace_extension(const u8path& extension) {
	path_.replace_extension(extension.path_);
	return *this;
}

 u8path& u8path::replace_filename(const u8path& extension) {
	path_.replace_filename(extension.path_);
	return *this;
}

 u8path& u8path::remove_filename() {
	path_.remove_filename();
	return *this;
}

} // namespace raco::utils::path
