/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <filesystem>
#include <spdlog/fmt/fmt.h>
#include <string>

namespace raco::utils {

// Wrapper class around std::filesystem::path
// Extends its functionality and ensures, that only std::filesystem::u8path is ever used as constructor, to avoid issues with non-ASCII letters.
class u8path {
	using path = std::filesystem::path;

private:
	path path_;

public:
	u8path();
	u8path(const path& path);
	u8path(const std::string& path);
	u8path(const char* path);

	// Operations to append and concat paths, ensuring that strings added are interpreted as UTF-8

	friend u8path operator/(const u8path& lhs, const path& rhs);
	friend u8path operator/(const u8path& lhs, const std::string& rhs);
	friend u8path operator/(const u8path& lhs, const char* rhs);
	u8path& operator/=(const u8path& p);
	u8path& append(const std::string& source);
	u8path& concat(const std::string& source);

	friend bool operator==(const u8path& lhs, const u8path& rhs);
	friend bool operator!=(const u8path& lhs, const u8path& rhs);
	friend bool operator<(const u8path& lhs, const u8path& rhs);

	// Check if the path argument denoting a file is contained in the current directory path either directly
	// or indirectly via nesting.
	bool contains(const u8path& path);

	// Allow implicit casting to to the internal std::filesystem::path, in order to conveniently pass this as parameter to IO functions.
	// On Linux, ifstream and ofstream struggle to use this implicit cast correctly for some reason. 
	// Thus, internalPath() is an explicit way to access the underlying std::filesystem::path
	
	operator path() const { return path_; }
	const path& internalPath() const;

	// Limit the ways ways to get std::string from this std::filesystem::path, both are unicode-aware.

	std::string string() const;
	std::wstring wstring() const;

	// Convenience methods to extend std::filesystem::path functionality

	bool exists(bool caseSensitive = false) const;
	bool existsDirectory() const;
	bool existsFile() const;
	bool userHasReadAccess() const;
		
	u8path normalized() const;
	// Construct relative path from relative or absolute file path.
	// Absolute file paths are returned normalized and relative to the basePath argument.
	// Relative file paths are returned normalized.
	u8path normalizedRelativePath(const u8path& basePath) const;
	// Construct absolute paths from relative or absolute file path.
	// Absolute file paths are returned normalized.
	// Relative file paths are returned normalized and absolute using the basePath.
	u8path normalizedAbsolutePath(const u8path& basePath) const;

	u8path rerootRelativePath(const u8path& oldBasePath, const u8path& newBasePath);

	static bool areSharingSameRoot(const u8path& lhd, const u8path& rhd);
	static std::string sanitizePathString(const std::string& path);
	static u8path current();

	// Wrapper functions for std::filesystem::path functionality

	bool empty() const;
	u8path parent_path() const;
	u8path extension() const;
	u8path stem() const;
	u8path root_path() const;
	u8path root_name() const;
	u8path filename() const;
	bool is_absolute() const;
	bool is_relative() const;	
	u8path& replace_extension(const u8path& extension);
	u8path& replace_filename(const u8path& extension);
	u8path& remove_filename();		

};

}  // namespace raco::utils::path


template <>
struct fmt::formatter<raco::utils::u8path> : formatter<string_view> {
	template <typename FormatContext>
	auto format(const raco::utils::u8path& c, FormatContext& ctx) {
		return formatter<string_view>::format(c.string(), ctx);
	}
};