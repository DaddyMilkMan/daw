# SecurityAgent

## Purpose

Performs automated security analysis, vulnerability scanning, and security best practice enforcement for the DAW codebase. Identifies potential security issues before they reach production.

## Triggers

- New code committed with security-sensitive operations
- Dependency updates requiring security review
- Authentication or authorization code changes
- Network communication code modifications
- File I/O or data serialization changes

## Outputs

- Security vulnerability reports
- Dependency vulnerability scanning results
- Security best practice recommendations
- Authentication/authorization analysis
- Input validation and sanitization checks

## Acceptance Criteria

- [ ] Agent can scan for common vulnerability patterns
- [ ] Agent checks dependencies for known CVEs
- [ ] Agent validates input sanitization practices
- [ ] Agent reviews authentication/authorization code
- [ ] Agent provides actionable security recommendations

## TODO Checklist

- [ ] Implement vulnerability pattern detection
- [ ] Add dependency vulnerability scanning (CVE database)
- [ ] Create input validation checker
- [ ] Implement authentication/authorization code analysis
- [ ] Add secrets detection in code and configs
- [ ] Write tests for vulnerability detection
- [ ] Add integration with security scanning tools (Snyk, OWASP)
- [ ] Document security policies and guidelines
- [ ] Integrate with CI pipeline for automated scanning
- [ ] Add comprehensive security audit logging
