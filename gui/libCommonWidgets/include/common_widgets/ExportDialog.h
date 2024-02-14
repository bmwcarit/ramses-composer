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

#include "PropertyBrowserButton.h"
#include "application/RaCoApplication.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLineEdit>

namespace raco::common_widgets {
class LogViewModel;

class ExportDialog final : public QDialog {
public:
	explicit ExportDialog(application::RaCoApplication* application, LogViewModel* logViewModel, QWidget* parent);

private:
	Q_SLOT void exportProject();
	Q_SLOT void updatePaths();
	void updateButtonStates();

	QGridLayout* layout_;
	QGridLayout* optionLayout_;
	QCheckBox* compressEdit_;
	QLineEdit* ramsesEdit_;
	QDialogButtonBox* buttonBox_;
	QComboBox* luaSavingModeCombo_;

	application::RaCoApplication* application_;
	void setupFilePickerButton(PropertyBrowserButton* button, QLineEdit* pathEdit, const std::string& fileType);
	utils::u8path getAbsoluteExportFilePath(QLineEdit* path);
};

}  // namespace raco::common_widgets
