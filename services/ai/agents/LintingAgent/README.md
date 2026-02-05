# LintingAgent

## Purpose

The LintingAgent provides comprehensive code quality checks, enforcing naming conventions, style rules, and detecting common code quality issues. It analyzes both C++ and Python code to ensure consistency with project standards and maintainability best practices.

## Features

### Naming Convention Checks
- **C++**: Enforces conventions from `docs/CODING_CONVENTIONS.md`
  - Local variables: `camelCase`
  - Member variables: `camelCase_` (with trailing underscore)
  - Classes/Structs: `PascalCase`
  - Constants: `kPascalCase` or `SCREAMING_SNAKE_CASE`
- **Python**: Enforces PEP8 naming conventions
  - Variables/functions: `snake_case`
  - Classes: `PascalCase`
  - Constants: `SCREAMING_SNAKE_CASE`

### Style Checks
- Trailing whitespace detection
- Comment formatting (spacing after `//` or `#`)
- Indentation consistency (via `.editorconfig`)
- Line length recommendations

### Code Quality Checks
- **Commented-out code**: Detects and flags code that has been commented out instead of removed
- **Magic numbers**: Identifies hardcoded numeric literals that should be named constants
- **TODO comments**: Tracks technical debt and unfinished work
- **Code complexity**: Identifies overly complex functions (future enhancement)
- **JUCE Graphics allowlist**: Fails lint when new `juce::Graphics` usages appear in UI code (Skia migration guard)

### Structural Checks
- File organization patterns
- Include guard consistency (C++)
- Import organization (Python)

## Triggers

The LintingAgent runs automatically:
- On every push to main branches
- On all pull requests
- Can be run manually via GitHub Actions workflow dispatch
- Can be run locally during development

## Usage

### Running Locally

```bash
# Scan entire project
python services/ai/agents/LintingAgent/linting_agent.py

# Scan specific directory
python services/ai/agents/LintingAgent/linting_agent.py apps/desktop/Source

# Scan with glob patterns
python services/ai/agents/LintingAgent/linting_agent.py "**/*.cpp" "**/*.h"

# Fail on errors (useful for CI)
python services/ai/agents/LintingAgent/linting_agent.py --fail-on-error

# Exclude directories
python services/ai/agents/LintingAgent/linting_agent.py --exclude build external _deps
```

### CI/CD Integration

The agent is integrated into the GitHub Actions workflow at `.github/workflows/linting-agent.yml`. It runs automatically on:
- Push events to main, develop, and feature branches
- Pull request creation and updates
- Manual workflow dispatch

## Outputs

### Findings Report
Each lint finding includes:
- **Category**: naming, style, commented_code, magic_number, todo, structure, complexity
- **Severity**: error, warning, info
- **Location**: file path and line number
- **Description**: What the issue is
- **Suggestion**: How to fix it

### Report Format
```
================================================================================
LINT REPORT
================================================================================
Files scanned: 123
Total findings: 45
  Errors: 2
  Warnings: 28
  Info: 15
================================================================================

apps/desktop/Source/AudioEngine.cpp:
  ✗ Line 42: [naming] Variable 'samplerate' does not follow naming convention
    Suggestion: Use 'sampleRate' or 'sampleRate_' for member variables
  ⚠ Line 105: [magic_number] Magic number: 44100
    Suggestion: Consider using a named constant
  ℹ Line 230: [todo] TODO: Optimize buffer allocation
    Suggestion: Consider creating a tracked issue
```

## Configuration

### Excluding Files/Directories
By default, the following directories are excluded:
- `build/`
- `_deps/`
- `external/`
- `.git/`
- `node_modules/`
- `__pycache__/`
- `.vscode/`
- `*_artefacts/`

Additional exclusions can be specified via command-line arguments or by modifying the workflow file.

### Severity Levels
- **ERROR**: Must be fixed before merging
- **WARNING**: Should be addressed but not blocking
- **INFO**: Informational, good to know

## Extensibility

### Adding New Checks

To add new lint checks:

1. **Add pattern to `_initialize_patterns()`**:
```python
self.cpp_patterns['new_check'] = re.compile(r'pattern')
```

2. **Add check logic in `_scan_file()`**:
```python
if patterns['new_check'].search(line):
    findings.append(LintFinding(...))
```

3. **Add new category if needed**:
```python
class LintCategory(Enum):
    NEW_CATEGORY = "new_category"
```

### Integrating External Tools

The agent can be extended to integrate with:
- **clang-tidy**: For advanced C++ static analysis
- **cppcheck**: For C++ bug detection
- **pylint**: For comprehensive Python linting
- **flake8**: For Python style checking
- **clang-format**: For automatic code formatting

Example integration:
```python
def run_clang_tidy(self, files: List[Path]) -> List[LintFinding]:
    """Run clang-tidy on C++ files."""
    # Implementation here
    pass
```

### Auto-Fix Capabilities

Future enhancement: Add auto-fix mode that can automatically correct certain issues:
- Remove trailing whitespace
- Fix comment spacing
- Format code with clang-format/black

```bash
# Future feature
python services/ai/agents/LintingAgent/linting_agent.py --fix
```

## Acceptance Criteria

- [x] Detects naming convention violations
- [x] Identifies commented-out code
- [x] Finds magic numbers
- [x] Tracks TODO comments
- [x] Detects style violations (trailing whitespace, spacing)
- [x] Enforces `juce::Graphics` allowlist for Skia migration
- [x] Runs on push/PR via GitHub Actions
- [x] Outputs actionable findings in CI logs
- [x] Extensible for additional checks
- [x] Documented for contributors

## TODO: Next Steps

- [ ] Integrate clang-tidy for advanced C++ analysis
- [ ] Add pylint/flake8 for comprehensive Python checking
- [ ] Implement auto-fix mode for simple violations
- [ ] Add code complexity metrics (cyclomatic complexity)
- [ ] Generate HTML reports with syntax highlighting
- [ ] Add PR comment integration (post findings as review comments)
- [ ] Track metrics over time (lint debt dashboard)
- [ ] Add custom rule configuration file support
- [ ] Integrate with pre-commit hooks
- [ ] Add performance profiling to identify slow scans

## Related Documentation

- `docs/CODING_CONVENTIONS.md` - Project coding standards
- `.editorconfig` - Editor configuration for consistent formatting
- `.github/workflows/linting-agent.yml` - CI workflow configuration

## Examples

### Example 1: Magic Number Detection
```cpp
// Before (flagged)
float frequency = 440.0;  // ⚠ Magic number

// After (clean)
constexpr float kStandardPitchHz = 440.0;
float frequency = kStandardPitchHz;  // ✓
```

### Example 2: Naming Convention
```cpp
// Before (flagged)
class AudioProcessor {
  int samplerate;  // ✗ Member should have trailing _
};

// After (clean)
class AudioProcessor {
  int sampleRate_;  // ✓
};
```

### Example 3: Commented-Out Code
```cpp
// Before (flagged)
void process() {
  // processAudio();  // ⚠ Commented-out code
  newProcess();
}

// After (clean)
void process() {
  newProcess();  // ✓
}
```

## Contributing

When adding new lint rules:
1. Follow the existing pattern structure
2. Add tests for the new rule
3. Update this README with examples
4. Ensure the rule is configurable if needed
5. Document the rationale for the rule
