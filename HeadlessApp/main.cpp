/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "log_system/log.h"
#include "core/PathManager.h"
#include "ramses_adaptor/SceneBackend.h"
#include "ramses_base/HeadlessEngineBackend.h"
#include "utils/CrashDump.h"
#include "components/DataChangeDispatcher.h"
#include "application/RaCoApplication.h"
#include "components/RaCoNameConstants.h"

#include <QCoreApplication>
#include <QTimer>
#include <iostream>

class Worker : public QObject {
	Q_OBJECT

public:
	Worker(QObject* parent, QString& projectFile, QString& exportPath, bool compressExport)
		: QObject(parent), projectFile_(projectFile), exportPath_(exportPath), compressExport_(compressExport) {
	}

public Q_SLOTS:
	void run() {
		raco::ramses_base::HeadlessEngineBackend backend{};
		raco::application::RaCoApplication app{backend, projectFile_};

		if ( !exportPath_.isEmpty() ) {
			QString ramsesPath = exportPath_ + "." + raco::names::FILE_EXTENSION_RAMSES_EXPORT;
			QString logicPath = exportPath_ + "." + raco::names::FILE_EXTENSION_LOGIC_EXPORT;

			std::string error;
			if (!app.exportProject(app.activeRaCoProject(), ramsesPath.toStdString(), logicPath.toStdString(), compressExport_, error)) {
				LOG_ERROR(raco::log_system::COMMON, "error exporting to {}\n{}", error.c_str(), ramsesPath.toStdString());
			}
		}

		Q_EMIT finished();
	}

Q_SIGNALS:
	void finished();

	private:
	QString projectFile_;
	QString exportPath_;
	bool compressExport_;
};

#include "main.moc"

int main(int argc, char* argv[]) {
	QCoreApplication::setApplicationName("Ramses Composer Headless");
	QCoreApplication::setApplicationVersion(RACO_OSS_VERSION);

	QCommandLineParser parser;
	parser.addHelpOption();
	parser.addVersionOption();
	parser.setApplicationDescription("Ramses Composer Headless for command line use");

	QCommandLineOption loadProjectAction(
		QStringList() << "p"
					  << "project",
		"Load a scene from specified path.",
		"project-path");
	QCommandLineOption exportProjectAction(
		QStringList() << "e"
					  << "export",
		"Export Ramses scene and logic to path. File extensions are added automatically.",
		"export-path");
	QCommandLineOption compressExportAction(
		QStringList() << "c"
					  << "compress",
		"Compress Ramses scene on export.");
	QCommandLineOption noDumpFileCheckOption(
		QStringList() << "d"
					  << "nodump",
		"Don't generate crash dumps on unhandled exceptions.");
	parser.addOption(loadProjectAction);
	parser.addOption(exportProjectAction);
	parser.addOption(compressExportAction);
	parser.addOption(noDumpFileCheckOption);

	// application must be instantiated before parsing command line
	QCoreApplication a(argc, argv);

	QStringList argList{};
	for (int i{0}; i < argc; i++)
		argList << argv[i];
	parser.process(argList);

	bool noDumpFiles = parser.isSet(noDumpFileCheckOption);
	raco::utils::crashdump::installCrashDumpHandler(noDumpFiles);

	raco::core::PathManager::init(QCoreApplication::applicationDirPath().toStdString());

	raco::log_system::init(raco::core::PathManager::logFilePath().c_str());

	QString projectFile{};
	if (parser.isSet(loadProjectAction)) {
		QFileInfo path(parser.value(loadProjectAction));
		if (path.suffix().compare( raco::names::PROJECT_FILE_EXTENSION, Qt::CaseInsensitive) == 0) {
			if (path.exists()) {
				projectFile = path.absoluteFilePath();
			} else {
				LOG_ERROR(raco::log_system::COMMON, "project file not found {}", path.filePath().toStdString());
			}
		} else {
			LOG_ERROR(raco::log_system::COMMON, "invalid file type specified as project file {}", path.filePath().toStdString());
		}
	}

	QString exportPath{};
	bool compressExport = parser.isSet(compressExportAction);
	if (parser.isSet(exportProjectAction)) {
		QFileInfo path( parser.value(exportProjectAction));

		exportPath = path.absoluteFilePath();
		if (path.suffix().compare(raco::names::FILE_EXTENSION_RAMSES_EXPORT, Qt::CaseInsensitive) == 0) {
			exportPath.chop(static_cast<int>(strlen(raco::names::FILE_EXTENSION_RAMSES_EXPORT) + 1));
		} else if (path.suffix().compare(raco::names::FILE_EXTENSION_LOGIC_EXPORT, Qt::CaseInsensitive) == 0) {
			exportPath.chop(static_cast<int>(strlen(raco::names::FILE_EXTENSION_LOGIC_EXPORT) + 1));
		}
	}

	Worker* task = new Worker(&a, projectFile, exportPath, compressExport );
	QObject::connect(task, &Worker::finished, &QCoreApplication::quit);
	QTimer::singleShot(0, task, &Worker::run);

	return a.exec();
}
