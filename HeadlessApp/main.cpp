/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "application/RaCoApplication.h"
#include "components/DataChangeDispatcher.h"
#include "components/RaCoNameConstants.h"
#include "core/PathManager.h"
#include "log_system/log.h"
#include "ramses_adaptor/SceneBackend.h"
#include "ramses_base/HeadlessEngineBackend.h"
#include "utils/CrashDump.h"
#include "utils/u8path.h"

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

spdlog::level::level_enum getLevelFromArg(const QString& arg) {
	bool logLevelValid;
	int logLevel = arg.toInt(&logLevelValid);
	spdlog::level::level_enum spdLogLevel;
	switch (logLevelValid ? logLevel : -1) {
		case 0:
			return spdlog::level::level_enum::off;
		case 1:
			return spdlog::level::level_enum::critical;
		case 2:
			return spdlog::level::level_enum::err;
		case 3:
			return spdlog::level::level_enum::warn;
		case 4:
			return spdlog::level::level_enum::info;
		case 5:
			return spdlog::level::level_enum::debug;
		case 6:
			return spdlog::level::level_enum::trace;
		default:
			LOG_ERROR(raco::log_system::COMMON, "Invalid Log Level: \"{}\". Continuing with verbose log output.", arg.toStdString().c_str());
			return spdlog::level::level_enum::trace;
	}
}

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
	QCommandLineOption logLevelOption(
		QStringList() << "l"
					  << "loglevel",
		"Maximum information level that should be printed as console log output. Possible options: 0 (off), 1 (critical), 2 (error), 3 (warn), 4 (info), 5 (debug), 6 (trace).",
		"log-level",
		"6");
	QCommandLineOption logFileOutputOption(
		QStringList() << "o"
					  << "outlogfile",
		"File name to write log file to.",
		"log-file-name",
		"");
	parser.addOption(loadProjectAction);
	parser.addOption(exportProjectAction);
	parser.addOption(compressExportAction);
	parser.addOption(noDumpFileCheckOption);
	parser.addOption(logLevelOption);
	parser.addOption(logFileOutputOption);

	// application must be instantiated before parsing command line
	QCoreApplication a(argc, argv);

	parser.process(QCoreApplication::arguments());

	bool noDumpFiles = parser.isSet(noDumpFileCheckOption);
	raco::utils::crashdump::installCrashDumpHandler(noDumpFiles);

	auto appDataPath = raco::utils::u8path(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toStdString()).parent_path() / "RamsesComposer";
	raco::core::PathManager::init(QCoreApplication::applicationDirPath().toStdString(), appDataPath);

	auto logLevel = getLevelFromArg(parser.value(logLevelOption));
	auto customLogFileName = parser.value(logFileOutputOption).toStdString();
	auto logFilePath = raco::utils::u8path(customLogFileName);
	bool customLogFileNameFailed = false;	

	if (!customLogFileName.empty()) {
		if (!logFilePath.parent_path().empty() && logFilePath.parent_path() != logFilePath && !logFilePath.parent_path().exists()) {
			std::filesystem::create_directories(logFilePath.parent_path());

			if (!logFilePath.parent_path().exists()) {
				logFilePath = raco::core::PathManager::logFileHeadlessName();
				customLogFileNameFailed = true;
			}
		}
	} else {
		logFilePath = raco::core::PathManager::logFileHeadlessName();
	}

	raco::log_system::init(logFilePath.internalPath().native());

	if (customLogFileNameFailed) {
		LOG_ERROR(raco::log_system::LOGGING, "Could not create log file at: " + customLogFileName + ". Using default location instead: " + logFilePath.string());
	}

	raco::log_system::setConsoleLogLevel(logLevel);
	raco::ramses_base::setRamsesLogLevel(logLevel);
	raco::ramses_base::setLogicLogLevel(logLevel);

	QString projectFile{};
	if (parser.isSet(loadProjectAction)) {
		QFileInfo path(parser.value(loadProjectAction));
		if (path.suffix().compare(raco::names::PROJECT_FILE_EXTENSION, Qt::CaseInsensitive) == 0) {
			if (path.exists()) {
				if (raco::utils::u8path(path.filePath().toStdString()).userHasReadAccess()) {
					projectFile = path.absoluteFilePath();
				} else {
					LOG_ERROR(raco::log_system::COMMON, "project file could not be read {}", path.filePath().toStdString());
				}
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
