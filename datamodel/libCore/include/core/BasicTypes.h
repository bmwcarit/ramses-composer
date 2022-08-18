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

#include "BasicAnnotations.h"
#include "data_storage/ReflectionInterface.h"
#include "data_storage/Value.h"

#include <array>

namespace raco::core {

using raco::data_storage::StructBase;
using raco::data_storage::ValueBase;
using raco::data_storage::Property;

class Vec2f : public StructBase {
public:
	static inline const TypeDescriptor typeDescription = {"Vec2f", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}
	Vec2f(const Vec2f& other, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr)
		: StructBase(getProperties()), x(other.x), y(other.y) {}

	Vec2f(double defaultValue = 0.0, double step = 0.1, double min = 0.0, double max = 1.0)
		: StructBase(getProperties()),
		  x{defaultValue, DisplayNameAnnotation{"X"}, RangeAnnotation<double>(min, max)},
		  y{defaultValue, DisplayNameAnnotation{"Y"}, RangeAnnotation<double>(min, max)} {}

	Vec2f& operator=(const Vec2f& other) {
		x = other.x;
		y = other.y;
		return *this;
	}

	Vec2f& operator=(const std::array<double, 2>& value) {
		x = value[0];
		y = value[1];
		return *this;
	}

	friend bool operator==(const Vec2f& left, const std::array<double, 2>& right) {
		return left.x.asDouble() == right[0] &&
			   left.y.asDouble() == right[1];
	}

	friend bool operator!=(const Vec2f& left, const std::array<double, 2>& right) {
		return !(left == right);
	}

	void copyAnnotationData(const Vec2f& other) {
		x.copyAnnotationData(other.x);
		y.copyAnnotationData(other.y);
	}

	std::vector<std::pair<std::string, ValueBase*>> getProperties() {
		return {{"x", &x}, {"y", &y}};
	}

public:
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>> x;
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>> y;
};

class Vec3f : public StructBase {
public:
	static inline const TypeDescriptor typeDescription = {"Vec3f", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}
	Vec3f(const Vec3f& other, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr)
		: StructBase(getProperties()), x(other.x), y(other.y), z(other.z) {}

	Vec3f(double defaultValue = 0.0, double step = 0.1, double min = 0.0, double max = 1.0)
		: StructBase(getProperties()),
		  x{defaultValue, DisplayNameAnnotation{"X"}, RangeAnnotation<double>(min, max)},
		  y{defaultValue, DisplayNameAnnotation{"Y"}, RangeAnnotation<double>(min, max)},
		  z{defaultValue, DisplayNameAnnotation{"Z"}, RangeAnnotation<double>(min, max)} {}

	Vec3f& operator=(const Vec3f& other) {
		x = other.x;
		y = other.y;
		z = other.z;
		return *this;
	}

	Vec3f& operator=(const std::array<double, 3>& value) {
		x = value[0];
		y = value[1];
		z = value[2];
		return *this;
	}

	friend bool operator==(const Vec3f& left, const std::array<double, 3>& right) {
		return left.x.asDouble() == right[0] &&
			   left.y.asDouble() == right[1] &&
			   left.z.asDouble() == right[2];
	}

	friend bool operator!=(const Vec3f& left, const std::array<double, 3>& right) {
		return !(left == right);
	}

	void copyAnnotationData(const Vec3f& other) {
		x.copyAnnotationData(other.x);
		y.copyAnnotationData(other.y);
		z.copyAnnotationData(other.z);
	}

	std::vector<std::pair<std::string, ValueBase*>> getProperties() {
		return {{"x", &x}, {"y", &y}, {"z", &z}};
	}

public:
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>> x;
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>> y;
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>> z;
};

class Vec4f : public StructBase {
public:
	static inline const TypeDescriptor typeDescription = {"Vec4f", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}
	Vec4f(const Vec4f& other, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr)
		: StructBase(getProperties()), x(other.x), y(other.y), z(other.z), w(other.w) {}

	Vec4f(double defaultValue = 0.0, double step = 0.1, double min = 0.0, double max = 1.0)
		: StructBase(getProperties()),
		  x{defaultValue, DisplayNameAnnotation{"X"}, RangeAnnotation<double>(min, max)},
		  y{defaultValue, DisplayNameAnnotation{"Y"}, RangeAnnotation<double>(min, max)},
		  z{defaultValue, DisplayNameAnnotation{"Z"}, RangeAnnotation<double>(min, max)},
		  w{defaultValue, DisplayNameAnnotation{"W"}, RangeAnnotation<double>(min, max)} {}

	Vec4f& operator=(const Vec4f& other) {
		x = other.x;
		y = other.y;
		z = other.z;
		w = other.w;
		return *this;
	}

	Vec4f& operator=(const std::array<double, 4>& value) {
		x = value[0];
		y = value[1];
		z = value[2];
		w = value[3];
		return *this;
	}

	friend bool operator==(const Vec4f& left, const std::array<double, 4>& right) {
		return left.x.asDouble() == right[0] &&
			   left.y.asDouble() == right[1] &&
			   left.z.asDouble() == right[2] &&
			   left.w.asDouble() == right[3];
	}

	friend bool operator!=(const Vec4f& left, const std::array<double, 4>& right) {
		return !(left == right);
	}

	void copyAnnotationData(const Vec4f& other) {
		x.copyAnnotationData(other.x);
		y.copyAnnotationData(other.y);
		z.copyAnnotationData(other.z);
		w.copyAnnotationData(other.w);
	}

	std::vector<std::pair<std::string, ValueBase*>> getProperties() {
		return {{"x", &x}, {"y", &y}, {"z", &z}, {"w", &w}};
	}

	Property<double, DisplayNameAnnotation, RangeAnnotation<double>> x;
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>> y;
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>> z;
	Property<double, DisplayNameAnnotation, RangeAnnotation<double>> w;
};

class Vec2i : public StructBase {
public:
	static inline const TypeDescriptor typeDescription = {"Vec2i", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}
	Vec2i(const Vec2i& other, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr)
		: StructBase(getProperties()), i1_(other.i1_), i2_(other.i2_) {}

	Vec2i(int defaultValue = 0, int step = 1, int min = 0, int max = 1)
		: StructBase(getProperties()),
		  i1_{defaultValue, DisplayNameAnnotation{"i1"}, RangeAnnotation<int>(min, max)},
		  i2_{defaultValue, DisplayNameAnnotation{"i2"}, RangeAnnotation<int>(min, max)} {}

	Vec2i(std::array<int, 2> values, int min, int max)
		: StructBase(getProperties()),
		  i1_{values[0], DisplayNameAnnotation{"i1"}, RangeAnnotation<int>(min, max)},
		  i2_{values[1], DisplayNameAnnotation{"i2"}, RangeAnnotation<int>(min, max)} {}

	Vec2i& operator=(const Vec2i& other) {
		i1_ = other.i1_;
		i2_ = other.i2_;
		return *this;
	}

	Vec2i& operator=(const std::array<int, 2>& value) {
		i1_ = value[0];
		i2_ = value[1];
		return *this;
	}

	friend bool operator==(const Vec2i& left, const std::array<int, 2>& right) {
		return left.i1_.asInt() == right[0] &&
			   left.i2_.asInt() == right[1];
	}

	friend bool operator!=(const Vec2i& left, const std::array<int, 2>& right) {
		return !(left == right);
	}

	void copyAnnotationData(const Vec2i& other) {
		i1_.copyAnnotationData(other.i1_);
		i2_.copyAnnotationData(other.i2_);
	}

	std::vector<std::pair<std::string, ValueBase*>> getProperties() {
		return {{"i1", &i1_}, {"i2", &i2_}};
	}

	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> i1_;
	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> i2_;
};

class Vec3i : public StructBase {
public:
	static inline const TypeDescriptor typeDescription = {"Vec3i", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}
	Vec3i(const Vec3i& other, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr)
		: StructBase(getProperties()), i1_(other.i1_), i2_(other.i2_), i3_(other.i3_) {}

	Vec3i(int defaultValue = 0, int step = 1, int min = 0, int max = 1)
		: StructBase(getProperties()),
		  i1_{defaultValue, DisplayNameAnnotation{"i1"}, RangeAnnotation<int>(min, max)},
		  i2_{defaultValue, DisplayNameAnnotation{"i2"}, RangeAnnotation<int>(min, max)},
		  i3_{defaultValue, DisplayNameAnnotation{"i3"}, RangeAnnotation<int>(min, max)} {}

	Vec3i& operator=(const Vec3i& other) {
		i1_ = other.i1_;
		i2_ = other.i2_;
		i3_ = other.i3_;
		return *this;
	}

	Vec3i& operator=(const std::array<int, 3>& value) {
		i1_ = value[0];
		i2_ = value[1];
		i3_ = value[2];
		return *this;
	}

	friend bool operator==(const Vec3i& left, const std::array<int, 3>& right) {
		return left.i1_.asInt() == right[0] &&
			   left.i2_.asInt() == right[1] &&
			   left.i3_.asInt() == right[2];
	}

	friend bool operator!=(const Vec3i& left, const std::array<int, 3>& right) {
		return !(left == right);
	}

	void copyAnnotationData(const Vec3i& other) {
		i1_.copyAnnotationData(other.i1_);
		i2_.copyAnnotationData(other.i2_);
		i3_.copyAnnotationData(other.i3_);
	}

	std::vector<std::pair<std::string, ValueBase*>> getProperties() {
		return {{"i1", &i1_}, {"i2", &i2_}, {"i3", &i3_}};
	}
	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> i1_;
	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> i2_;
	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> i3_;
};

class Vec4i : public StructBase {
public:
	static inline const TypeDescriptor typeDescription = {"Vec4i", false};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}
	bool serializationRequired() const override {
		return true;
	}
	Vec4i(const Vec4i& other, std::function<SEditorObject(SEditorObject)>* translateRef = nullptr) : StructBase(getProperties()), i1_(other.i1_), i2_(other.i2_), i3_(other.i3_), i4_(other.i4_) {}

	Vec4i(int defaultValue = 0, int step = 1, int min = 0, int max = 1)
		: StructBase(getProperties()),
		  i1_{defaultValue, DisplayNameAnnotation{"i1"}, RangeAnnotation<int>(min, max)},
		  i2_{defaultValue, DisplayNameAnnotation{"i2"}, RangeAnnotation<int>(min, max)},
		  i3_{defaultValue, DisplayNameAnnotation{"i3"}, RangeAnnotation<int>(min, max)},
		  i4_{defaultValue, DisplayNameAnnotation{"i4"}, RangeAnnotation<int>(min, max)} {}

	Vec4i(std::array<int, 4> values, int min, int max)
		: StructBase(getProperties()),
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

	Vec4i& operator=(const std::array<int, 4>& value) {
		i1_ = value[0];
		i2_ = value[1];
		i3_ = value[2];
		i4_ = value[3];
		return *this;
	}

	friend bool operator==(const Vec4i& left, const std::array<int, 4>& right) {
		return left.i1_.asInt() == right[0] &&
			   left.i2_.asInt() == right[1] &&
			   left.i3_.asInt() == right[2] &&
			   left.i4_.asInt() == right[3];
	}

	friend bool operator!=(const Vec4i& left, const std::array<int, 4>& right) {
		return !(left == right);
	}

	void copyAnnotationData(const Vec4i& other) {
		i1_.copyAnnotationData(other.i1_);
		i2_.copyAnnotationData(other.i2_);
		i3_.copyAnnotationData(other.i3_);
		i4_.copyAnnotationData(other.i4_);
	}

	std::vector<std::pair<std::string, ValueBase*>> getProperties() {
		return {{"i1", &i1_}, {"i2", &i2_}, {"i3", &i3_}, {"i4", &i4_}};
	}

	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> i1_;
	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> i2_;
	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> i3_;
	Property<int, DisplayNameAnnotation, RangeAnnotation<int>> i4_;
};
}  // namespace raco::data_storage