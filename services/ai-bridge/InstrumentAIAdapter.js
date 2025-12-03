/**
 * InstrumentAIAdapter.js
 * 
 * Handles interaction with LLM providers and parameter mapping.
 */

class InstrumentAIAdapter {
    constructor(config) {
        this.sendCommand = config.sendCommand;
        this.callLLM = config.callLLM;
    }

    /**
     * Wrapper for calling the LLM. 
     * Validation or logging can be added here.
     */
    async callLLM(promptData) {
        if (!this.callLLM) throw new Error("LLM Provider not configured");
        return await this.callLLM(promptData);
    }
}

module.exports = { InstrumentAIAdapter };