/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "application/RaCoApplication.h"
#include <QCheckBox>
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
	Q_SLOT void updateButtonStates();

	QGridLayout* layout_;
	QCheckBox* compressEdit_;
	QLineEdit* pathEdit_;
	QLineEdit* ramsesEdit_;
	QLineEdit* logicEdit_;
	QDialogButtonBox* buttonBox_;

	bool hasErrors_;

	application::RaCoApplication* application_;
};

}  // namespace raco::common_widgets
