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

#include "application/RaCoApplication.h"
#include "components/DataChangeDispatcher.h"
#include "components/RaCoNameConstants.h"
#include "core/PathManager.h"
#include "core/ProjectMigration.h"
#include "log_system/log.h"
#include "ramses_adaptor/SceneBackend.h"
#include "ramses_widgets/RendererBackend.h"
#include "style/RaCoStyle.h"
#include "utils/CrashDump.h"
#include "utils/u8path.h"

#include <QApplication>
#include <QCommandLineParser>

void createStdOutConsole();

#ifdef _WIN32
#include <windows.h>
void createStdOutConsole() {
	if (AllocConsole()) {
		FILE* stream;
		freopen_s(&stream, "CONOUT$", "w", stdout);
		freopen_s(&stream, "CONOUT$", "w", stderr);
	}
}
#endif
#ifndef _WIN32
void createStdOutConsole() { /* NOOP */
}
#endif

int main(int argc, char* argv[]) {
	QCoreApplication::setApplicationName("Ramses Composer");
	QCoreApplication::setApplicationVersion(RACO_OSS_VERSION);

	// Enable Qt's virtualized coordinate system, which makes qt pixel size different from physical pixel size depending on the scale factor set in the operating system.
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	// By default, Qt will round the scale factor to the closest integer in some contexts. Disable rounding, since it produces invalid font sizes for Windows 125%, 150%, 175% etc. scaling mode.
	QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
	// Also, we need support for high resolution icons on scale factors greater than one.
	QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

	// QDialogs will show a "?"-Button by default. While it is possible to disable this for every single dialog, we never need this and thus, disabling it globally is easier.
	QApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);

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

	parser.process(QCoreApplication::arguments());

	bool noDumpFiles = parser.isSet(noDumpFileCheckOption);
	raco::utils::crashdump::installCrashDumpHandler(noDumpFiles);

	if (parser.isSet(consoleOption)) {
		createStdOutConsole();
	}

	auto appDataPath = raco::utils::u8path(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toStdString()).parent_path() / "RamsesComposer";
	raco::core::PathManager::init(QCoreApplication::applicationDirPath().toStdString(), appDataPath);
	raco::log_system::init(raco::core::PathManager::logFileEditorName().internalPath().native());

	const QStringList args = parser.positionalArguments();

	// support both loading with named parameter for compatibility with headless version and
	// loading with positional parameter drag&drop onto desktop icon
	QString projectFile{};
	if (parser.isSet(loadProjectAction)) {
		projectFile = QFileInfo(parser.value(loadProjectAction)).absoluteFilePath();
	} else if (args.size() > 0) {
		projectFile = QFileInfo(args.at(0)).absoluteFilePath();
	}

	// set font, must be done after application instance
	raco::style::RaCoStyle::installFont();

	auto ramsesCommandLineArgs = parser.value(forwardCommandLineArgs).toStdString();
	raco::ramses_widgets::RendererBackend rendererBackend{parser.isSet(forwardCommandLineArgs) ? ramsesCommandLineArgs : ""};

	std::unique_ptr<raco::application::RaCoApplication> app;

	try {
		app = std::make_unique<raco::application::RaCoApplication>(rendererBackend, projectFile, true);
	} catch (const raco::application::FutureFileVersion& error) {
		LOG_ERROR(raco::log_system::COMMON, "File load error: project file was created with newer file version {} but current file version is {}.", error.fileVersion_, raco::serialization::RAMSES_PROJECT_FILE_VERSION);
		app.reset();
	} catch (const raco::core::ExtrefError& error) {
		LOG_ERROR(raco::log_system::COMMON, "File Load Error: external reference update failed with error {}.", error.what());
		app.reset();
	} catch (const std::exception& error) {
		LOG_ERROR(raco::log_system::COMMON, "File Load Error: {}", error.what());
		app.reset();
	}

	if (app) {
		MainWindow w{app.get(), &rendererBackend};
		w.show();

		return a.exec();
	} else {
		exit(1);
	}
}
