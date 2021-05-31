/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "mainwindow.h"

#include "core/PathManager.h"
#include "log_system/log.h"
#include "ramses_widgets/RendererBackend.h"
#include "ramses_adaptor/SceneBackend.h"
#include "utils/CrashDump.h"
#include "components/DataChangeDispatcher.h"
#include "application/RaCoApplication.h"
#include "components/RaCoNameConstants.h"
#include "style/RaCoStyle.h"

#include <QApplication>
#include <QCommandLineParser>

void createStdOutConsole();

#ifdef _WIN32
#include <windows.h>
void createStdOutConsole() {
	if (AllocConsole()) {
		FILE *stream;
		freopen_s(&stream, "CONOUT$", "w", stdout);
		freopen_s(&stream, "CONOUT$", "w", stderr);
	}
}
#endif
#ifndef _WIN32
void createStdOutConsole() { /* NOOP */
}
#endif

int main(int argc, char *argv[]) {
	QCoreApplication::setApplicationName("Ramses Composer");
	QCoreApplication::setApplicationVersion(RACO_OSS_VERSION);

	QCommandLineParser parser;
	parser.addHelpOption();
	parser.addVersionOption();
	parser.setApplicationDescription("Ramses Composer - interactive authoring tool for Ramses and Ramses Logic engines.");
	QCommandLineOption consoleOption(
		QStringList() << "c"
					  << "console",
		"Open with std out console.");
	QCommandLineOption forwardCommandLineArgs(
		QStringList() << "r"
					  << "ramses-framework-arguments",
		"Override arguments passed to the ramses framework.",
		"default-args");
	QCommandLineOption noDumpFileCheckOption(
		QStringList() << "d"
					  << "nodump",
		"Don't generate crash dumps on unhandled exceptions.");
	QCommandLineOption loadProjectAction(
		QStringList() << "p"
					  << "project",
		"Load a scene from specified path.",
		"project-path");
	parser.addOption(consoleOption);
	parser.addOption(forwardCommandLineArgs);
	parser.addOption(noDumpFileCheckOption);
	parser.addOption(loadProjectAction);

	// apply global style, must be done before application instance
	QApplication::setStyle(new raco::style::RaCoStyle());

	// application must be instantiated before parsing command line
	QApplication a(argc, argv);

	// force use of style palette, required on Linux
	a.setPalette(a.style()->standardPalette());

	QStringList argList{};
	for (int i{0}; i < argc; i++)
		argList << argv[i];
	parser.process(argList);

	bool noDumpFiles = parser.isSet(noDumpFileCheckOption);
	raco::utils::crashdump::installCrashDumpHandler(noDumpFiles);

	if (parser.isSet(consoleOption)) {
		createStdOutConsole();
	}

	raco::core::PathManager::init(QCoreApplication::applicationDirPath().toStdString());

	raco::log_system::init(raco::core::PathManager::logFilePath().c_str());

	const QStringList args = parser.positionalArguments();

	// support both loading with named parameter for compatibility with headless version and
	// loading with positional parameter drag&drop onto desktop icon
	QString projectFile{};
	{
		QFileInfo *projectFileCandidate = nullptr;
		if (parser.isSet(loadProjectAction)) {
			projectFileCandidate = new QFileInfo(parser.value(loadProjectAction));
		} else if (!projectFileCandidate && args.size() > 0 && args.at(0).endsWith(raco::names::PROJECT_FILE_EXTENSION, Qt::CaseInsensitive)) {
			projectFileCandidate = new QFileInfo(args.at(0));
		}
		if (projectFileCandidate) {
			if (projectFileCandidate->suffix().compare(raco::names::PROJECT_FILE_EXTENSION, Qt::CaseInsensitive) == 0) {
				if (projectFileCandidate->exists()) {
					projectFile = projectFileCandidate->absoluteFilePath();
					LOG_INFO(raco::log_system::COMMON, "starting Ramses Composer with project file {}", projectFile.toStdString());
				} else {
					LOG_ERROR(raco::log_system::COMMON, "project file not found {}", projectFileCandidate->filePath().toStdString());
				}
			} else {
				LOG_ERROR(raco::log_system::COMMON, "invalid file type specified as project file {}", projectFileCandidate->filePath().toStdString());
			}
			delete projectFileCandidate;
		}
	}


	// set font, must be done after application instance
	raco::style::RaCoStyle::installFont();

	auto ramsesCommandLineArgs = parser.value(forwardCommandLineArgs).toStdString();
	raco::ramses_widgets::RendererBackend rendererBackend{parser.isSet(forwardCommandLineArgs) ? ramsesCommandLineArgs : ""};
	raco::application::RaCoApplication app{rendererBackend, projectFile};

	MainWindow w{&app, &rendererBackend};
	w.show();
	return a.exec();
}
