# Zenith DAW Cleanup Plan
Generated: January 1, 2026

## Phase 1: File Removal (Agent: AuditAgent, 30 min)
Files to delete immediately:
- [ ] `daw/compile_commands.json`
- [ ] `Testing/` directory
- [ ] `*.log` files in root/daw
- [ ] `daw_gui_log*.txt` (if present)
- [ ] `build_error*.txt` (if present)

Commands:
```bash
rm -f daw/compile_commands.json
rm -rf Testing/
find . -maxdepth 2 -name "*.log" -delete
find . -maxdepth 2 -name "daw_gui_log*.txt" -delete
find . -maxdepth 2 -name "build_error*.txt" -delete
```

## Phase 2: Documentation Archive (Agent: DocAgent, 60 min)
Files to move to docs/archive/:
- [ ] ROAST_REPORT_*.md (all versions)
- [ ] *_SUMMARY.md files
- [ ] *_STATUS.md files
- [ ] [list all historical docs]

## Phase 3: Code Organization (Agent: ArchitectAgent, 90 min)
Directories to validate/create:
- [ ] include/rendering/ - Skia rendering classes
- [ ] include/state/ - ValueTree state management
- [ ] include/audio/ - Audio engine
- [ ] src/ (mirrors include/)
- [ ] tests/ - Unit tests
- [ ] docs/architecture/ - Architecture documentation

## Phase 4: Git Configuration (Agent: AuditAgent, 15 min)
Update `.gitignore`:
```gitignore
# Build artifacts
build/
*.txt
!CMakeLists.txt
!*/CMakeLists.txt
compile_commands.json
Testing/
*.log

# IDE files
.vscode/settings.json
.idea/
*.swp
*.DS_Store
Thumbs.db
*.tmp
*~

# Node modules (Crucial add)
node_modules/
```

## Phase 5: Validation (Agent: ValidationAgent, 30 min)
Checklist:
- [ ] No build artifacts in repo
- [ ] All docs organized
- [ ] Git status clean
- [ ] Repository size reduced (if artifacts were large)
