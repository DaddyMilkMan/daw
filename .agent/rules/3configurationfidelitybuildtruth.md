---
trigger: always_on
---

Trigger: Build/Config Files

Source of Truth: CMakeLists.txt is the only authority for build configuration. The Agent must ignore hardcoded paths in .clangd or IDE files.

Hardcoding Ban: The insertion of absolute paths (e.g., C:/zenith/...) or platform specific defines (e.g., -D_WIN32) into configuration files is prohibited.

Refactor Requirement: If hardcoded paths are detected, the Agent must immediately propose a refactor using CMake variables.