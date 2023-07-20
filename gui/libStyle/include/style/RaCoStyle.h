/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <QProxyStyle>

QT_BEGIN_NAMESPACE
class QPainterPath;
QT_END_NAMESPACE

namespace raco::style {

class RaCoStyle : public QProxyStyle {
	Q_OBJECT

protected:
	// add a buffer to the right side of the drag indicator so that the right border of the indicator can be seen.
	static constexpr int DRAG_INDICATOR_RIGHT_PIXEL_PUFFER = 4;
	// corner radius of rounded elements
	static constexpr int CORNER_RADIUS = 8;
	// additional indent for text in rounded input fields
	static constexpr int CORNER_SPACING = 4;
	// transparency value for "disabled" icons
	static inline constexpr auto DISABLED_ICON_ALPHA = 0.6;

public:
	RaCoStyle();

	static void installFont();
	/**
	 * Gets a property from a widget or one of it's parents.
	 * @param widget the widget on which to look for the property.
	 * @param level specifies at which level in the widget hierarchy to look for the property (e.g. level 1 will look for the property at widget->parent()).
	 * @return [QVariant] of the property (the [QVariant] is invalid if the property doesn't exist).
	 */
	static QVariant saveGetProperty(const QWidget *widget, const char *name, size_t level = 0);

	QPalette standardPalette() const override;

	void polish(QWidget *widget) override;
	int pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const override;
	int styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *returnData) const override;

	QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc, const QWidget *widget) const override;
	QRect subElementRect(QStyle::SubElement sr, const QStyleOption *opt, const QWidget *widget) const override;
	QSize sizeFromContents(ContentsType ct, const QStyleOption *opt, const QSize &csz, const QWidget *widget) const override;
	QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option, const QWidget *widget) const override;
	QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap, const QStyleOption *opt) const override;

	void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const override;
	void drawControl(ControlElement ce, const QStyleOption *opt, QPainter *p, const QWidget *widget) const override;
	void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const override;

	void drawRoundedRect(const QRect &rect, QPainter *p, const QBrush &fill, const QBrush *clear = nullptr, const int radius = CORNER_RADIUS) const;
	bool eventFilter(QObject *obj, QEvent *event);

private:
	mutable QPalette defPalette_;
};
}  // namespace raco::style
