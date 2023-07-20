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

#include <QObject>
#include <QSettings>
#include <QString>

namespace raco::components {

/**
 * Global RaCo Preferences
 */
class RaCoPreferences final : public QObject {
	Q_OBJECT
private:
	RaCoPreferences();

public:
	static void init() noexcept;
	static RaCoPreferences& instance() noexcept;

	bool save();
	void load();

	QString userProjectsDirectory{};

	QString imageSubdirectory;
	QString meshSubdirectory;
	QString scriptSubdirectory;
	QString interfaceSubdirectory;
	QString shaderSubdirectory;
	QString screenshotDirectory;

	int featureLevel;
	bool isUriValidationCaseSensitive;
	bool preventAccidentalUpgrade;
};

}  // namespace raco
