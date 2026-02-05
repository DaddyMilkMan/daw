# DocumentationAgent

## Purpose

The DocumentationAgent is an automated system for validating and monitoring project documentation health. It ensures that critical documentation files are present, up-to-date, and synchronized with the codebase.

## Responsibilities

- **Required Documentation Validation**: Checks for presence of essential documentation files (README.md, architecture docs, ADRs, etc.)
- **Documentation Freshness**: Monitors when documentation was last updated and flags potentially stale files
- **Agent Documentation**: Ensures each agent has proper README documentation
- **Public API Documentation**: Validates that public headers have appropriate Doxygen-style comments
- **Tech Brief/ADR Tracking**: Monitors architectural decision records and technical design documentation

## Triggers

- **CI Pipeline**: Runs automatically on every push and pull request via GitHub Actions
- **Manual Execution**: Can be run locally for immediate feedback:
  ```bash
  python services/ai/agents/DocumentationAgent/documentation_agent.py
  ```

## Inputs

- Project repository structure
- Documentation files in `docs/` directory
- Agent directories in `services/ai/agents/`
- Public header files in `include/`
- Configuration via command-line arguments:
  - `--project-root`: Specify project root directory
  - `--output-json`: Save results as JSON report
  - `--max-age-days`: Maximum age before flagging as outdated (default: 180 days)
  - `--fail-on-critical`: Exit with error code if critical issues found

## Outputs

### CI Logs
- Summary of documentation validation results
- Count of issues by severity (critical, high, medium, low)
- List of missing or outdated documentation files
- Actionable recommendations for each issue

### Optional JSON Report
- Structured report of all findings
- Can be used for further analysis or integration with other tools

### Exit Codes
- `0`: Success (no critical issues, or not failing on critical)
- `1`: Critical documentation issues found (when `--fail-on-critical` is used)

## Acceptance Criteria

- ✅ Validates presence of all required documentation files
- ✅ Checks documentation freshness (last modified date)
- ✅ Ensures each agent has a README.md
- ✅ Validates tech-briefs/ADR documentation
- ✅ Provides clear, actionable feedback in CI logs
- ✅ Runs on every push and pull request via GitHub Actions
- ✅ Can be run locally for immediate feedback
- ✅ Easily expandable to add new documentation checks

## TODO

Future enhancements:
- [ ] Parse and validate specific documentation formats (check for required sections)
- [ ] Detect orphaned documentation (docs for code that no longer exists)
- [ ] Validate internal documentation links
- [ ] Check for broken external links
- [ ] Integrate with PR review comments (post findings as review comments)
- [ ] Add spell checking for documentation
- [ ] Validate code examples in documentation compile/run correctly
- [ ] Track documentation coverage metrics over time
- [ ] Generate documentation diff reports for PRs

## Usage Examples

### Run all checks:
```bash
python services/ai/agents/DocumentationAgent/documentation_agent.py
```

### Run with custom project root:
```bash
python services/ai/agents/DocumentationAgent/documentation_agent.py --project-root /path/to/project
```

### Save results as JSON:
```bash
python services/ai/agents/DocumentationAgent/documentation_agent.py --output-json doc_report.json
```

### Fail CI on critical issues:
```bash
python services/ai/agents/DocumentationAgent/documentation_agent.py --fail-on-critical
```

## Integration with CI

The agent is integrated into the GitHub Actions workflow at `.github/workflows/documentation-agent.yml`. The workflow:

1. Triggers on push and pull requests
2. Sets up Python 3.x environment
3. Runs the documentation agent
4. Outputs results to CI logs
5. (Optional) Fails the build on critical issues

## Expansion Guide

To add new documentation checks:

1. Add a new check method to the `DocumentationAgent` class in `documentation_agent.py`
2. Call the new method from `run_all_checks()`
3. Use the existing `DocIssue` and severity levels for consistent reporting
4. Update this README with the new check description

Example:
```python
def check_my_new_requirement(self) -> None:
    """Check for my custom documentation requirement."""
    print("Checking my custom requirement...")
    
    # Your validation logic here
    if something_is_wrong:
        self.report.issues.append(DocIssue(
            severity=Severity.HIGH,
            status=DocStatus.MISSING,
            file_path=Path("path/to/file"),
            description="Description of the issue",
            recommendation="How to fix it"
        ))
```

## See Also

- [Main Agents README](../README.md)
- [Contributing Guidelines](../../CONTRIBUTING.md)
- [Documentation Index](../../docs/DOCUMENTATION_INDEX.md)
