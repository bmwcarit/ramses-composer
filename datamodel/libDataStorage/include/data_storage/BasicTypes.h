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

#include "ReflectionInterface.h"
#include "Value.h"
#include "BasicAnnotations.h"

#include <array>

namespace raco::data_storage {

class Vec2f : public ClassWithReflectedMembers {
public:
	static inline const TypeDescriptor typeDescription = { "Vec2f", false };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}
	Vec2f(const Vec2f& other) : ClassWithReflectedMembers(getProperties()), x(other.x), y(other.y) {}

	Vec2f(double defaultValue = 0.0, double step = 0.1, double min = 0.0, double max = 1.0) :
		ClassWithReflectedMembers(getProperties()),
		x{ defaultValue, DisplayNameAnnotation{ "X" }, RangeAnnotation<double>(min, max) },
		y{ defaultValue, DisplayNameAnnotation{ "Y" }, RangeAnnotation<double>(min, max) }
	{}

	Vec2f& operator=(const Vec2f& other) {
		x = other.x;
		y = other.y;
		return *this;
	}
	
	void copyAnnotationData(const Vec2f& other) {
		x.copyAnnotationData(other.x);
		y.copyAnnotationData(other.y);
	}

	std::vector<std::pair<std::string, ValueBase*>> getProperties() {
		return { {"x", &x}, {"y", &y} };
	}

public:
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>> x;
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>> y;
};


class Vec3f : public ClassWithReflectedMembers {
public:
	static inline const TypeDescriptor typeDescription = { "Vec3f", false };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}
	Vec3f(const Vec3f& other) : ClassWithReflectedMembers(getProperties()), x(other.x), y(other.y), z(other.z) {}

	Vec3f(double defaultValue = 0.0, double step = 0.1, double min = 0.0, double max = 1.0) :
		ClassWithReflectedMembers(getProperties()),
		x{ defaultValue, DisplayNameAnnotation{ "X" }, RangeAnnotation<double>(min, max) },
		y{ defaultValue, DisplayNameAnnotation{ "Y" }, RangeAnnotation<double>(min, max) },
		z{ defaultValue, DisplayNameAnnotation{ "Z" }, RangeAnnotation<double>(min, max) }
	{}

	Vec3f& operator=(const Vec3f& other) {
		x = other.x;
		y = other.y;
		z = other.z;
		return *this;
	}
	
	void copyAnnotationData(const Vec3f& other) {
		x.copyAnnotationData(other.x);
		y.copyAnnotationData(other.y);
		z.copyAnnotationData(other.z);
	}

	std::vector<std::pair<std::string, ValueBase*>> getProperties() {
		return { {"x", &x}, {"y", &y}, {"z", &z} };
	}

public:
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>> x;
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>> y;
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>> z;
};


class Vec4f : public ClassWithReflectedMembers {
public:
	static inline const TypeDescriptor typeDescription = { "Vec4f", false };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}
	Vec4f(const Vec4f& other) : ClassWithReflectedMembers(getProperties()), x(other.x), y(other.y), z(other.z), w(other.w) {}

	Vec4f(double defaultValue = 0.0, double step = 0.1, double min = 0.0, double max = 1.0) :
		ClassWithReflectedMembers(getProperties()),
		x{ defaultValue, DisplayNameAnnotation{ "X" }, RangeAnnotation<double>(min, max) },
		y{ defaultValue, DisplayNameAnnotation{ "Y" }, RangeAnnotation<double>(min, max) },
		z{ defaultValue, DisplayNameAnnotation{ "Z" }, RangeAnnotation<double>(min, max) },
		w{ defaultValue, DisplayNameAnnotation{ "W" }, RangeAnnotation<double>(min, max) }
	{}

	Vec4f& operator=(const Vec4f& other) {
		x = other.x;
		y = other.y;
		z = other.z;
		w = other.w;
		return *this;
	}

	void copyAnnotationData(const Vec4f& other) {
		x.copyAnnotationData(other.x);
		y.copyAnnotationData(other.y);
		z.copyAnnotationData(other.z);
		w.copyAnnotationData(other.w);
	}
	
	std::vector<std::pair<std::string, ValueBase*>> getProperties() {
		return { {"x", &x}, {"y", &y}, {"z", &z}, {"w", &w } };
	}

	Property<double, DisplayNameAnnotation, RangeAnnotation<double>> x;
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>> y;
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>> z;
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>> w;
};


class Vec2i : public ClassWithReflectedMembers {
public:
	static inline const TypeDescriptor typeDescription = { "Vec2i", false };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}
	Vec2i(const Vec2i& other) : ClassWithReflectedMembers(getProperties()), i1_(other.i1_), i2_(other.i2_) {}

	Vec2i(int defaultValue = 0, int step = 1, int min = 0, int max = 1) :
		ClassWithReflectedMembers(getProperties()),
		i1_{ defaultValue, DisplayNameAnnotation{ "i1" }, RangeAnnotation<int>(min, max) },
		i2_{ defaultValue, DisplayNameAnnotation{ "i2" }, RangeAnnotation<int>(min, max) }
	{}

    Vec2i(std::array<int, 2> values, int min, int max) : ClassWithReflectedMembers(getProperties()),
														i1_{values[0], DisplayNameAnnotation{"i1"}, RangeAnnotation<int>(min, max)},
														i2_{values[1], DisplayNameAnnotation{"i2"}, RangeAnnotation<int>(min, max)} {}

	Vec2i& operator=(const Vec2i& other) {
		i1_ = other.i1_;
		i2_ = other.i2_;
		return *this;
	}

	void copyAnnotationData(const Vec2i& other) {
		i1_.copyAnnotationData(other.i1_);
		i2_.copyAnnotationData(other.i2_);
	}

	std::vector<std::pair<std::string, ValueBase*>> getProperties() {
		return { {"i1", &i1_}, {"i2", &i2_} };
	}

	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> i1_;
	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> i2_;
};

class Vec3i : public ClassWithReflectedMembers {
public:
	static inline const TypeDescriptor typeDescription = { "Vec3i", false };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}
	Vec3i(const Vec3i& other) : ClassWithReflectedMembers(getProperties()), i1_(other.i1_), i2_(other.i2_), i3_(other.i3_) {}

	Vec3i(int defaultValue = 0, int step = 1, int min = 0, int max = 1) :
		ClassWithReflectedMembers(getProperties()),
		i1_{ defaultValue, DisplayNameAnnotation{ "i1" }, RangeAnnotation<int>(min, max) },
		i2_{ defaultValue, DisplayNameAnnotation{ "i2" }, RangeAnnotation<int>(min, max) },
		i3_{ defaultValue, DisplayNameAnnotation{ "i3" }, RangeAnnotation<int>(min, max) }
	{}

	Vec3i& operator=(const Vec3i& other) {
		i1_ = other.i1_;
		i2_ = other.i2_;
		i3_ = other.i3_;
		return *this;
	}

	void copyAnnotationData(const Vec3i& other) {
		i1_.copyAnnotationData(other.i1_);
		i2_.copyAnnotationData(other.i2_);
		i3_.copyAnnotationData(other.i3_);
	}

	std::vector<std::pair<std::string, ValueBase*>> getProperties() {
		return { {"i1", &i1_}, {"i2", &i2_}, {"i3", &i3_} };
	}
	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> i1_;
	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> i2_;
	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> i3_;
};

class Vec4i : public ClassWithReflectedMembers {
public:
	static inline const TypeDescriptor typeDescription = { "Vec4i", false };
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}
	Vec4i(const Vec4i& other) : ClassWithReflectedMembers(getProperties()), i1_(other.i1_), i2_(other.i2_), i3_(other.i3_), i4_(other.i4_) {}

	Vec4i(int defaultValue = 0, int step = 1, int min = 0, int max = 1) :
		ClassWithReflectedMembers(getProperties()),
		i1_{ defaultValue, DisplayNameAnnotation{ "i1" }, RangeAnnotation<int>(min, max) },
		i2_{ defaultValue, DisplayNameAnnotation{ "i2" }, RangeAnnotation<int>(min, max) },
		i3_{ defaultValue, DisplayNameAnnotation{ "i3" }, RangeAnnotation<int>(min, max) },
		i4_{ defaultValue, DisplayNameAnnotation{ "i4" }, RangeAnnotation<int>(min, max) }
	{}

	Vec4i(std::array<int, 4> values, int min, int max) : ClassWithReflectedMembers(getProperties()),
																	i1_{values[0], DisplayNameAnnotation{"i1"}, RangeAnnotation<int>(min, max)},
																	i2_{values[1], DisplayNameAnnotation{"i2"}, RangeAnnotation<int>(min, max)},
																	i3_{values[2], DisplayNameAnnotation{"i3"}, RangeAnnotation<int>(min, max)},
																	i4_{values[3], DisplayNameAnnotation{"i4"}, RangeAnnotation<int>(min, max)} {}

	Vec4i& operator=(const Vec4i& other) {
		i1_ = other.i1_;
		i2_ = other.i2_;
		i3_ = other.i3_;
		i4_ = other.i4_;
		return *this;
	}
	
	void copyAnnotationData(const Vec4i& other) {
		i1_.copyAnnotationData(other.i1_);
		i2_.copyAnnotationData(other.i2_);
		i3_.copyAnnotationData(other.i3_);
		i4_.copyAnnotationData(other.i4_);
	}

	std::vector<std::pair<std::string, ValueBase*>> getProperties() {
		return { {"i1", &i1_}, {"i2", &i2_}, {"i3", &i3_}, {"i4", &i4_} };
	}

	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> i1_;
	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> i2_;
	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> i3_;
	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> i4_;
};

}