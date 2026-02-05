/*
  ==============================================================================
    agents/ObservabilityAgent/PrometheusExporter.h
    Lightweight Prometheus text format exporter.
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include "ObservabilityAgent.h"

namespace zenith {
namespace agents {

/**
    Exports metrics to a file in Prometheus text format (0.0.4).
    Uses the "Textfile Collector" pattern with atomic writes.
*/
class PrometheusExporter {
public:
    PrometheusExporter() = default;
    ~PrometheusExporter() = default;

    /**
        Formats metrics and writes them atomically to the destination file.

        @param metrics The list of metrics to export.
        @param destination The target file to write to.
        @return Result::ok() on success, or an error message.
    */
    juce::Result exportMetrics(const std::vector<ObservabilityAgent::Metric>& metrics,
                               const juce::File& destination);

    /**
        Formats a single metric into Prometheus text format line(s).
        Includes HELP and TYPE lines.
    */
    static juce::String formatMetric(const ObservabilityAgent::Metric& metric);

private:
    // Helper to get Prometheus type string
    static juce::String getTypeString(ObservabilityAgent::MetricType type);

    // Helper to format labels
    static juce::String formatLabels(const std::vector<std::pair<std::string, std::string>>& labels);

    // Helper to sanitize metric names (Prometheus regex: [a-zA-Z_:][a-zA-Z0-9_:]*)
    static juce::String sanitizeName(const std::string& name);

    // Helper to sanitize label values (escape backslashes, double-quotes, newlines)
    static juce::String sanitizeLabelValue(const std::string& value);
};

} // namespace agents
} // namespace zenith
