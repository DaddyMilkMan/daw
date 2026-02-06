#!/usr/bin/env python3
"""
SAFE INCLUDE UPDATER
====================

Updates #include statements after file renames WITHOUT using dangerous regex.
Uses token-aware parsing to avoid modifying comments and strings.

Usage:
    python3 scripts/update-includes-safe.py --map-file renames.json --dry-run
    python3 scripts/update-includes-safe.py --map-file renames.json --execute
"""

import argparse
import json
import re
import sys
from dataclasses import dataclass
from enum import Enum, auto
from pathlib import Path
from typing import Dict, List, Optional, Tuple


class TokenType(Enum):
    """Types of tokens in C++ source."""
    INCLUDE_DIRECTIVE = auto()  # #include
    STRING_LITERAL = auto()     # "..."
    ANGLE_LITERAL = auto()      # <...>
    COMMENT_SINGLE = auto()     // ...
    COMMENT_MULTI = auto()      /* ... */
    CODE = auto()               # Everything else


@dataclass
class Token:
    """A token from the source code."""
    type: TokenType
    value: str
    line: int


def tokenize(content: str) -> List[Token]:
    """
    Tokenize C++ source code, distinguishing includes from strings/comments.
    This is safer than regex because it understands C++ syntax.
    """
    tokens = []
    lines = content.split('\n')
    
    in_multi_line_comment = False
    
    for line_num, line in enumerate(lines, 1):
        original_line = line
        pos = 0
        
        while pos < len(line):
            # Check for multi-line comment end
            if in_multi_line_comment:
                end = line.find('*/', pos)
                if end != -1:
                    tokens.append(Token(
                        TokenType.COMMENT_MULTI,
                        line[pos:end+2],
                        line_num
                    ))
                    pos = end + 2
                    in_multi_line_comment = False
                else:
                    tokens.append(Token(
                        TokenType.COMMENT_MULTI,
                        line[pos:],
                        line_num
                    ))
                    break
                continue
            
            remaining = line[pos:]
            
            # Check for #include
            if remaining.lstrip().startswith('#include'):
                # Find the end of the include line
                tokens.append(Token(TokenType.INCLUDE_DIRECTIVE, '#include', line_num))
                pos += remaining.index('#include') + 8
                continue
            
            # Check for single-line comment
            if '//' in remaining:
                comment_start = remaining.index('//')
                if pos == 0 or remaining[:comment_start].strip() == '':
                    # Comment at start of line or after whitespace
                    if comment_start > 0:
                        tokens.append(Token(TokenType.CODE, remaining[:comment_start], line_num))
                    tokens.append(Token(TokenType.COMMENT_SINGLE, remaining[comment_start:], line_num))
                    break
            
            # Check for multi-line comment start
            if '/*' in remaining:
                comment_start = remaining.index('/*')
                if comment_start > 0:
                    tokens.append(Token(TokenType.CODE, remaining[:comment_start], line_num))
                
                # Check if it ends on this line
                end = remaining.find('*/', comment_start + 2)
                if end != -1:
                    tokens.append(Token(
                        TokenType.COMMENT_MULTI,
                        remaining[comment_start:end+2],
                        line_num
                    ))
                    pos += end + 2
                else:
                    tokens.append(Token(
                        TokenType.COMMENT_MULTI,
                        remaining[comment_start:],
                        line_num
                    ))
                    in_multi_line_comment = True
                    break
                continue
            
            # Check for string literals
            if '"' in remaining:
                quote_pos = remaining.index('"')
                # Make sure it's not an escaped quote
                if quote_pos == 0 or remaining[quote_pos-1] != '\\':
                    # Code before string
                    if quote_pos > 0:
                        tokens.append(Token(TokenType.CODE, remaining[:quote_pos], line_num))
                    
                    # Find end of string
                    end = quote_pos + 1
                    while end < len(remaining):
                        if remaining[end] == '"' and remaining[end-1] != '\\':
                            break
                        end += 1
                    
                    tokens.append(Token(TokenType.STRING_LITERAL, remaining[quote_pos:end+1], line_num))
                    pos += end + 1
                    continue
            
            # Check for angle brackets (after #include)
            if '<' in remaining and tokens and tokens[-1].type == TokenType.INCLUDE_DIRECTIVE:
                angle_pos = remaining.index('<')
                end = remaining.find('>', angle_pos)
                if end != -1:
                    if angle_pos > 0:
                        tokens.append(Token(TokenType.CODE, remaining[:angle_pos], line_num))
                    tokens.append(Token(TokenType.ANGLE_LITERAL, remaining[angle_pos:end+1], line_num))
                    pos += end + 1
                    continue
            
            # Regular code
            tokens.append(Token(TokenType.CODE, remaining, line_num))
            break
    
    return tokens


def find_include_path(token: Token) -> Optional[str]:
    """Extract include path from a string or angle token."""
    if token.type == TokenType.STRING_LITERAL:
        # "path/to/file.h" -> path/to/file.h
        match = re.match(r'"([^"]+)"', token.value)
        if match:
            return match.group(1)
    elif token.type == TokenType.ANGLE_LITERAL:
        # <path/to/file.h> -> path/to/file.h
        match = re.match(r'<([^>]+)>', token.value)
        if match:
            return match.group(1)
    return None


def should_update_include(include_path: str, rename_map: Dict[str, str]) -> Optional[str]:
    """
    Check if an include path should be updated.
    Returns new path if it should be updated, None otherwise.
    """
    # Direct match
    if include_path in rename_map:
        return rename_map[include_path]
    
    # Check if it ends with a renamed file
    for old_path, new_path in rename_map.items():
        if include_path.endswith(Path(old_path).name):
            # It's including a renamed file
            # Try to construct new path based on context
            old_basename = Path(old_path).name
            new_basename = Path(new_path).name
            
            # If it's just the basename, return new basename
            if include_path == old_basename:
                return new_basename
            
            # If it has a path, try to update just the filename
            if '/' in include_path:
                # Check if the old path is a suffix
                if include_path.endswith(old_path):
                    return include_path.replace(old_path, new_path)
    
    return None


def process_file(filepath: Path, rename_map: Dict[str, str], dry_run: bool) -> Tuple[bool, str]:
    """
    Process a single file, updating includes as needed.
    Returns (was_modified, new_content or error_message).
    """
    try:
        content = filepath.read_text()
    except Exception as e:
        return False, f"Error reading: {e}"
    
    tokens = tokenize(content)
    modified = False
    changes = []
    
    # Process tokens and build new content
    new_content_parts = []
    i = 0
    while i < len(tokens):
        token = tokens[i]
        
        # Check if this is an include path (string literal following #include)
        if token.type == TokenType.STRING_LITERAL and i > 0:
            prev_token = tokens[i-1]
            if prev_token.type == TokenType.INCLUDE_DIRECTIVE:
                include_path = find_include_path(token)
                if include_path:
                    new_path = should_update_include(include_path, rename_map)
                    if new_path:
                        # Reconstruct the include with proper quotes
                        new_token_value = f'"{new_path}"'
                        new_content_parts.append(new_token_value)
                        changes.append(f"Line {token.line}: {include_path} -> {new_path}")
                        modified = True
                        i += 1
                        continue
        
        # Check angle includes
        if token.type == TokenType.ANGLE_LITERAL and i > 0:
            prev_token = tokens[i-1]
            if prev_token.type == TokenType.INCLUDE_DIRECTIVE:
                include_path = find_include_path(token)
                if include_path:
                    new_path = should_update_include(include_path, rename_map)
                    if new_path:
                        new_token_value = f'<{new_path}>'
                        new_content_parts.append(new_token_value)
                        changes.append(f"Line {token.line}: {include_path} -> {new_path}")
                        modified = True
                        i += 1
                        continue
        
        # Keep token as-is
        new_content_parts.append(token.value)
        i += 1
    
    new_content = ''.join(new_content_parts)
    
    if modified:
        if dry_run:
            return True, f"Would update {filepath}:\n" + "\n".join(f"  {c}" for c in changes)
        else:
            filepath.write_text(new_content)
            return True, f"Updated {filepath}:\n" + "\n".join(f"  {c}" for c in changes)
    
    return False, "No changes needed"


def main():
    parser = argparse.ArgumentParser(
        description="Safely update #include statements after renames"
    )
    parser.add_argument(
        "--map-file",
        required=True,
        help="JSON file mapping old paths to new paths"
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Preview changes without modifying files"
    )
    parser.add_argument(
        "--files",
        nargs="*",
        help="Specific files to process (default: all .cpp/.h)"
    )
    
    args = parser.parse_args()
    
    # Load rename map
    map_file = Path(args.map_file)
    if not map_file.exists():
        print(f"❌ Map file not found: {map_file}")
        return 1
    
    rename_map = json.loads(map_file.read_text())
    print(f"Loaded {len(rename_map)} renames from {map_file}")
    
    # Find files to process
    if args.files:
        files = [Path(f) for f in args.files]
    else:
        files = []
        for pattern in ["modules/**/*.cpp", "modules/**/*.h", "apps/**/*.cpp", "apps/**/*.h"]:
            files.extend(Path(".").glob(pattern))
        files = [f for f in files if "external" not in str(f)]
    
    print(f"Processing {len(files)} files...")
    
    # Process files
    modified_count = 0
    for filepath in files:
        was_modified, message = process_file(filepath, rename_map, args.dry_run)
        if was_modified:
            print(f"\n{message}")
            modified_count += 1
    
    print(f"\n{'='*60}")
    print(f"Files {'would be ' if args.dry_run else ''}modified: {modified_count}")
    
    if args.dry_run and modified_count > 0:
        print("\nRun without --dry-run to apply changes.")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
