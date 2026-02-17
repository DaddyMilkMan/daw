/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "SkiaComponent.h"
#include "SkiaLayout.h"
#include "ZenithDesignSystem.h"
#include <juce_core/juce_core.h>

// Forward declarations for UI components
namespace zenith {
namespace layout {
class ConfigValue {
public:
  enum class Type { Bool, Int, Float, String, Color, Array, Object };

  ConfigValue() = default;
  explicit ConfigValue(bool value);
  explicit ConfigValue(int value);
  explicit ConfigValue(float value);
  explicit ConfigValue(const juce::String &value);
  explicit ConfigValue(SkColor value);
  explicit ConfigValue(const juce::Array<ConfigValue> &array);
  explicit ConfigValue(const juce::DynamicObject::Ptr &object);

  // Type checking
  Type getType() const { return type_; }
  bool isBool() const { return type_ == Type::Bool; }
  bool isInt() const { return type_ == Type::Int; }
  bool isFloat() const { return type_ == Type::Float; }
  bool isString() const { return type_ == Type::String; }
  bool isColor() const { return type_ == Type::Color; }
  bool isArray() const { return type_ == Type::Array; }
  bool isObject() const { return type_ == Type::Object; }

  // Value access
  bool getBool() const;
  int getInt() const;
  float getFloat() const;
  juce::String getString() const;
  SkColor getColor() const;
  juce::Array<ConfigValue> getArray() const;
  juce::DynamicObject::Ptr getObject() const;

  // Conversion
  juce::var toVar() const;
  static ConfigValue fromVar(const juce::var &var);

private:
  Type type_ = Type::String;
  juce::var value_;
};

// ============================================================================
// Configuration Manager
// ============================================================================

} // namespace
