/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "utils/ZipUtils.h"

#include "zip.h"


namespace raco::utils::zip {

std::string projectToZip(const char* fileContents, const char* projectFileName) {
	char* buffer = nullptr;
	size_t bufferSize = 0;

	struct zip_t* zip = zip_stream_open(nullptr, 0, ZIP_COMPRESSION_LEVEL, 'w');
	{
		zip_entry_open(zip, projectFileName);
		{
			zip_entry_write(zip, fileContents, strlen(fileContents));
		}
		zip_entry_close(zip);

		zip_stream_copy(zip, (void**)&buffer, &bufferSize);
	}

	if (zip_entries_total(zip) != 1) {
		delete buffer;
		return "";
	}

	zip_stream_close(zip);

	std::string out(&buffer[0], &buffer[0] + bufferSize);
	
	delete buffer;

	return out;
}

UnZipStatus zipToProject(const char* fileContents, int fileContentSize) {
	char* buffer = nullptr;
	size_t bufferSize = 0;

	struct zip_t* zip = zip_stream_open(fileContents, fileContentSize, 0, 'r');
	if (!zip) {
		return {false, "File was not able to be opened as a zip archive."};
	}

	if (zip_entries_total(zip) != 1) {
		return {false, "This archive does not contain one singular file - RaCo zip archives should contain only one singular project file"};
	}

	{
		// Open the only file in our RaCo zip project: the actual project file
		// Ignore file name parameter for edge case: renaming of archive
		auto errorcode = zip_entry_openbyindex(zip, 0);

		if (errorcode != 0) {
			zip_stream_close(zip);
			return {false, zip_strerror(errorcode)};
		} else {
			zip_entry_read(zip, (void**)&buffer, &bufferSize);
		}

		zip_entry_close(zip);
	}
	zip_stream_close(zip);

	std::string out(&buffer[0], &buffer[0] + bufferSize);

	delete buffer;

	return {true, out};
}

bool isZipFile(const std::string& fileContents) {
	return (fileContents.size() >= 4) && (fileContents[0] == 0x50 && fileContents[1] == 0x4b
		&& ((fileContents[2] == 0x03 && fileContents[3] == 0x04)
			|| (fileContents[2] == 0x05 && fileContents[3] == 0x06)
			|| (fileContents[2] == 0x07 && fileContents[3] == 0x08)));
}

}  // namespace raco::utils::zip
