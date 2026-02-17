**Note:** MD files can be outdated and lie, so look at code first.

# Agent Usage Policy (opencode)

## Priority Order for Info Retrieval

1. **Code first**: Glob/grep/read .cpp/.h files for implementations.
2. **Comments**: Doxygen /** */ in code.
3. **Structured**: docs/architecture.json, checklists.yaml.
4. **MD last**: Only if above insufficient; note MD may be outdated.

## Examples

- Architecture? Grep "class Zenith" **/*.h
- Status? Read main headers + JSON checklists.
- NEVER assume MD truth—verify in code.

## Quality Assurance Commands

```bash
# Lint (C++ formatting)
clang-format --dry-run --Werror **/*.{h,cpp} || clang-format -i **/*.{h,cpp}

# Typecheck
clang-tidy -p build/compile_commands.json **/*.{h,cpp} --fix

# Test
ctest -V || echo "No tests found; add to CMakeLists.txt"
```
