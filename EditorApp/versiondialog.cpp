/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "versiondialog.h"
#include "ui_versiondialog.h"
#include "ramses_base/Utils.h"

#ifndef RAMSES_VERSION
#define RAMSES_VERSION "?.?.?"
#endif

#ifndef RLOGIC_VERSION
#define RLOGIC_VERSION "?.?.?"
#endif

#ifndef RACO_OSS_COMMIT
#define RACO_OSS_COMMIT "???"
#endif

#ifndef RACO_OSS_VERSION
#define RACO_OSS_VERSION "?.?.?"
#endif

VersionDialog::VersionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VersionDialog)
{
    ui->setupUi(this);
	ui->ramsesComposerVersion->setText(QString(RACO_OSS_VERSION));
	ui->ramsesComposerCommit->setText(QString(RACO_OSS_COMMIT));
	ui->ramsesVersion->setText(QString::fromStdString(raco::ramses_base::getRamsesVersionString()));
	ui->ramsesBuiltVersion->setText(QString(RAMSES_VERSION));
	ui->logicEngineVersion->setText(QString::fromStdString(raco::ramses_base::getLogicEngineVersionString()));
	ui->logicEngineBuiltVersion->setText(QString(RLOGIC_VERSION));
}

VersionDialog::~VersionDialog()
{
    delete ui;
}
 