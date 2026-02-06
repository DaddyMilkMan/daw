/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

// PerformanceMonitor.cpp

#include "PerformanceMonitor.h"
#include "../../design-system/ZenithTheme.h"
#include <algorithm>
#include <numeric>

namespace zenith::ui {

//==============================================================================
// Construction
//==============================================================================

PerformanceMonitor::PerformanceMonitor() {
    monitoringEnabled_ = true;
    needsAdjustment_ = false;

    // Set default thresholds
    goodFrameTimeThreshold_ = 16.67f;    // 60fps
    acceptableFrameTimeThreshold_ = 33.33f;  // 30fps
    poorFrameTimeThreshold_ = 50.0f;      // 20fps

    // Set default quality settings
    qualitySettings_ = QualitySettings::createUltraQuality();

    // Initialize metrics
    currentMetrics_.frameRate = 60.0f;
    currentMetrics_.frameTimeMs = 16.67f;
    currentMetrics_.frameTimeMinMs = 16.67f;
    currentMetrics_.frameTimeMaxMs = 16.67f;
    currentMetrics_.frameTimeAverageMs = 16.67f;
    currentMetrics_.frameTimeJitterMs = 0.0f;

    // Initialize with history
    lastFrameTime_ = std::chrono::steady_clock::now();
    lastAdjustmentTime_ = lastFrameTime_;

    // Add initial history entry
    addToHistory(currentMetrics_);
}

//==============================================================================
// Configuration
//==============================================================================

void PerformanceMonitor::setMonitoringEnabled(bool enabled) {
    monitoringEnabled_ = enabled;
    if (!enabled) {
        reset();
    }
}

void PerformanceMonitor::setQualitySettings(const QualitySettings& settings) {
    QualitySettings oldLevel = qualitySettings_;
    qualitySettings_ = settings;
    notifyQualityChange(oldLevel.level, settings.level);
}

void PerformanceMonitor::setPerformanceThresholds(float good, float acceptable, float poor) {
    goodFrameTimeThreshold_ = good;
    acceptableFrameTimeThreshold_ = acceptable;
    poorFrameTimeThreshold_ = poor;
}

void PerformanceMonitor::setMemoryLimit(size_t bytes) {
    currentMetrics_.memoryLimitBytes = bytes;
}

//==============================================================================
// Frame Tracking
//==============================================================================

void PerformanceMonitor::beginFrame() {
    if (!monitoringEnabled_) {
        return;
    }

    frameStartTime_ = std::chrono::steady_clock::now();
}

void PerformanceMonitor::endFrame() {
    if (!monitoringEnabled_) {
        return;
    }

    auto frameEndTime = std::chrono::steady_clock::now();
    auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(frameEndTime - frameStartTime_);
    float frameTimeMs = frameDuration.count() / 1000.0f;

    // Update frame timing metrics
    currentMetrics_.frameTimeMs = frameTimeMs;
    currentMetrics_.frameRate = 1000.0f / frameTimeMs;

    // Add to frame times history
    frameTimes_.push_back(frameTimeMs);
    if (frameTimes_.size() > 120) {  // Keep last 2 seconds of data
        frameTimes_.erase(frameTimes_.begin());
    }

    // Update min/max/average
    currentMetrics_.frameTimeMinMs = *std::min_element(frameTimes_.begin(), frameTimes_.end());
    currentMetrics_.frameTimeMaxMs = *std::max_element(frameTimes_.begin(), frameTimes_.end());
    currentMetrics_.frameTimeAverageMs = std::accumulate(frameTimes_.begin(), frameTimes_.end(), 0.0f) / frameTimes_.size();
    currentMetrics_.frameTimeJitterMs = calculateFrameJitter();

    // Check for performance warnings
    checkPerformanceWarnings();

    // Add to history
    addToHistory(currentMetrics_);

    // Clean old history
    cleanOldHistory();

    // Check if quality adjustment is needed
    if (needsAdjustment_ && std::chrono::steady_clock::now() - lastAdjustmentTime_ > std::chrono::seconds(2)) {
        applyQualityAdjustment();
        lastAdjustmentTime_ = std::chrono::steady_clock::now();
    }
}

void PerformanceMonitor::trackGPUPerformance(float gpuTimeMs, float memoryMB, float textureCount, float drawCalls) {
    currentMetrics_.gpuFrameTimeMs = gpuTimeMs;
    currentMetrics_.gpuMemoryUsageMB = memoryMB;
    currentMetrics_.gpuTextureCount = textureCount;
    currentMetrics_.gpuDrawCalls = drawCalls;
}

void PerformanceMonitor::trackMemoryUsage(size_t bytes) {
    currentMetrics_.memoryUsageBytes = bytes;
    if (bytes > currentMetrics_.peakMemoryUsageBytes) {
        currentMetrics_.peakMemoryUsageBytes = bytes;
    }
}

void PerformanceMonitor::trackDirtyRects(int count) {
    currentMetrics_.dirtyRectCount = count;
}

void PerformanceMonitor::trackAnimatedClips(int count) {
    currentMetrics_.animatedClips = count;
}

//==============================================================================
// Quality Adjustment
//==============================================================================

QualityLevel PerformanceMonitor::getCurrentQualityLevel() const {
    return qualitySettings_.level;
}

QualityLevel PerformanceMonitor::getAdjustedQualityLevel() const {
    if (qualitySettings_.level != QualityLevel::Auto) {
        return qualitySettings_.level;
    }

    return calculateOptimalQuality();
}

void PerformanceMonitor::applyQualityAdjustment() {
    if (!monitoringEnabled_ || qualitySettings_.level != QualityLevel::Auto) {
        return;
    }

    QualityLevel optimal = calculateOptimalQuality();
    QualityLevel current = getCurrentQualityLevel();

    if (optimal != current) {
        // Don't downgrade immediately - give time for recovery
        if (optimal < current && currentMetrics_.frameTimeMs > poorFrameTimeThreshold_) {
            setQualityLevel(optimal);
        }
        // Upgrade if performance is excellent
        else if (optimal > current && currentMetrics_.frameTimeMs < goodFrameTimeThreshold_) {
            setQualityLevel(optimal);
        }
    }

    needsAdjustment_ = false;
}

void PerformanceMonitor::setQualityLevel(QualityLevel level) {
    QualitySettings oldSettings = qualitySettings_;

    switch (level) {
        case QualityLevel::Ultra:
            qualitySettings_ = QualitySettings::createUltraQuality();
            break;
        case QualityLevel::High:
            qualitySettings_ = QualitySettings::createHighQuality();
            break;
        case QualityLevel::Medium:
            qualitySettings_ = QualitySettings::createMediumQuality();
            break;
        case QualityLevel::Low:
            qualitySettings_ = QualitySettings::createLowQuality();
            break;
        case QualityLevel::Auto:
            qualitySettings_ = QualitySettings::createUltraQuality();
            qualitySettings_.level = QualityLevel::Auto;
            break;
    }

    notifyQualityChange(oldSettings.level, qualitySettings_.level);
}

bool PerformanceMonitor::needsQualityAdjustment() const {
    if (qualitySettings_.level != QualityLevel::Auto) {
        return false;
    }

    return currentMetrics_.frameTimeMs > acceptableFrameTimeThreshold_ ||
           currentMetrics_.memoryUsageBytes > currentMetrics_.memoryLimitBytes * 0.9f;
}

//==============================================================================
// Metrics Access
//==============================================================================

PerformanceMetrics PerformanceMonitor::getCurrentMetrics() const {
    return currentMetrics_;
}

std::vector<PerformanceHistoryEntry> PerformanceMonitor::getHistory(int seconds) const {
    std::vector<PerformanceHistoryEntry> result;
    auto cutoff = std::chrono::steady_clock::now() - std::chrono::seconds(seconds);

    for (const auto& entry : history_) {
        if (entry.timestamp >= cutoff) {
            result.push_back(entry);
        }
    }

    return result;
}

juce::String PerformanceMonitor::getPerformanceReport() const {
    juce::StringArray report;

    report.add("=== Performance Report ===");
    report.add("Frame Rate: " + juce::String(currentMetrics_.frameRate, 1) + " fps");
    report.add("Frame Time: " + juce::String(currentMetrics_.frameTimeMs, 2) + " ms");
    report.add("Performance Status: " + currentMetrics_.getPerformanceStatus());
    report.add("Performance Score: " + juce::String(currentMetrics_.getPerformanceScore(), 1) + "/100");
    report.add("Memory Usage: " + juce::String(currentMetrics_.memoryUsageBytes / (1024 * 1024), 1) + " MB");
    report.add("GPU Memory: " + juce::String(currentMetrics_.gpuMemoryUsageMB, 1) + " MB");
    report.add("GPU Draw Calls: " + juce::String(currentMetrics_.gpuDrawCalls, 0));
    report.add("Dirty Rects: " + juce::String(currentMetrics_.dirtyRectCount, 0));
    report.add("Animated Clips: " + juce::String(currentMetrics_.animatedClips, 0));
    report.add("Quality Level: " + qualityLevelToString(getAdjustedQualityLevel()));

    return report.joinIntoString("\n");
}

juce::String PerformanceMonitor::getPerformanceIndicator() const {
    float score = currentMetrics_.getPerformanceScore();
    juce::String status = currentMetrics_.getPerformanceStatus();

    if (score >= 80.0f) {
        return "● " + status + " (" + juce::String(score, 0) + "%)";
    } else if (score >= 60.0f) {
        return "◐ " + status + " (" + juce::String(score, 0) + "%)";
    } else if (score >= 40.0f) {
        return "○ " + status + " (" + juce::String(score, 0) + "%)";
    } else {
        return "◯ " + status + " (" + juce::String(score, 0) + "%)";
    }
}

//==============================================================================
// Visual Feedback
//==============================================================================

void PerformanceMonitor::drawPerformanceOverlay(SkCanvas* canvas, float x, float y, float width, float height) {
    if (!monitoringEnabled_) {
        return;
    }

    // Background
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetARGB(180, 0, 0, 0));
    bgPaint.setAntiAlias(true);
    canvas->drawRoundRect(SkRect::MakeXYWH(x, y, width, height), 8.0f, 8.0f, bgPaint);

    // Text
    SkPaint textPaint;
    textPaint.setColor(SkColorSetARGB(255, 255, 255, 255));
    textPaint.setTextSize(14.0f);
    textPaint.setAntiAlias(true);

    // Performance indicator
    SkColor statusColor = currentMetrics_.getPerformanceColor();
    SkPaint statusPaint;
    statusPaint.setColor(statusColor);
    statusPaint.setAntiAlias(true);

    canvas->drawRoundRect(SkRect::MakeXYWH(x + 4, y + 4, 16, 16), 4.0f, 4.0f, statusPaint);

    canvas->drawSimpleText("Performance: ", x + 24, y + 14, textPaint);
    canvas->drawSimpleText(performanceIndicator().toRawUTF8(), x + 120, y + 14, textPaint);

    // Frame rate
    canvas->drawSimpleText(juce::String("FPS: " + juce::String(currentMetrics_.frameRate, 1)).toRawUTF8(),
                          x + 4, y + 34, textPaint);

    // Memory usage
    canvas->drawSimpleText(juce::String("RAM: " + juce::String(currentMetrics_.memoryUsageBytes / (1024 * 1024), 0) + "MB").toRawUTF8(),
                          x + 4, y + 54, textPaint);

    // Quality level
    canvas->drawSimpleText(juce::String("Quality: " + qualityLevelToString(getAdjustedQualityLevel())).toRawUTF8(),
                          x + 4, y + 74, textPaint);
}

void PerformanceMonitor::drawPerformanceGraph(SkCanvas* canvas, float x, float y, float width, float height, int seconds) {
    if (!monitoringEnabled_ || history_.empty()) {
        return;
    }

    // Background
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetARGB(180, 0, 0, 0));
    bgPaint.setAntiAlias(true);
    canvas->drawRoundRect(SkRect::MakeXYWH(x, y, width, height), 8.0f, 8.0f, bgPaint);

    // Get recent history
    auto history = getHistory(seconds);
    if (history.size() < 2) {
        return;
    }

    // Calculate graph bounds
    float graphX = x + 10;
    float graphY = y + 30;
    float graphWidth = width - 20;
    float graphHeight = height - 40;

    // Draw grid lines
    SkPaint gridPaint;
    gridPaint.setColor(SkColorSetARGB(100, 255, 255, 255));
    gridPaint.setStrokeWidth(1.0f);

    // Horizontal grid lines
    for (int i = 0; i <= 4; i++) {
        float gridY = graphY + (graphHeight / 4) * i;
        canvas->drawLine(graphX, gridY, graphX + graphWidth, gridY, gridPaint);
    }

    // Vertical grid lines
    for (int i = 0; i <= 5; i++) {
        float gridX = graphX + (graphWidth / 5) * i;
        canvas->drawLine(gridX, graphY, gridX, graphY + graphHeight, gridPaint);
    }

    // Draw frame time graph
    SkPaint framePaint;
    framePaint.setColor(SkColorSetARGB(255, 59, 130, 246));
    framePaint.setStrokeWidth(2.0f);
    framePaint.setStyle(SkPaint::kStroke_Style);

    SkPath framePath;
    float maxFrameTime = poorFrameTimeThreshold_ * 1.2f;

    for (size_t i = 0; i < history.size(); i++) {
        float px = graphX + (graphWidth / (history.size() - 1)) * i;
        float py = graphY + graphHeight - (history[i].metrics.frameTimeMs / maxFrameTime) * graphHeight;

        if (i == 0) {
            framePath.moveTo(px, py);
        } else {
            framePath.lineTo(px, py);
        }
    }

    canvas->drawPath(framePath, framePaint);

    // Draw target line
    SkPaint targetPaint;
    targetPaint.setColor(SkColorSetARGB(180, 52, 211, 153));
    targetPaint.setStrokeWidth(1.0f);
    targetPaint.setStyle(SkPaint::kStroke_Style);

    float targetY = graphY + graphHeight - (goodFrameTimeThreshold_ / maxFrameTime) * graphHeight;
    canvas->drawLine(graphX, targetY, graphX + graphWidth, targetY, targetPaint);

    // Labels
    SkPaint textPaint;
    textPaint.setColor(SkColorSetARGB(255, 255, 255, 255));
    textPaint.setTextSize(12.0f);
    textPaint.setAntiAlias(true);

    canvas->drawSimpleText("Frame Time", graphX, y + 15, textPaint);

    // Time labels
    textPaint.setTextSize(10.0f);
    for (int i = 0; i <= 5; i++) {
        float labelX = graphX + (graphWidth / 5) * i;
        juce::String timeLabel = juce::String(seconds - (seconds * i / 5)) + "s";
        canvas->drawSimpleText(timeLabel.toRawUTF8(), labelX, y + height - 5, textPaint);
    }
}

void PerformanceMonitor::drawQualityIndicator(SkCanvas* canvas, float x, float y, float size) {
    if (!monitoringEnabled_) {
        return;
    }

    // Background circle
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetARGB(180, 0, 0, 0));
    bgPaint.setAntiAlias(true);
    canvas->drawCircle(x + size / 2, y + size / 2, size / 2, bgPaint);

    // Quality level indicator
    QualityLevel level = getAdjustedQualityLevel();
    SkColor qualityColor = SkColorSetARGB(255, 255, 255, 255);

    switch (level) {
        case QualityLevel::Ultra:
            qualityColor = SkColorSetARGB(255, 139, 92, 246); // Purple
            break;
        case QualityLevel::High:
            qualityColor = SkColorSetARGB(255, 59, 130, 246); // Blue
            break;
        case QualityLevel::Medium:
            qualityColor = SkColorSetARGB(255, 34, 197, 94); // Green
            break;
        case QualityLevel::Low:
            qualityColor = SkColorSetARGB(255, 239, 68, 68); // Red
            break;
        case QualityLevel::Auto:
            qualityColor = SkColorSetARGB(255, 251, 191, 36); // Yellow
            break;
    }

    SkPaint qualityPaint;
    qualityPaint.setColor(qualityColor);
    qualityPaint.setAntiAlias(true);

    // Draw quality bars
    int barCount = 4;
    float barWidth = (size - 20) / barCount;
    float barHeight = size * 0.6f;

    for (int i = 0; i < barCount; i++) {
        float barX = x + 10 + i * barWidth;
        float barY = y + size - 10 - barHeight;

        if (i < static_cast<int>(level) || (level == QualityLevel::Auto && i < 3)) {
            canvas->drawRoundRect(SkRect::MakeXYWH(barX, barY, barWidth - 2, barHeight), 2.0f, 2.0f, qualityPaint);
        } else {
            canvas->drawRoundRect(SkRect::MakeXYWH(barX, barY, barWidth - 2, barHeight), 2.0f, 2.0f, bgPaint);
        }
    }

    // Draw "AUTO" text if in auto mode
    if (level == QualityLevel::Auto) {
        SkPaint textPaint;
        textPaint.setColor(qualityColor);
        textPaint.setTextSize(8.0f);
        textPaint.setAntiAlias(true);
        canvas->drawSimpleText("AUTO", x + size / 2 - 15, y + size / 2 + 3, textPaint);
    }
}

//==============================================================================
// Event Handlers
//==============================================================================

void PerformanceMonitor::checkPerformanceWarnings() {
    if (currentMetrics_.frameTimeMs > poorFrameTimeThreshold_) {
        needsAdjustment_ = true;
        notifyCriticalIssue(currentMetrics_);
    } else if (currentMetrics_.frameTimeMs > acceptableFrameTimeThreshold_) {
        needsAdjustment_ = true;
        notifyPerformanceWarning(currentMetrics_);
    }
}

//==============================================================================
// Utilities
//==============================================================================

void PerformanceMonitor::reset() {
    currentMetrics_ = PerformanceMetrics();
    history_.clear();
    frameTimes_.clear();
    lastFrameTime_ = std::chrono::steady_clock::now();
    lastAdjustmentTime_ = lastFrameTime_;
}

juce::String PerformanceMonitor::exportPerformanceData() const {
    juce::String data;
    data += "timestamp,frame_rate,frame_time,quality_level\n";

    for (const auto& entry : history_) {
        auto timestamp = std::chrono::system_clock::to_time_t(entry.timestamp);
        data += juce::String(timestamp) + "," +
                juce::String(entry.metrics.frameRate, 2) + "," +
                juce::String(entry.metrics.frameTimeMs, 2) + "," +
                juce::String(static_cast<int>(entry.metrics.qualityLevel)) + "\n";
    }

    return data;
}

void PerformanceMonitor::importPerformanceData(const juce::String& data) {
    // This would parse CSV data and update history
    // Implementation would depend on the exact format
    // For now, this is a placeholder
}

bool PerformanceMonitor::isPerformanceAcceptable() const {
    return currentMetrics_.frameTimeMs <= acceptableFrameTimeThreshold_ &&
           currentMetrics_.memoryUsageBytes <= currentMetrics_.memoryLimitBytes * 0.9f;
}

//==============================================================================
// Private Helpers
//==============================================================================

void PerformanceMonitor::updateMetrics() {
    // Calculate rolling averages
    if (frameTimes_.size() > 60) {
        std::vector<float> recent(frameTimes_.end() - 60, frameTimes_.end());
        currentMetrics_.frameTimeAverageMs = std::accumulate(recent.begin(), recent.end(), 0.0f) / recent.size();
    }

    // Update quality level if in auto mode
    if (qualitySettings_.level == QualityLevel::Auto) {
        QualityLevel optimal = calculateOptimalQuality();
        if (optimal != getCurrentQualityLevel()) {
            setQualityLevel(optimal);
        }
    }
}

void PerformanceMonitor::addToHistory(const PerformanceMetrics& metrics) {
    PerformanceHistoryEntry entry;
    entry.metrics = metrics;
    history_.push_back(entry);

    // Keep history size reasonable
    if (history_.size() > 6000) {  // ~5 minutes at 20fps
        history_.erase(history_.begin());
    }
}

void PerformanceMonitor::cleanOldHistory() {
    auto cutoff = std::chrono::steady_clock::now() - std::chrono::minutes(5);
    while (!history_.empty() && history_.front().timestamp < cutoff) {
        history_.pop_front();
    }
}

QualityLevel PerformanceMonitor::calculateOptimalQuality() const {
    if (currentMetrics_.frameTimeMs <= goodFrameTimeThreshold_ &&
        currentMetrics_.memoryUsageBytes <= currentMetrics_.memoryLimitBytes * 0.5f) {
        return QualityLevel::Ultra;
    } else if (currentMetrics_.frameTimeMs <= goodFrameTimeThreshold_ &&
               currentMetrics_.memoryUsageBytes <= currentMetrics_.memoryLimitBytes * 0.7f) {
        return QualityLevel::High;
    } else if (currentMetrics_.frameTimeMs <= acceptableFrameTimeThreshold_ &&
               currentMetrics_.memoryUsageBytes <= currentMetrics_.memoryLimitBytes * 0.8f) {
        return QualityLevel::Medium;
    } else {
        return QualityLevel::Low;
    }
}

QualityLevel PerformanceMonitor::downgradeQuality(QualityLevel current) const {
    switch (current) {
        case QualityLevel::Ultra:
            return QualityLevel::High;
        case QualityLevel::High:
            return QualityLevel::Medium;
        case QualityLevel::Medium:
            return QualityLevel::Low;
        default:
            return QualityLevel::Low;
    }
}

QualityLevel PerformanceMonitor::upgradeQuality(QualityLevel current) const {
    switch (current) {
        case QualityLevel::Low:
            return QualityLevel::Medium;
        case QualityLevel::Medium:
            return QualityLevel::High;
        case QualityLevel::High:
            return QualityLevel::Ultra;
        default:
            return QualityLevel::Ultra;
    }
}

float PerformanceMonitor::calculateAverageFrameTime() const {
    if (frameTimes_.empty()) {
        return 16.67f;
    }

    return std::accumulate(frameTimes_.begin(), frameTimes_.end(), 0.0f) / frameTimes_.size();
}

float PerformanceMonitor::calculateFrameJitter() const {
    if (frameTimes_.size() < 2) {
        return 0.0f;
    }

    float mean = calculateAverageFrameTime();
    float variance = 0.0f;

    for (float time : frameTimes_) {
        variance += (time - mean) * (time - mean);
    }

    return std::sqrt(variance / frameTimes_.size());
}

size_t PerformanceMonitor::getMemoryUsage() const {
    // This would query actual memory usage from the system
    // For now, return cached value
    return currentMetrics_.memoryUsageBytes;
}

void PerformanceMonitor::notifyQualityChange(QualityLevel oldLevel, QualityLevel newLevel) {
    if (onQualityChange) {
        onQualityChange(oldLevel, newLevel);
    }
}

void PerformanceMonitor::notifyPerformanceWarning(const PerformanceMetrics& metrics) {
    if (onPerformanceWarning) {
        onPerformanceWarning(metrics);
    }
}

void PerformanceMonitor::notifyCriticalIssue(const PerformanceMetrics& metrics) {
    if (onCriticalPerformanceIssue) {
        onCriticalPerformanceIssue(metrics);
    }
}

juce::String PerformanceMonitor::qualityLevelToString(QualityLevel level) {
    switch (level) {
        case QualityLevel::Ultra:
            return "Ultra";
        case QualityLevel::High:
            return "High";
        case QualityLevel::Medium:
            return "Medium";
        case QualityLevel::Low:
            return "Low";
        case QualityLevel::Auto:
            return "Auto";
        default:
            return "Unknown";
    }
}

juce::String PerformanceMonitor::getQualityDescription(QualityLevel level) {
    switch (level) {
        case QualityLevel::Ultra:
            return "Maximum quality with all effects enabled";
        case QualityLevel::High:
            return "High quality with most effects";
        case QualityLevel::Medium:
            return "Balanced quality and performance";
        case QualityLevel::Low:
            return "Minimum quality for smooth performance";
        case QualityLevel::Auto:
            return "Automatically adjusts quality based on performance";
        default:
            return "Unknown quality level";
    }
}

} // namespace zenith::ui