/*
  ==============================================================================
    APILimits.h
    API usage limits and tier management for Zenith DAW
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <chrono>

namespace zenith {
namespace ai {

enum class APITier {
    Light,    // $5 for $10 of API
    Pro,      // $10 for $20 of API
    Heavy     // $25 for $50 of API
};

class APILimits {
public:
    struct TierConfig {
        APITier tier;
        float monthlyCost;      // User pays
        float apiAllowance;     // API credit received
        int requestsPerMinute;
        int requestsPerDay;
        int requestsPerMonth;
    };
    
    struct Usage {
        int requestsThisMinute;
        int requestsToday;
        int requestsThisMonth;
        float costThisMonth;
        juce::Time lastResetTime;
        juce::Time monthStartTime;
    };
    
    APILimits();
    ~APILimits() = default;
    
    // Tier management
    void setTier(APITier tier);
    APITier getCurrentTier() const { return currentTier; }
    TierConfig getCurrentTierConfig() const;
    
    // Usage tracking
    bool canMakeRequest();
    void recordRequest();
    void recordCost(float cost);
    
    // Usage information
    Usage getCurrentUsage() const;
    int getRemainingRequests() const;
    int getRemainingDailyRequests() const;
    int getRemainingMonthlyRequests() const;
    float getRemainingBudget() const;
    
    // Limits
    int getRequestsPerMinute() const;
    int getRequestsPerDay() const;
    int getRequestsPerMonth() const;
    float getMonthlyBudget() const;
    
    // Reset
    void resetDailyUsage();
    void resetMonthlyUsage();
    
    // Configuration
    void loadConfiguration();
    void saveConfiguration();
    
private:
    APITier currentTier;
    std::atomic<int> requestsThisMinute;
    std::atomic<int> requestsToday;
    std::atomic<int> requestsThisMonth;
    std::atomic<float> costThisMonth;
    
    juce::Time lastMinuteReset;
    juce::Time lastDayReset;
    juce::Time monthStart;
    
    static const std::unordered_map<APITier, TierConfig> tierConfigs;
    
    void checkAndResetCounters();
    void resetMinuteCounter();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(APILimits)
};

class APIUsageMonitor {
public:
    APIUsageMonitor(APILimits& limits);
    ~APIUsageMonitor() = default;
    
    // Request tracking
    bool trackRequest();
    void trackResponse(const juce::String& response, int inputTokens, int outputTokens);
    
    // Cost calculation
    float calculateCost(int inputTokens, int outputTokens) const;
    
    // Statistics
    float getAverageResponseTime() const;
    float getSuccessRate() const;
    int getTotalRequests() const;
    
    // Alerts
    bool shouldShowLowBudgetWarning() const;
    bool shouldShowRateLimitWarning() const;
    
private:
    APILimits& limits;
    
    std::atomic<int> totalRequests;
    std::atomic<int> successfulRequests;
    std::atomic<float> totalResponseTime;
    std::atomic<int> lastMinuteRequests;
    
    juce::Array<float> recentResponseTimes;
    juce::Time lastAlertTime;
    
    void updateStatistics();
    void checkLimits();
};

} // namespace ai
} // namespace zenith
