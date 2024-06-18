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

#include "components/RaCoPreferences.h"
#include "application/RaCoApplication.h"

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>

namespace raco::components {
class RaCoPreferences;
};

namespace raco::common_widgets {

class PreferencesView final : public QDialog {
	Q_OBJECT
public:
	explicit PreferencesView(application::RaCoApplication* racoApp, QWidget* parent);

	bool dirty();

	Q_SLOT void save();
	Q_SIGNAL void dirtyChanged(bool dirty);

private:
	application::RaCoApplication* racoApp_;

	QLineEdit* userProjectEdit_;
	QSpinBox* featureLevelEdit_;
	QCheckBox* uriValidationCaseSensitiveCheckbox_;
	QCheckBox* preventAccidentalUpgradeEdit_;
	QCheckBox* preventAccidentalUpgradeCheckbox_;
	QLineEdit* screenshotDirectoryEdit_;
	QLineEdit* globalPythonScriptEdit_;
	QCheckBox* projectPythonScriptCheckbox_;

	QString convertPathToAbsolute(const QString& path) const;
};

}  // namespace raco::common_widgets
