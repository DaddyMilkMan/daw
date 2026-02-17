/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux
    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
    SPDX-License-Identifier: Apache-2.0 
*/
void CustomLFO::addPoint(const ShapePoint& point) {
    shape_.push_back(point);
    std::sort(shape_.begin(), shape_.end(),
        [](const ShapePoint& a, const ShapePoint& b) { return a.x < b.x; });
    tableDirty_ = true;
}
void CustomLFO::removePoint(int index) {
    if (index >= 0 && index < static_cast<int>(shape_.size())) {
        shape_.erase(shape_.begin() + index);
        tableDirty_ = true;
    }
}
void CustomLFO::clearShape() {
    shape_.clear();
    tableDirty_ = true;
}
void CustomLFO::setPresetShape(const juce::String& name) {
    shape_ = createPresetShape(name);
    tableDirty_ = true;
}
std::vector<CustomLFO::ShapePoint> CustomLFO::createPresetShape(const juce::String& name) {
    std::vector<ShapePoint> points;
    if (name == "Sine" || name.startsWith("Random")) {
        points = {
            {0.0f, 0.0f, 0.0f},
            {0.25f, 1.0f, 0.5f},
            {0.5f, 0.0f, 0.0f},
            {0.75f, -1.0f, -0.5f},
            {1.0f, 0.0f, 0.0f}
        };
    } else if (name == "Triangle") {
        points = {
            {0.0f, 0.0f, 0.0f},
            {0.25f, 1.0f, 0.0f},
            {0.5f, 0.0f, 0.0f},
            {0.75f, -1.0f, 0.0f},
            {1.0f, 0.0f, 0.0f}
        };
    } else if (name == "Saw") {
        points = {
            {0.0f, -1.0f, 0.0f},
            {1.0f, 1.0f, 0.0f}
        };
    } else if (name == "Square") {
        points = {
            {0.0f, 1.0f, 0.0f},
            {0.5f, 1.0f, 0.0f},
            {0.5f, -1.0f, 0.0f},
            {1.0f, -1.0f, 0.0f}
        };
    } else if (name == "SCurve") {
        points = {
            {0.0f, -1.0f, 0.5f},
            {0.5f, 0.0f, 0.0f},
            {1.0f, 1.0f, 0.5f}
        };
    } else if (name == "Exponential") {
        points = {
            {0.0f, 0.0f, 0.8f},
            {1.0f, 1.0f, 0.8f}
        };
    } else if (name == "Logarithmic") {
        points = {
            {0.0f, 0.0f, -0.8f},
            {1.0f, 1.0f, -0.8f}
        };
    } else if (name == "Bounce") {
        points = {
            {0.0f, 0.0f, 0.0f},
            {0.5f, 1.0f, 0.9f},
            {1.0f, 0.0f, 0.0f}
        };
    } else if (name == "Pulse") {
        points = {
            {0.0f, 1.0f, 0.0f},
            {0.1f, 1.0f, 0.0f},
            {0.1f, -1.0f, 0.0f},
            {1.0f, -1.0f, 0.0f}
        };
    } else if (name == "Wave") {
        points = {
            {0.0f, 0.0f, 0.0f},
            {0.125f, 0.5f, 0.3f},
            {0.25f, 1.0f, 0.0f},
            {0.375f, 0.5f, -0.3f},
            {0.5f, 0.0f, 0.0f},
            {0.625f, -0.5f, 0.3f},
            {0.75f, -1.0f, 0.0f},
            {0.875f, -0.5f, -0.3f},
            {1.0f, 0.0f, 0.0f}
        };
    } else if (name == "Ramp") {
        points = {
            {0.0f, -1.0f, 0.3f},
            {0.7f, 0.8f, 0.3f},
            {1.0f, -1.0f, 0.0f}
        };
    } else {
        // Default sine
        return createPresetShape("Sine");
    }
    return points;
}
void CustomLFO::setMorphShape(const std::vector<ShapePoint>& shape) {
    morphShape_ = shape;
    std::sort(morphShape_.begin(), morphShape_.end(),
        [](const ShapePoint& a, const ShapePoint& b) { return a.x < b.x; });
    tableDirty_ = true;
}
void CustomLFO::setMorphPosition(float position) {
    morphPosition_ = juce::jlimit(0.0f, 1.0f, position);
}
float CustomLFO::evaluateShape(float position, const std::vector<ShapePoint>& points) const {
    if (points.empty()) return 0.0f;
    if (points.size() == 1) return points[0].y;
    size_t idx = 0;
    for (size_t i = 0; i < points.size() - 1; ++i) {
        if (position >= points[i].x && position <= points[i + 1].x) {
            idx = i;
            break;
        }
    }
    if (position < points[0].x) return points[0].y;
    if (position > points.back().x) return points.back().y;
    float x0 = points[idx > 0 ? idx - 1 : idx].x;
    float y0 = points[idx > 0 ? idx - 1 : idx].y;
    float x1 = points[idx].x;
    float y1 = points[idx].y;
    float x2 = points[idx + 1].x;
    float y2 = points[idx + 1].y;
    float x3 = points[idx + 2 < points.size() ? idx + 2 : idx + 1].x;
    float y3 = points[idx + 2 < points.size() ? idx + 2 : idx + 1].y;
    float t = (position - x1) / (x2 - x1 + 1e-6f);
    float tension = points[idx].curve;
    return hermiteInterpolate(t, y0, y1, y2, y3, tension);
}
float CustomLFO::hermiteInterpolate(float x, float y0, float y1, float y2, float y3, float tension) const {
    float c = (y2 - y0) * 0.5f;
    float v = y1 - y2;
    float w = c + v;
    float a = (y3 - y1) - c - c;
    float b = w + w;
    return a * x * x * x + b * x * x + (-w - w - w) * x + y1;
}
