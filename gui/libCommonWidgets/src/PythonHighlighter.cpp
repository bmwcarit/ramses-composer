/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "common_widgets/PythonHighlighter.h"

namespace raco::common_widgets {

PythonHighlighter::PythonHighlighter(QTextDocument *parent, bool consoleMode) : QSyntaxHighlighter(parent), isConsole_(consoleMode) {
	HighlightingRule rule;

	if (!isConsole_) {
		keywordFormat_.setForeground(raco::style::Colors::color(raco::style::Colormap::externalReference));

		QStringList keywordPatterns;
		keywordPatterns << "\\bFalse\\b"
						<< "\\bNone\\b"
						<< "\\bTrue\\b"
						<< "\\band\\b"
						<< "\\bas\\b"
						<< "\\bassert\\b"
						<< "\\bbreak\\b"
						<< "\\bclass\\b"
						<< "\\bcontinue\\b"
						<< "\\bdef\\b"
						<< "\\bdel\\b"
						<< "\\belif\\b"
						<< "\\belse\\b"
						<< "\\bexcept\\b"
						<< "\\bfinally\\b"
						<< "\\bfor\\b"
						<< "\\bfrom\\b"
						<< "\\bglobal\\b"
						<< "\\bif\\b"
						<< "\\bimport\\b"
						<< "\\bin\\b"
						<< "\\bis\\b"
						<< "\\blambda\\b"
						<< "\\bnonlocal\\b"
						<< "\\bnot\\b"
						<< "\\bor\\b"
						<< "\\bpass\\b"
						<< "\\braise\\b"
						<< "\\breturn\\b"
						<< "\\btry\\b"
						<< "\\bwhile\\b"
						<< "\\bwith\\b"
						<< "\\byield\\b";

		for (const QString &pattern : keywordPatterns) {
			rule.pattern = QRegularExpression(pattern);
			rule.format = keywordFormat_;
			highlightingRules_.append(rule);
		}

		numberFormat_.setForeground(raco::style::Colors::color(raco::style::Colormap::textHighlightNumbers));
		rule.pattern = QRegularExpression(QStringLiteral("\\b[0-9]+\\b|\\b[0-9]+\\.[0-9]+\\b"));
		rule.format = numberFormat_;
		highlightingRules_.append(rule);

		singleLineCommentFormat_.setForeground(raco::style::Colors::color(raco::style::Colormap::textHighlightComments));
		rule.pattern = QRegularExpression(QStringLiteral("#[^\n]*"));
		rule.format = singleLineCommentFormat_;
		highlightingRules_.append(rule);

		quotationFormat_.setForeground(raco::style::Colors::color(raco::style::Colormap::textHighlightComments));
		rule.pattern = QRegularExpression(QStringLiteral("\".*\""));
		rule.format = quotationFormat_;
		highlightingRules_.append(rule);

		quotationFormat_.setForeground(raco::style::Colors::color(raco::style::Colormap::textHighlightComments));
		rule.pattern = QRegularExpression(QStringLiteral("'.*'"));
		rule.format = quotationFormat_;
		highlightingRules_.append(rule);

		operatorsFormat_.setForeground(raco::style::Colors::color(raco::style::Colormap::externalReference));
		rule.pattern = QRegularExpression(QStringLiteral("[(){}\\[\\]<>!=+\\-*/%^&|.,;:]+"));
		rule.format = operatorsFormat_;
		highlightingRules_.append(rule);

		indentationFormat_.setForeground(raco::style::Colors::color(raco::style::Colormap::textHighlightIndentation));
		rule.pattern = QRegularExpression(QString(QChar(0x2219)));
		rule.format = indentationFormat_;
		highlightingRules_.append(rule);

		multiLineCommentFormat_.setForeground(raco::style::Colors::color(raco::style::Colormap::textHighlightComments));
		commentStartExpression_ = QRegularExpression(QStringLiteral("\"\"\"\\b.*$"));
		commentEndExpression_ = QRegularExpression(QStringLiteral("^.*\\b\"\"\""));
	} else {
		arrowsFormat_.setForeground(raco::style::Colors::color(raco::style::Colormap::textHighlightArrows));
		rule.pattern = QRegularExpression(QStringLiteral(">>>|\\.  \\.  \\. "));
		rule.format = arrowsFormat_;
		highlightingRules_.append(rule);
	}
}

void PythonHighlighter::highlightBlock(const QString &text) {
	for (const HighlightingRule &rule : highlightingRules_) {
		QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
		while (matchIterator.hasNext()) {
			QRegularExpressionMatch match = matchIterator.next();
			setFormat(match.capturedStart(), match.capturedLength(), rule.format);
		}
	}

	if (!isConsole_) {
		setCurrentBlockState(0);

		int startIndex = 0;
		if (previousBlockState() != 1)
			startIndex = text.indexOf(commentStartExpression_);

		while (startIndex >= 0) {
			QRegularExpressionMatch match = commentEndExpression_.match(text, startIndex);
			int endIndex = match.capturedStart();
			int commentLength;
			if (endIndex == -1) {
				setCurrentBlockState(1);
				commentLength = text.length() - startIndex;
			} else {
				commentLength = endIndex - startIndex + match.capturedLength();
			}
			setFormat(startIndex, commentLength, multiLineCommentFormat_);
			startIndex = text.indexOf(commentStartExpression_, startIndex + commentLength);
		}
	}
}

}  // namespace raco::common_widgets