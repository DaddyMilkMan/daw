/*
  ==============================================================================
    agents/ObservabilityAgent/PrometheusExporter.cpp
    Lightweight Prometheus text format exporter implementation.
  ==============================================================================
*/

#include "PrometheusExporter.h"

namespace zenith {
namespace agents {

juce::Result PrometheusExporter::exportMetrics(const std::vector<ObservabilityAgent::Metric>& metrics,
                                               const juce::File& destination) {
    if (metrics.empty()) {
        return juce::Result::ok();
    }

    // 1. Generate content in memory
    juce::String output;
    // Pre-allocate decent size to avoid reallocations
    output.preallocateBytes(metrics.size() * 128);

    for (const auto& metric : metrics) {
        output += formatMetric(metric);
    }

    // 2. Atomic Write Strategy: Write to .tmp file then rename
    // This prevents partial reads by the scraper (node_exporter/telegraf)
    auto tempFile = destination.withFileExtension("tmp");

    // Ensure parent directory exists
    auto parent = destination.getParentDirectory();
    if (!parent.exists()) {
        auto result = parent.createDirectory();
        if (result.failed()) {
            return result;
        }
    }

    // Write to temp file
    if (!tempFile.replaceWithText(output)) {
        return juce::Result::fail("Failed to write to temporary file: " + tempFile.getFullPathName());
    }

    // Rename temp file to destination (atomic on POSIX, usually atomic enough on Windows for this purpose)
    if (!tempFile.moveFileTo(destination)) {
        // Cleanup on failure
        tempFile.deleteFile();
        return juce::Result::fail("Failed to rename temp file to destination: " + destination.getFullPathName());
    }

    return juce::Result::ok();
}

juce::String PrometheusExporter::formatMetric(const ObservabilityAgent::Metric& metric) {
    juce::String s;
    auto safeName = sanitizeName(metric.name);

    // TYPE
    s << "# TYPE " << safeName << " " << getTypeString(metric.type) << "\n";

    // HELP (Defaulting to simple help text as we don't store help strings in Metric struct yet)
    s << "# HELP " << safeName << " Metric export from Zenith DAW\n";

    // METRIC LINE: name{labels} value timestamp
    s << safeName;

    if (!metric.labels.empty()) {
        s << formatLabels(metric.labels);
    }

    s << " " << juce::String(metric.value);

    // Timestamp omitted: The "Textfile Collector" pattern lets the scraper (e.g., node_exporter)
    // assign the timestamp when the file is read. This avoids issues with clock skew and
    // steady_clock vs system_clock mismatches (steady_clock is monotonic, not epoch-based).

    s << "\n";
    return s;
}

juce::String PrometheusExporter::getTypeString(ObservabilityAgent::MetricType type) {
    switch (type) {
        case ObservabilityAgent::MetricType::Counter:   return "counter";
        case ObservabilityAgent::MetricType::Gauge:     return "gauge";
        case ObservabilityAgent::MetricType::Histogram: return "histogram"; // Complex type, treating as simple value for now
        case ObservabilityAgent::MetricType::Timer:     return "summary";   // Timer usually maps to summary or histogram
        default:                                        return "untyped";
    }
}

juce::String PrometheusExporter::formatLabels(const std::vector<std::pair<std::string, std::string>>& labels) {
    juce::String s = "{";
    for (size_t i = 0; i < labels.size(); ++i) {
        s << sanitizeName(labels[i].first) << "=\""
          << sanitizeLabelValue(labels[i].second) << "\"";

        if (i < labels.size() - 1) {
            s << ",";
        }
    }
    s << "}";
    return s;
}

juce::String PrometheusExporter::sanitizeName(const std::string& name) {
    // Regex: [a-zA-Z_:][a-zA-Z0-9_:]*
    // Simple sanitization: replace invalid chars with underscore
    juce::String s(name);
    return s.replaceCharacter('-', '_')
            .replaceCharacter(' ', '_')
            .replaceCharacter('.', '_');
}

juce::String PrometheusExporter::sanitizeLabelValue(const std::string& value) {
    juce::String s(value);
    // Escape backslash, double-quote, newline
    s = s.replace("\\", "\\\\");
    s = s.replace("\"", "\\\"");
    s = s.replace("\n", "\\n");
    return s;
}

} // namespace agents
} // namespace zenith
