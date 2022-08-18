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

#include "application/RaCoApplication.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>

namespace raco::common_widgets {
class RunScriptDialog final : public QDialog {
	Q_OBJECT
public:
	static constexpr auto ENTRIES_SIZE = 3;

	RunScriptDialog(std::map<QString, qint64>& scriptEntries, std::map<QString, qint64>& commandLineParamEntries, QWidget* parent = nullptr);

Q_SIGNALS:
	void pythonScriptRunRequested(const QString& pythonFilePath, const QStringList& arguments);

private:
	Q_SLOT void updateButtonStates();
	Q_SLOT void runScript();
	void updateComboBoxItems();

	QGridLayout* layout_;
	QComboBox* scriptPathEdit_;
	QComboBox* argumentsEdit_;
	QLabel* warningLabel_;
	QDialogButtonBox* buttonBox_;
	std::map<QString, qint64>& scriptEntries_;
	std::map<QString, qint64>& commandLineParamEntries_;
};

}  // namespace raco::common_widgets
