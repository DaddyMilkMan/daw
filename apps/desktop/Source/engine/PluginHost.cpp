/*
  formatManager.addFormat(std::make_unique<InternalPluginFormat>());

  // Get VST3 format pointer for later use
  for (int i = 0; i < formatManager.getNumFormats(); ++i) {
    auto *format = formatManager.getFormat(i);
    if (format->getName().contains("VST3")) {
      vst3Format = format;
      DBG("PluginHost: VST3 format registered");
      break;
    }
  }

  if (vst3Format == nullptr) {
    DBG("PluginHost: WARNING - VST3 format not available!");
  }

  DBG("PluginHost: Initialized");
}

PluginHost::~PluginHost() {
  DBG("PluginHost: Destructor");
  cancelScan();
  if (scanThread && scanThread->isThreadRunning()) {
    scanThread->stopThread(5000); // Wait up to 5 seconds
  }
}

