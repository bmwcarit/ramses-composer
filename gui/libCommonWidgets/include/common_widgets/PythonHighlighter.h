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

#include "style/Colors.h"

#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

namespace raco::common_widgets {

class PythonHighlighter : public QSyntaxHighlighter {
public:
	PythonHighlighter(QTextDocument *parent = nullptr, bool consoleMode = false);

protected:
	void highlightBlock(const QString &text) override;

private:
	struct HighlightingRule {
		QRegularExpression pattern;
		QTextCharFormat format;
	};

	bool isConsole_{false};

	QList<HighlightingRule> highlightingRules_;

	QRegularExpression commentStartExpression_;
	QRegularExpression commentEndExpression_;

	QTextCharFormat numberFormat_;
	QTextCharFormat keywordFormat_;
	QTextCharFormat classFormat;
	QTextCharFormat singleLineCommentFormat_;
	QTextCharFormat multiLineCommentFormat_;
	QTextCharFormat quotationFormat_;
	QTextCharFormat operatorsFormat_;
	QTextCharFormat indentationFormat_;
	QTextCharFormat arrowsFormat_;
};

}  // namespace raco::common_widgets
