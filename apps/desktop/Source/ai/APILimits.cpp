/*
  ==============================================================================
    APILimits.cpp
    API usage limits and tier management implementation
  ==============================================================================
*/

#include "APILimits.h"
#include <fstream>

namespace zenith {
namespace ai {

// Tier configurations
const std::unordered_map<APITier, APILimits::TierConfig> APILimits::tierConfigs = {
    {APITier::Light, {
        APITier::Light,
        5.0f,    // $5 monthly cost
        10.0f,   // $10 API allowance
        20,      // 20 requests per minute
        500,     // 500 requests per day
        10000    // 10,000 requests per month
    }},
    {APITier::Pro, {
        APITier::Pro,
        10.0f,   // $10 monthly cost
        20.0f,   // $20 API allowance
        50,      // 50 requests per minute
        1500,    // 1,500 requests per day
        30000    // 30,000 requests per month
    }},
    {APITier::Heavy, {
        APITier::Heavy,
        25.0f,   // $25 monthly cost
        50.0f,   // $50 API allowance
        100,     // 100 requests per minute
        5000,    // 5,000 requests per day
        100000   // 100,000 requests per month
    }}
};

// APILimits Implementation
APILimits::APILimits() 
    : currentTier(APITier::Pro),
      requestsThisMinute(0),
      requestsToday(0),
      requestsThisMonth(0),
      costThisMonth(0.0f),
      lastMinuteReset(juce::Time::getCurrentTime()),
      lastDayReset(juce::Time::getCurrentTime()),
      monthStart(juce::Time::getCurrentTime()) {
    
    loadConfiguration();
}

void APILimits::setTier(APITier tier) {
    currentTier = tier;
    saveConfiguration();
    
    // Reset counters when changing tier
    resetDailyUsage();
    resetMonthlyUsage();
}

APILimits::TierConfig APILimits::getCurrentTierConfig() const {
    auto it = tierConfigs.find(currentTier);
    if (it != tierConfigs.end()) {
        return it->second;
    }
    return tierConfigs.at(APITier::Pro); // Default to Pro
}

bool APILimits::canMakeRequest() const {
    checkAndResetCounters();
    
    auto config = getCurrentTierConfig();
    
    return (requestsThisMinute.load() < config.requestsPerMinute) &&
           (requestsToday.load() < config.requestsPerDay) &&
           (requestsThisMonth.load() < config.requestsPerMonth) &&
           (costThisMonth.load() < config.apiAllowance);
}

void APILimits::recordRequest() {
    checkAndResetCounters();
    
    requestsThisMinute++;
    requestsToday++;
    requestsThisMonth++;
}

void APILimits::recordCost(float cost) {
    costThisMonth += cost;
    saveConfiguration();
}

APILimits::Usage APILimits::getCurrentUsage() const {
    checkAndResetCounters();
    
    Usage usage;
    usage.requestsThisMinute = requestsThisMinute.load();
    usage.requestsToday = requestsToday.load();
    usage.requestsThisMonth = requestsThisMonth.load();
    usage.costThisMonth = costThisMonth.load();
    usage.lastResetTime = lastMinuteReset;
    usage.monthStartTime = monthStart;
    
    return usage;
}

int APILimits::getRemainingRequests() const {
    checkAndResetCounters();
    auto config = getCurrentTierConfig();
    return config.requestsPerMinute - requestsThisMinute.load();
}

int APILimits::getRemainingDailyRequests() const {
    checkAndResetCounters();
    auto config = getCurrentTierConfig();
    return config.requestsPerDay - requestsToday.load();
}

int APILimits::getRemainingMonthlyRequests() const {
    checkAndResetCounters();
    auto config = getCurrentTierConfig();
    return config.requestsPerMonth - requestsThisMonth.load();
}

float APILimits::getRemainingBudget() const {
    auto config = getCurrentTierConfig();
    return config.apiAllowance - costThisMonth.load();
}

int APILimits::getRequestsPerMinute() const {
    return getCurrentTierConfig().requestsPerMinute;
}

int APILimits::getRequestsPerDay() const {
    return getCurrentTierConfig().requestsPerDay;
}

int APILimits::getRequestsPerMonth() const {
    return getCurrentTierConfig().requestsPerMonth;
}

float APILimits::getMonthlyBudget() const {
    return getCurrentTierConfig().apiAllowance;
}

void APILimits::resetDailyUsage() {
    requestsToday = 0;
    lastDayReset = juce::Time::getCurrentTime();
    saveConfiguration();
}

void APILimits::resetMonthlyUsage() {
    requestsThisMonth = 0;
    costThisMonth = 0.0f;
    monthStart = juce::Time::getCurrentTime();
    saveConfiguration();
}

void APILimits::loadConfiguration() {
    juce::File configFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                           .getChildFile("ZenithDAW")
                           .getChildFile("api_limits.json");
    
    if (configFile.exists()) {
        try {
            auto content = configFile.loadFileAsString();
            auto json = juce::JSON::parse(content);
            
            if (json.isObject()) {
                // Load tier
                juce::String tierStr = json.getProperty("tier", "Pro");
                if (tierStr == "Light") currentTier = APITier::Light;
                else if (tierStr == "Heavy") currentTier = APITier::Heavy;
                else currentTier = APITier::Pro;
                
                // Load usage
                auto usage = json.getProperty("usage", juce::var());
                if (usage.isObject()) {
                    requestsToday = usage.getProperty("requestsToday", 0);
                    requestsThisMonth = usage.getProperty("requestsThisMonth", 0);
                    costThisMonth = usage.getProperty("costThisMonth", 0.0f);
                    
                    double lastDayTime = usage.getProperty("lastDayReset", 0.0);
                    lastDayReset = juce::Time(lastDayTime);
                    
                    double monthStartTime = usage.getProperty("monthStart", 0.0);
                    monthStart = juce::Time(monthStartTime);
                }
            }
        } catch (const std::exception& e) {
            juce::Logger::writeToLog("Failed to load API limits config: " + juce::String(e.what()));
        }
    }
}

void APILimits::saveConfiguration() {
    juce::File configFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                           .getChildFile("ZenithDAW")
                           .getChildFile("api_limits.json");
    
    // Ensure directory exists
    configFile.getParentDirectory().createDirectory();
    
    juce::DynamicObject::Ptr config = new juce::DynamicObject();
    
    // Save tier
    juce::String tierStr;
    switch (currentTier) {
        case APITier::Light: tierStr = "Light"; break;
        case APITier::Pro: tierStr = "Pro"; break;
        case APITier::Heavy: tierStr = "Heavy"; break;
    }
    config->setProperty("tier", tierStr);
    
    // Save usage
    juce::DynamicObject::Ptr usage = new juce::DynamicObject();
    usage->setProperty("requestsToday", requestsToday.load());
    usage->setProperty("requestsThisMonth", requestsThisMonth.load());
    usage->setProperty("costThisMonth", costThisMonth.load());
    usage->setProperty("lastDayReset", lastDayReset.toMilliseconds());
    usage->setProperty("monthStart", monthStart.toMilliseconds());
    
    config->setProperty("usage", usage);
    
    configFile.replaceWithText(juce::JSON::toString(config));
}

void APILimits::checkAndResetCounters() const {
    auto now = juce::Time::getCurrentTime();
    
    // Check minute counter
    if (now - lastMinuteReset > juce::RelativeTime(60.0)) {
        const_cast<APILimits*>(this)->resetMinuteCounter();
    }
    
    // Check day counter
    if (now.getDayOfMonth() != lastDayReset.getDayOfMonth()) {
        const_cast<APILimits*>(this)->resetDailyUsage();
    }
    
    // Check month counter
    if (now.getMonth() != monthStart.getMonth() || now.getYear() != monthStart.getYear()) {
        const_cast<APILimits*>(this)->resetMonthlyUsage();
    }
}

void APILimits::resetMinuteCounter() {
    requestsThisMinute = 0;
    lastMinuteReset = juce::Time::getCurrentTime();
}

// APIUsageMonitor Implementation
APIUsageMonitor::APIUsageMonitor(APILimits& limits) 
    : limits(limits),
      totalRequests(0),
      successfulRequests(0),
      totalResponseTime(0.0f),
      lastMinuteRequests(0),
      lastAlertTime(juce::Time::getCurrentTime()) {
}

bool APIUsageMonitor::trackRequest() {
    if (!limits.canMakeRequest()) {
        return false;
    }
    
    limits.recordRequest();
    totalRequests++;
    lastMinuteRequests++;
    
    checkLimits();
    return true;
}

void APIUsageMonitor::trackResponse(const juce::String& response, int inputTokens, int outputTokens) {
    // Record successful request
    successfulRequests++;
    
    // Calculate and record cost
    float cost = calculateCost(inputTokens, outputTokens);
    limits.recordCost(cost);
    
    // Update response time statistics
    // This would need actual timing implementation
    updateStatistics();
}

float APIUsageMonitor::calculateCost(int inputTokens, int outputTokens) const {
    // Grok API pricing (example rates)
    const float inputCostPerToken = 0.0001f;  // $0.10 per 1M tokens
    const float outputCostPerToken = 0.0003f; // $0.30 per 1M tokens
    
    return (inputTokens * inputCostPerToken) + (outputTokens * outputCostPerToken);
}

float APIUsageMonitor::getAverageResponseTime() const {
    int total = totalRequests.load();
    if (total == 0) return 0.0f;
    
    return totalResponseTime.load() / total;
}

float APIUsageMonitor::getSuccessRate() const {
    int total = totalRequests.load();
    if (total == 0) return 0.0f;
    
    return (static_cast<float>(successfulRequests.load()) / total) * 100.0f;
}

int APIUsageMonitor::getTotalRequests() const {
    return totalRequests.load();
}

bool APIUsageMonitor::shouldShowLowBudgetWarning() const {
    float remaining = limits.getRemainingBudget();
    float totalBudget = limits.getMonthlyBudget();
    
    // Show warning if less than 20% budget remaining
    return (remaining / totalBudget) < 0.2f;
}

bool APIUsageMonitor::shouldShowRateLimitWarning() const {
    int remaining = limits.getRemainingRequests();
    int total = limits.getRequestsPerMinute();
    
    // Show warning if less than 10% requests remaining
    return (static_cast<float>(remaining) / total) < 0.1f;
}

void APIUsageMonitor::updateStatistics() {
    // Update rolling average of response times
    // This would track actual response times
}

void APIUsageMonitor::checkLimits() {
    auto now = juce::Time::getCurrentTime();
    
    // Don't spam alerts (only once per minute)
    if (now - lastAlertTime < juce::RelativeTime(60.0)) {
        return;
    }
    
    // Check for rate limit warning
    if (shouldShowRateLimitWarning()) {
        // Show warning
        lastAlertTime = now;
    }
    
    // Check for budget warning
    if (shouldShowLowBudgetWarning()) {
        // Show warning
        lastAlertTime = now;
    }
}

} // namespace ai
} // namespace zenith
