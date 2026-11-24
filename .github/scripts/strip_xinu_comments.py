#!/usr/bin/env python3
"""Remove non-legal comments from XINU text source and build files."""
from pathlib import Path
import io
import re
import tokenize

ROOT = Path("XINU")
C_EXT = {".c", ".h", ".cc", ".hh", ".cpp", ".hpp", ".cxx", ".hxx",
         ".m", ".mm", ".inc", ".def", ".defs", ".iig", ".d", ".y", ".l", ".pbxproj", ".modulemap", ".cocci", ".ipp", ".tcc", ".java", ".js",
         ".jsx", ".ts", ".tsx", ".swift", ".rs", ".go", ".cs", ".kt"}
HASH_EXT = {".sh", ".bash", ".zsh", ".py", ".pl", ".pm", ".rb", ".r",
            ".mk", ".mak", ".make", ".yaml", ".yml", ".toml", ".ini",
            ".cfg", ".conf", ".properties"}
XML_EXT = {".xml", ".plist", ".html", ".htm", ".xhtml", ".svg"}
ASM_EXT = {".s", ".asm"}
LEGAL = re.compile(r"(SPDX-License-Identifier|Copyright|著作権|Apple Public Source License|"
                   r"APPLE PUBLIC SOURCE LICENSE|All rights reserved|Redistribution and use|"
                   r"Licensed under|This file is part of)", re.I)

def legal(s):
    return bool(LEGAL.search(s))

def blank(s):
    return "".join("\n" if c == "\n" else "\r" if c == "\r" else " " for c in s)

def c_strip(src, asm=False):
    out, i, n = [], 0, len(src)
    quote = None
    raw_end = None
    while i < n:
        if raw_end:
            if src.startswith(raw_end, i):
                out.append(raw_end); i += len(raw_end); raw_end = None
            else:
                out.append(src[i]); i += 1
            continue
        if quote:
            c = src[i]; out.append(c); i += 1
            if c == "\\\\" and i < n:
                out.append(src[i]); i += 1
            elif c == quote:
                quote = None
            continue
        if src[i] in ('"', "'"):
            quote = src[i]; out.append(src[i]); i += 1; continue
        if src.startswith("//", i):
            j = src.find("\n", i)
            if j < 0: j = n
            chunk = src[i:j]
            out.append(chunk if legal(chunk) else blank(chunk))
            i = j; continue
        if src.startswith("/*", i):
            j = src.find("*/", i + 2)
            j = n if j < 0 else j + 2
            chunk = src[i:j]
            out.append(chunk if legal(chunk) else blank(chunk))
            i = j; continue
        out.append(src[i]); i += 1
    return "".join(out)

def python_strip(src):
    lines = src.splitlines(keepends=True)
    offsets, total = [], 0
    for line in lines:
        offsets.append(total); total += len(line)
    chars = list(src)
    try:
        toks = tokenize.generate_tokens(io.StringIO(src).readline)
        for tok in toks:
            if tok.type != tokenize.COMMENT:
                continue
            if legal(tok.string) or tok.start[0] == 1 and tok.string.startswith("#!"):
                continue
            a = offsets[tok.start[0]-1] + tok.start[1]
            b = offsets[tok.end[0]-1] + tok.end[1]
            for k in range(a, b):
                if chars[k] not in "\r\n": chars[k] = " "
    except (tokenize.TokenError, IndentationError):
        return src
    return "".join(chars)

def line_comment_strip(src, marker="#", preserve_directives=False):
    out = []
    for line in src.splitlines(keepends=True):
        body = line.rstrip("\r\n")
        ending = line[len(body):]
        stripped = body.lstrip()
        if stripped.startswith(marker):
            if legal(stripped) or (preserve_directives and re.match(r"#\s*(include|define|if|ifdef|ifndef|elif|else|endif|pragma|error|warning|line|undef)", stripped)):
                out.append(line)
            else:
                out.append(ending if ending else "")
        else:
            out.append(line)
    return "".join(out)

def xml_strip(src):
    return re.sub(r"<!--.*?-->", lambda m: m.group(0) if legal(m.group(0)) else blank(m.group(0)),
                  src, flags=re.S)

def main():
    changed = 0
    scanned = 0
    for p in ROOT.rglob("*"):
        if not p.is_file() or p.is_symlink():
            continue
        ext = p.suffix.lower()
        if ext not in C_EXT | HASH_EXT | XML_EXT | ASM_EXT:
            continue
        try:
            raw = p.read_bytes()
            text = raw.decode("utf-8")
        except (UnicodeDecodeError, OSError):
            continue
        scanned += 1
        if ext in C_EXT:
            new = c_strip(text)
        elif ext in ASM_EXT:
            new = c_strip(text, asm=(ext == ".asm"))
        elif ext == ".py":
            new = python_strip(text)
        elif ext in XML_EXT:
            new = xml_strip(text)
        else:
            new = line_comment_strip(text, "#", preserve_directives=ext == ".S")
        if new != text:
            p.write_bytes(new.encode("utf-8"))
            changed += 1
    print(f"Scanned {scanned} text source/build files; changed {changed}.")
    print("Preserved copyright, SPDX, and license notices.")

if __name__ == "__main__":
    main()
