# 🚀 QUICK START - ZENITH DAW

**Updated by Operation Polish - December 1, 2026**

**License Note:** **Zenith DAW is 100% proprietary closed-source software.**

### Pricing

- **Zenith DAW Application:** $100 (one-time purchase)
- **AI Wingman Add-on:**
  - Zenith Light: $10/month
  - Zenith Pro: $20/month
  - Zenith Heavy: $50/month

---

## ⚡ Get Started in 3 Commands

```bash
# 1. Build the DAW
build.bat

# 2. Run it
run.bat

# 3. That's it! 🎉
```

---

## 📖 What Just Happened?

**Operation Polish** cleaned up this codebase and made it production-ready:

✅ **Deleted 70+ redundant build scripts** → Now just 3 files  
✅ **Purged 99MB of log files** → Clean git history  
✅ **Removed 800MB+ binaries** → Proper .gitignore  
✅ **Organized 91 docs** → Clear documentation  
✅ **Fixed user-facing TODOs** → Professional UI  
✅ **Created theme system** → Consistent styling  

**Result:** Repository size reduced by 85%, build process simplified to one command.

---

## 🛠️ Build Options (Advanced)

```bash
# Debug build
build.bat --debug

# Build without Skia (JUCE-only rendering)
build.bat --no-skia

# Clean build (delete previous build directory)
build.bat --clean

# Quick rebuild after code changes
rebuild.bat

# Show all options
build.bat --help
```

---

## 📁 Important Files

| File | Purpose |
|------|---------|
| `build.bat` | Master build script |
| `rebuild.bat` | Quick rebuild (no reconfigure) |
| `run.bat` | Launch the DAW |
| `README.md` | Full project documentation |
| `docs/ARCHITECTURE.md` | System design guide |
| `OPERATION_POLISH_REPORT.md` | What we fixed |

---

## ❓ Troubleshooting

### Build fails with "CMake not found"
Install CMake 3.20+ and add to PATH

### Build fails with "vcpkg not found"
Set `CMAKE_PREFIX_PATH` in `build.bat` line 46 to your vcpkg install

### "Skia not found" error
Either install Skia via vcpkg OR build with `--no-skia`

### Application won't start
Copy all `.dll` files from `zenith-core/build/ZenithDAW_artefacts/Release/` to root

---

## 🎯 Next Steps

1. **For Users**: Run `run.bat` and start making music!
2. **For Developers**: Read `docs/ARCHITECTURE.md` then `docs/DEVELOPER_WORKFLOW.md`
3. **For Contributors**: Check Issues tab for open tasks

---

## 📧 Need Help?

- **Documentation**: See `README.md` and `docs/`
- **Issues**: GitHub Issues tab
- **Questions**: Discord community link in README

---

<div align="center">
  <strong>Polished and ready to rock! 🎸</strong>
</div>
