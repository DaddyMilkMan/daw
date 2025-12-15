# Safe Cleanup Protocol
1. **Verify Before Overwrite**: ALWAYS check file size and line count before replacing a file. If the new content is significantly smaller (>20% reduction), STOP and justify why logic is being removed.
2. **Preserve Intelligence**: Source/ai and Source/engine contain critical, often complex logic. Never delete " messy\ folders or files just to \purify\ the project structure.
3. **Refactor, Don't Delete**: If code is messy, refactor it in place. Do not replace working complex code with clean stubs or simplified placeholders.
4. **No Blind Deletions**: Do not delete files or directories solely to clean up the tree. Every deletion requires a specific implementation reason.
