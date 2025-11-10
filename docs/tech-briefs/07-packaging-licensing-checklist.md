# Packaging & Licensing Checklist for Zenith DAW

**Release Requirements for Commercial Distribution**
**Version:** 1.0
**Date:** 2025-11-10

---

## Executive Summary

This document provides a comprehensive checklist for packaging and licensing Zenith DAW for commercial distribution on Windows and macOS. It covers:

1. **Software Licensing** (JUCE, VST3, CEF)
2. **Code Signing** (Windows & macOS)
3. **Notarization** (macOS)
4. **Installer Creation** (MSIX, DMG/PKG)
5. **Trademark Usage** (VST logo)

---

## Software Licensing Requirements

### 1. JUCE Framework License

**Official Documentation:**
- JUCE 8 EULA: https://juce.com/legal/juce-8-licence/
- JUCE License Overview: https://juce.com/get-juce/
- GitHub LICENSE.md: https://github.com/juce-framework/JUCE/blob/master/LICENSE.md

**Dual Licensing Structure:**
> "JUCE Framework modules are dual-licensed under the AGPLv3 and the commercial JUCE licence."
> — [JUCE LICENSE.md](https://github.com/juce-framework/JUCE/blob/master/LICENSE.md)

**AGPLv3 Option:**
> "If you are 'propagating' or 'conveying' closed-source software containing JUCE outside of your organisation then you may be violating the terms of the AGPLv3. The creation and use of 'in-house' tools and the internal development of 'pre-release' software is usually permitted under the AGPLv3."
> — [JUCE Forum: JUCE8 License and Open-Source Projects](https://forum.juce.com/t/juce8-license-and-open-source-projects/60987)

**Commercial License Requirement:**
> "If you are not using JUCE under the AGPLv3 then you will require a JUCE licence, and you will need to maintain a licence for at least the duration over which you are distributing closed-source binaries containing JUCE."
> — [JUCE Forum: JUCE 8 EULA](https://forum.juce.com/t/archived-juce-8-eula/60947)

**JUCE 8 EULA (2024):**
> "The JUCE 8 End User Licence Agreement was published on May 20th, 2024. All JUCE subscriptions are subject to the JUCE 8 EULA."
> — [JUCE Forum: JUCE 8 EULA](https://forum.juce.com/t/archived-juce-8-eula/60947)

#### Checklist

- [ ] **Decide on licensing model:**
  - [ ] **Open Source (AGPLv3):** Publish Zenith DAW source code under AGPLv3
  - [ ] **Commercial (Closed-Source):** Purchase JUCE commercial license

- [ ] **If Commercial License:**
  - [ ] Sign up for JUCE subscription at https://juce.com/get-juce/
  - [ ] Maintain active subscription while distributing binaries
  - [ ] Include JUCE attribution in "About" dialog (optional but recommended)

- [ ] **If AGPLv3:**
  - [ ] Publish full source code of Zenith DAW (including build scripts)
  - [ ] Include AGPLv3 license text in distribution
  - [ ] Provide source access to end-users

**Recommendation:** Use **commercial JUCE license** for Zenith DAW (closed-source product).

---

### 2. VST3 SDK License

**Official Documentation:**
- VST 3 Licensing: https://steinbergmedia.github.io/vst3_dev_portal/pages/VST+3+Licensing/Index.html
- VST 3 SDK Licensing FAQ: https://forums.steinberg.net/t/vst-3-sdk-licensing-faq/201638
- GitHub License: https://github.com/steinbergmedia/vst3sdk/blob/master/LICENSE.txt

**Major Change (2024):**
> "The VST 3 SDK is now available under the MIT Open Source License, representing a significant shift from previous licensing models. Licensing under GPLv3 and the Steinberg proprietary license is no longer available."
> — [KVR Audio: Steinberg Moves VST 3 SDK to MIT](https://www.kvraudio.com/news/steinberg-moves-vst-3-sdk-to-mit-open-source-license-asio-now-gplv3-65179)

**Redistribution:**
> "Code licensed under MIT license can be used, modified, and redistributed freely — including in commercial products — provided MIT license terms are followed."
> — [Steinberg Forums: VST 3 SDK Licensing FAQ](https://forums.steinberg.net/t/vst-3-sdk-licensing-faq/201638)

**No License Agreement Required:**
> "Developers no longer need to sign or manage a proprietary Steinberg VST3 license agreement."
> — [Steinberg Developer Help: VST 3 Licensing](https://steinbergmedia.github.io/vst3_dev_portal/pages/VST+3+Licensing/Index.html)

**Source Code Disclosure:**
> "The MIT License does not require you to disclose your source code."
> — [Steinberg Forums: VST 3 SDK Licensing FAQ](https://forums.steinberg.net/t/vst-3-sdk-licensing-faq/201638)

**Trademark Usage:**
> "'VST' name or logo is optional under MIT license, but if used, must comply with Steinberg's official trademark rules."
> — [Steinberg Forums: VST 3 SDK Licensing FAQ](https://forums.steinberg.net/t/vst-3-sdk-licensing-faq/201638)

**Critical: VST2 Exclusion:**
> ⚠️ "Be sure that you do NOT include the Steinberg VST 2 files: like `aeffect.h` and `aeffectx.h`, as these remain under a different, more restrictive license."
> — [Steinberg Developer Help: VST 3 Licensing](https://developer.steinberg.help/pages/viewpage.action?pageId=9797946)

#### Checklist

- [ ] **Use VST3 SDK (MIT License)**
  - [ ] Download from https://github.com/steinbergmedia/vst3sdk
  - [ ] Include MIT license text in `LICENSES/VST3_SDK.txt`
  - [ ] **Do NOT** include VST2 headers (`aeffect.h`, `aeffectx.h`)

- [ ] **VST Trademark Usage (Optional)**
  - [ ] If using "VST" logo: Follow Steinberg trademark guidelines
  - [ ] Recommended: Display "VST is a trademark of Steinberg Media Technologies GmbH"

- [ ] **No Additional Requirements**
  - No license agreement to sign
  - No royalties or fees
  - No source code disclosure required

**Result:** VST3 is **free to use** for commercial products under MIT license.

---

### 3. Audio Unit (AU) License (macOS)

**Official Documentation:**
- Audio Unit Framework: https://developer.apple.com/documentation/audiounit

**License:**
Audio Unit APIs are part of Apple's macOS SDK, provided under the **Apple Software License Agreement**.

#### Checklist

- [ ] **Accept Apple Developer License**
  - Automatically accepted when joining Apple Developer Program

- [ ] **No Additional Requirements**
  - No fees for hosting Audio Units
  - No special licensing for AU support

**Result:** AU hosting is **free** on macOS (no licensing concerns).

---

### 4. CEF (Chromium Embedded Framework) License

**Official Documentation:**
- CEF GitHub: https://github.com/chromiumembedded/cef
- CEF License (BSD): https://bitbucket.org/chromiumembedded/cef/src/master/LICENSE.txt

**License:**
CEF is licensed under **BSD 3-Clause License** (permissive, commercial-friendly).

#### Checklist

- [ ] **Include CEF License**
  - [ ] Copy `LICENSE.txt` to `LICENSES/CEF.txt` in distribution
  - [ ] Display "Chromium Embedded Framework" in "About" dialog (optional but recommended)

- [ ] **Bundle CEF Binaries**
  - [ ] Windows: Include `libcef.dll`, `chrome_elf.dll`, resources, etc.
  - [ ] macOS: Include `Chromium Embedded Framework.framework`

**Result:** CEF is **free to use** for commercial products.

---

## Code Signing & Notarization

### macOS: Code Signing & Notarization

**Official Documentation:**
- Notarizing macOS Software: https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution
- Melatonin Guide (Audio Plugins): https://melatonin.dev/blog/how-to-code-sign-and-notarize-macos-audio-plugins-in-ci/

**Requirements:**
> "All notarized applications must use the 'runtime' option and include an authenticated date/time stamp in the signature. You need a paid Apple Developer Program subscription, which costs about $99 per year."
> — [Melatonin: Code Sign and Notarize Audio Plugins](https://melatonin.dev/blog/how-to-code-sign-and-notarize-macos-audio-plugins-in-ci/)

> "Mac software distributed outside the Mac App Store must be notarized by Apple in order to run on macOS Catalina and later versions."
> — [Apple Developer: Notarizing Your Mac Software](https://developer.apple.com/news/?id=09032019a)

#### Step-by-Step Checklist

##### 1. Join Apple Developer Program

- [ ] **Enroll in Apple Developer Program**
  - Cost: $99/year
  - Sign up at: https://developer.apple.com/programs/

- [ ] **Create Certificates**
  - [ ] "Developer ID Application" certificate (for signing .app)
  - [ ] "Developer ID Installer" certificate (for signing .pkg)

##### 2. Enable Hardened Runtime

- [ ] **Add `--options runtime` flag to codesign**
  ```bash
  codesign --sign "Developer ID Application: Your Company" \
           --options runtime \
           --timestamp \
           --deep \
           --force \
           ZenithDAW.app
  ```

- [ ] **Create Entitlements File (if needed)**
  ```xml
  <?xml version="1.0" encoding="UTF-8"?>
  <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "...">
  <plist version="1.0">
  <dict>
      <key>com.apple.security.device.audio-input</key>
      <true/>
      <key>com.apple.security.device.microphone</key>
      <true/>
      <key>com.apple.security.cs.allow-unsigned-executable-memory</key>
      <true/>  <!-- Required for JIT-compiled plugins -->
  </dict>
  </plist>
  ```

##### 3. Sign All Binaries

> "You need to code sign each binary, so you would have one line for each AU/VST3/.app or whatever it is you are distributing."
> — [Melatonin: Code Sign and Notarize Audio Plugins](https://melatonin.dev/blog/how-to-code-sign-and-notarize-macos-audio-plugins-in-ci/)

- [ ] **Sign in Correct Order (inside-out):**
  1. [ ] Embedded frameworks (e.g., `CEF.framework`)
  2. [ ] Plugins (VST3/AU bundles)
  3. [ ] Main application bundle (`.app`)
  4. [ ] Installer (`.pkg` or `.dmg`)

##### 4. Notarize with Apple

- [ ] **Create .zip or .dmg for notarization**
  ```bash
  ditto -c -k --keepParent ZenithDAW.app ZenithDAW.zip
  ```

- [ ] **Submit to Apple Notary Service**
  ```bash
  xcrun notarytool submit ZenithDAW.zip \
      --apple-id your@email.com \
      --team-id ABCDE12345 \
      --password "app-specific-password"
  ```

- [ ] **Wait for approval** (usually 5-30 minutes)

- [ ] **Staple notarization ticket**
  ```bash
  xcrun stapler staple ZenithDAW.app
  ```

##### 5. Verify

- [ ] **Check codesign:**
  ```bash
  codesign --verify --deep --strict --verbose=2 ZenithDAW.app
  spctl --assess --verbose=4 ZenithDAW.app
  ```

- [ ] **Test on clean macOS system** (not your dev machine)

**Common Issues:**

> ⚠️ "The `--options runtime` flag enables the Hardened Runtime, which is necessary to pass notarization, but this might interfere with your app as it disables JIT and other dynamic features."
> — [Big Binary: Code Sign and Notarize Electron App](https://bigbinary.com/blog/code-sign-notorize-mac-desktop-app)

**Solution:** Add entitlements for JIT (`com.apple.security.cs.allow-unsigned-executable-memory`) if hosting plugins that use JIT compilation.

---

### Windows: Code Signing

**Official Documentation:**
- Sign a Windows App Package: https://learn.microsoft.com/en-us/windows/msix/package/signing-package-overview
- MSIX Code Signing: https://www.advancedinstaller.com/msix-certificates-developer.html

**Requirements:**
> "MSIX package signing is mandatory, unlike MSI installers where it was optional. MSIX packages must be signed using a code signing certificate that is trusted by the end device."
> — [Advanced Installer: MSIX and Code Signing Certificates](https://www.advancedinstaller.com/msix-certificates-developer.html)

> "The certificate has to chain to one of the trusted roots on the device. By default, Windows 10 trusts certificates from most of the certificate authorities that provide code signing certificates."
> — [Microsoft Learn: Signing Package Overview](https://learn.microsoft.com/en-us/windows/msix/package/signing-package-overview)

#### Step-by-Step Checklist

##### 1. Obtain Code Signing Certificate

- [ ] **Purchase from Trusted CA:**
  - DigiCert
  - Sectigo
  - GlobalSign
  - Cost: ~$200-$500/year

- [ ] **Certificate Requirements:**
  - [ ] Subject must match publisher in `AppxManifest.xml`
  - [ ] Must chain to trusted root (Windows Trust Store)
  - [ ] EV (Extended Validation) recommended for SmartScreen reputation

##### 2. Sign MSIX Package

- [ ] **Sign with `signtool.exe`:**
  ```cmd
  signtool sign /fd SHA256 /a /f MyCert.pfx /p password /tr http://timestamp.digicert.com ZenithDAW.msix
  ```

- [ ] **Include Timestamp:**
  > "It is highly recommended that timestamping is used when signing your app, as it preserves the signature allowing the app package to be accepted even after the certificate has expired."
  > — [Microsoft Learn: Create Certificate for Package Signing](https://learn.microsoft.com/en-us/windows/msix/package/create-certificate-package-signing)

##### 3. Verify Signature

- [ ] **Check signature:**
  ```cmd
  signtool verify /pa ZenithDAW.msix
  ```

- [ ] **Test installation on clean Windows system**

**Alternative: Traditional .exe Installer**

If not using MSIX:

- [ ] **Sign .exe with Authenticode:**
  ```cmd
  signtool sign /fd SHA256 /a /f MyCert.pfx /p password /tr http://timestamp.digicert.com ZenithDAW-Setup.exe
  ```

---

## Installer Creation

### macOS: DMG or PKG?

**DMG (Disk Image):**

> ⚠️ "Apple broke .dmg's usefulness for audio plugins when Gatekeeper and notarization was introduced - users can no longer drag and drop to ~/Library or /Library symlinks in a dmg, though it's fixed again in Ventura 14.0 but got reports of it broken in 14.1."
> — [Melatonin: Code Sign and Notarize Audio Plugins](https://melatonin.dev/blog/how-to-code-sign-and-notarize-macos-audio-plugins-in-ci/)

**PKG (Installer Package):**
- More reliable for system-wide installations
- Can run scripts (copy to /Applications, etc.)
- Requires "Developer ID Installer" certificate

#### Checklist

- [ ] **Choose Installer Format:**
  - [ ] **DMG:** Simple drag-and-drop (test on Ventura 14.0+)
  - [ ] **PKG:** Automated installation (recommended for DAWs)

- [ ] **Sign Installer:**
  - [ ] DMG: `codesign --sign "Developer ID Application" ZenithDAW.dmg`
  - [ ] PKG: `productsign --sign "Developer ID Installer" ZenithDAW.pkg ZenithDAW-signed.pkg`

- [ ] **Notarize Installer** (follow steps above)

---

### Windows: MSIX or EXE Installer?

**MSIX (Modern):**
- Sandboxed, secure
- Auto-updates via Microsoft Store
- Requires code signing certificate

**EXE Installer (Traditional):**
- NSIS, Inno Setup, WiX
- Full system access
- Easier to deploy outside Store

#### Checklist

- [ ] **Choose Installer Format:**
  - [ ] **MSIX:** For Microsoft Store distribution
  - [ ] **EXE Installer:** For direct download (recommended for DAWs)

- [ ] **Sign Installer** (see code signing section above)

- [ ] **Test on Clean Windows VM**
  - [ ] Windows 10 (version 2004+)
  - [ ] Windows 11

---

## Trademark & Branding Compliance

### VST Logo Usage

**Official Guidance:**
- Steinberg VST Usage Guidelines: https://steinbergmedia.github.io/vst3_dev_portal/pages/VST+3+Licensing/Index.html

**Optional Usage:**
> "'VST' name or logo is optional under MIT license, but if used, must comply with Steinberg's official trademark rules. Following the Steinberg VST usage guidelines is considered best practice, but it is optional."
> — [Steinberg Forums: VST 3 SDK Licensing FAQ](https://forums.steinberg.net/t/vst-3-sdk-licensing-faq/201638)

#### Checklist

- [ ] **If Using "VST" Logo:**
  - [ ] Follow Steinberg's official trademark guidelines
  - [ ] Display: "VST is a trademark of Steinberg Media Technologies GmbH"

- [ ] **If NOT Using Logo:**
  - [ ] No special requirements (still allowed to say "Supports VST3 plugins")

---

## Final Release Checklist

### Pre-Release

- [ ] **License Compliance:**
  - [ ] JUCE commercial license purchased (if closed-source)
  - [ ] VST3 SDK MIT license included in `LICENSES/` folder
  - [ ] CEF license included in `LICENSES/` folder

- [ ] **Code Signing:**
  - [ ] **macOS:** All binaries signed with Developer ID
  - [ ] **Windows:** Installer signed with EV certificate

- [ ] **Notarization:**
  - [ ] **macOS:** App notarized by Apple, ticket stapled

- [ ] **Testing:**
  - [ ] Install on clean macOS system (not dev machine)
  - [ ] Install on clean Windows system (not dev machine)
  - [ ] Verify no Gatekeeper warnings (macOS)
  - [ ] Verify no SmartScreen warnings (Windows)

### Distribution

- [ ] **Create Installers:**
  - [ ] **macOS:** `ZenithDAW-1.0.0-macOS.dmg` or `.pkg`
  - [ ] **Windows:** `ZenithDAW-1.0.0-Setup.exe` or `.msix`

- [ ] **Upload to Distribution Channels:**
  - [ ] Company website
  - [ ] Plugin Boutique / Sweetwater (if applicable)
  - [ ] Microsoft Store (if using MSIX)

- [ ] **Verify Download Links:**
  - [ ] All download links work
  - [ ] Installers are correctly signed

---

## Estimated Costs

| **Item** | **Cost** | **Frequency** |
|----------|----------|---------------|
| **JUCE Commercial License** | ~$40-$60/month | Ongoing (subscription) |
| **Apple Developer Program** | $99 | Annual |
| **Windows Code Signing Cert** | $200-$500 | Annual |
| **Total (Year 1)** | ~$780-$1,380 | — |

---

## Sources

### JUCE Licensing
1. JUCE 8 EULA: https://juce.com/legal/juce-8-licence/
2. JUCE GitHub LICENSE.md: https://github.com/juce-framework/JUCE/blob/master/LICENSE.md
3. JUCE Forum: JUCE 8 EULA Discussion: https://forum.juce.com/t/archived-juce-8-eula/60947

### VST3 Licensing
4. VST 3 Developer Portal: https://steinbergmedia.github.io/vst3_dev_portal/pages/VST+3+Licensing/Index.html
5. Steinberg Forums: VST 3 SDK Licensing FAQ: https://forums.steinberg.net/t/vst-3-sdk-licensing-faq/201638
6. KVR Audio: VST3 MIT License Announcement: https://www.kvraudio.com/news/steinberg-moves-vst-3-sdk-to-mit-open-source-license-asio-now-gplv3-65179

### macOS Code Signing & Notarization
7. Apple Developer: Notarizing macOS Software: https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution
8. Melatonin: Code Sign and Notarize Audio Plugins: https://melatonin.dev/blog/how-to-code-sign-and-notarize-macos-audio-plugins-in-ci/

### Windows Code Signing
9. Microsoft Learn: Sign a Windows App Package: https://learn.microsoft.com/en-us/windows/msix/package/signing-package-overview
10. Advanced Installer: MSIX and Code Signing: https://www.advancedinstaller.com/msix-certificates-developer.html

---

**Document Version:** 1.0
**Last Updated:** 2025-11-10
**Action:** Review before each release
