# GitHub Configuration Directory

This directory contains configuration files for Zenith DAW's GitHub repository setup and automation.

## Directory Structure

```
.github/
├── config/
│   ├── settings.yml          # Main GitHub repository settings
│   └── README.md            # This file
├── ISSUE_TEMPLATE/          # Issue templates for bug reports, features, etc.
│   ├── bug_report.yml       # Bug report template
│   ├── feature_request.yml  # Feature request template
│   ├── documentation.yml    # Documentation issue template
│   └── community/          # Community-specific issue templates
│       ├── discussion.yml   # Community discussion template
│       ├── showcase.yml     # Showcase template
│       ├── help.yml         # Help request template
│       └── collaboration.yml # Collaboration template
├── PULL_REQUEST_TEMPLATE/  # Pull request templates
│   ├── pull_request_template.md  # Default PR template
│   └── community/          # Community-specific PR templates
│       ├── contributor.md  # Community contributor template
│       └── documentation.md # Documentation update template
└── workflows/              # GitHub Actions workflows
    └── [various workflow files]
```

## Configuration Files

### `settings.yml`
The main configuration file that defines:
- Repository settings and metadata
- Branch protection rules
- Team permissions
- Code owner assignments
- Issue and PR template configurations
- Automation settings
- Security configurations
- Community guidelines

### Issue Templates
Standard templates for reporting issues:
- **Bug Report**: Structured template for reporting bugs
- **Feature Request**: Template for suggesting new features
- **Documentation**: For documentation-related issues

### Community Issue Templates
Enhanced templates for community engagement:
- **Discussion**: For community discussions
- **Showcase**: For sharing projects and creations
- **Help**: For asking community for help
- **Collaboration**: For finding collaborators

### Pull Request Templates
Templates for different types of contributions:
- **Default**: Standard PR template for code changes
- **Community**: For community contributions
- **Documentation**: For documentation updates

## Getting Started

### Setting Up Repository Configuration

1. **Repository Settings**
   ```bash
   # Repository information
   gh repo edit zenith-daw --description "Zenith DAW - A modern, cross-platform Digital Audio Workstation"
   gh repo edit zenith-daw --homepage "https://zenithdaw.com"
   ```

2. **Teams and Collaborators**
   ```bash
   # Create team
   gh team create core-developers --description "Core development team" --permission admin

   # Add member to team
   gh team add-member core-developers --user micahcooley
   ```

3. **Branch Protection**
   ```bash
   # Apply branch protection
   gh branch protect main --required-status-checks SecurityAgent,TestingAgent
   gh branch protect main --required-pull-request-reviews
   ```

### Customizing Templates

1. **Issue Templates**
   - Edit YAML files in `ISSUE_TEMPLATE/`
   - Test templates using the GitHub interface
   - Update `settings.yml` with new template configurations

2. **Pull Request Templates**
   - Edit Markdown files in `PULL_REQUEST_TEMPLATE/`
   - Use YAML front matter for custom fields
   - Update `settings.yml` with template configurations

### Automation Setup

1. **Dependabot**
   - Configure automated dependency updates
   - Set update schedule and assignees
   - Monitor for security vulnerabilities

2. **Stale Issues**
   - Configure automatic issue closing
   - Set stale message and timeframe
   - Monitor issue activity

3. **Security Scanning**
   - Enable automated secret scanning
   - Configure vulnerability alerts
   - Set up codeql analysis

## Best Practices

### Repository Maintenance
- Regular review of repository settings
- Update team members as needed
- Monitor issue and PR templates
- Keep automation scripts current

### Community Management
- Monitor community discussions
- Moderate issues and pull requests
- Respond to community inquiries
- Update documentation as needed

### Security
- Regular security scans
- Monitor for vulnerabilities
- Keep dependencies updated
- Review third-party integrations

## Troubleshooting

### Common Issues

1. **Template Not Showing**
   - Verify template syntax in YAML
   - Check GitHub repository settings
   - Ensure proper file permissions

2. **Automation Not Working**
   - Check workflow triggers
   - Verify action permissions
   - Review logs for errors

3. **Branch Protection Issues**
   - Check required status checks
   - Verify team permissions
   - Review protection rules

### Getting Help

- GitHub Support: [GitHub Support](https://support.github.com/)
- GitHub Documentation: [GitHub Docs](https://docs.github.com/)
- Community Discussions: [Zenith DAW Discussions](https://github.com/your-repo/zenith-daw/discussions)

## Contributing

To contribute improvements to this configuration:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request
5. Review feedback and make adjustments

## License

This configuration is part of the Zenith DAW project and is licensed under the same terms as the main project.