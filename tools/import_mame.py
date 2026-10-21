#!/usr/bin/env python3
"""Copy the MAME sources one or more drivers need into a vendored snapshot.

The device set comes from MAME's own scripts/build/makedep.py (the logic behind
`make SOURCES=...`). Device options are resolved to files through the
scripts/src/*.lua file lists, then the set is closed over #include so headers
and their companion sources are present. Whole 3rdparty libraries are copied
when any selected file includes them, so their license files come along.

Usage: import_mame.py <mame-source-root> <destination> <driver.cpp>...
Driver paths are relative to the MAME root, e.g. src/mame/sharp/x68k.cpp.
"""

import os
import re
import shutil
import subprocess
import sys

OPTION_RE = re.compile(r'^(BUSES|CPUS|MACHINES|SOUNDS|VIDEOS|FORMATS)\["([^"]+)"\] = true$')
IF_RE = re.compile(r'^if (.*) then\s*$')
REF_RE = re.compile(r'(BUSES|CPUS|MACHINES|SOUNDS|VIDEOS|FORMATS)\["([^"]+)"\]')
FILE_RE = re.compile(r'MAME_DIR \.\. "([^"]+)"')
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*"([^"]+)"', re.M)

# The include directories MAME's build passes to device, emulator and driver code.
INCLUDE_DIRS = [
    'src/osd', 'src/emu', 'src/devices', 'src/mame', 'src/mame/shared', 'src/lib',
    'src/lib/util', 'src/lib/netlist', '3rdparty',
]

# Always-present parts of the emulator core.
BASE_TREES = ['src/emu', 'src/lib/util']
BASE_FILES = ['COPYING', 'README.md']
BASE_DIRS_FLAT = ['src/osd']
LEGAL_TREE = 'docs/legal'

# 3rdparty libraries the core links whether or not a source includes them directly.
BASE_3RDPARTY = ['expat', 'zlib', 'lzma', 'zstd', 'utf8proc', 'flac', 'softfloat3', 'rapidjson', 'asio']

SOURCE_EXTS = ('.cpp', '.c', '.ipp', '.hxx')


def run_makedep(root, drivers):
    out = subprocess.run(
        [sys.executable, '-I', 'scripts/build/makedep.py', '-r', '.', 'sourcesproject',
         '-t', 'kairo', '-l', 'src/mame/mame.lst', *drivers],
        cwd=root, check=True, capture_output=True, text=True).stdout
    options = set()
    for line in out.splitlines():
        m = OPTION_RE.match(line.strip())
        if m:
            options.add((m.group(1), m.group(2)))
    # The generated project lists include directories first; driver sources follow `files{`.
    files = set(FILE_RE.findall(out.split('files{', 1)[1]))
    return out, options, files


def lua_option_files(root, options):
    """Files from every `if <option refs> then ... end` block with an enabled option."""
    files = set()
    for name in ('bus', 'cpu', 'machine', 'sound', 'video', 'formats'):
        path = os.path.join(root, 'scripts', 'src', name + '.lua')
        with open(path, encoding='utf-8') as f:
            lines = f.read().splitlines()
        active = None
        depth = 0
        for line in lines:
            stripped = line.strip()
            if active is None:
                m = IF_RE.match(stripped)
                if m:
                    refs = set(REF_RE.findall(m.group(1)))
                    active = bool(refs & options)
                    depth = 1
                continue
            if IF_RE.match(stripped):
                depth += 1
            elif stripped == 'end':
                depth -= 1
                if depth == 0:
                    active = None
                    continue
            if active:
                files.update(FILE_RE.findall(line))
    return without_include_dirs(root, files)


def without_include_dirs(root, paths):
    """Lua blocks also name include directories; only files are copied by path."""
    return {p for p in paths if not os.path.isdir(os.path.join(root, p))}


def unconditional_formats(root):
    """formats.lua lists the shared floppy/format core outside any option block."""
    path = os.path.join(root, 'scripts', 'src', 'formats.lua')
    with open(path, encoding='utf-8') as f:
        text = f.read()
    head = text.split('\nif ', 1)[0]
    return without_include_dirs(root, set(FILE_RE.findall(head)))


def walk(root, rel):
    base = os.path.join(root, rel)
    if not os.path.isdir(base):
        raise SystemExit(f'directory missing from MAME tree: {rel}')
    for dirpath, _, names in os.walk(base):
        for n in names:
            yield os.path.relpath(os.path.join(dirpath, n), root).replace('\\', '/')


def resolve_include(root, from_file, inc):
    candidates = [os.path.join(os.path.dirname(from_file), inc)]
    candidates += [os.path.join(d, inc) for d in INCLUDE_DIRS]
    for c in candidates:
        c = os.path.normpath(c).replace('\\', '/')
        if os.path.isfile(os.path.join(root, c)):
            return c
    return None


def companions(root, header):
    stem, _ = os.path.splitext(header)
    for ext in SOURCE_EXTS:
        if os.path.isfile(os.path.join(root, stem + ext)):
            yield stem + ext


def generated_inputs(root, inc):
    """Sources for generated includes: layouts (.lh from .lay) and fonts (.fh)."""
    stem, ext = os.path.splitext(os.path.basename(inc))
    if ext == '.lh':
        for d in ('src/emu/layout', 'src/mame/layout'):
            p = f'{d}/{stem}.lay'
            if os.path.isfile(os.path.join(root, p)):
                yield p
    elif ext == '.fh':
        for dirpath, _, names in os.walk(os.path.join(root, 'src', 'emu')):
            for n in names:
                if os.path.splitext(n)[0] == stem:
                    yield os.path.relpath(os.path.join(dirpath, n), root).replace('\\', '/')


def close_over_includes(root, seed):
    selected = set(seed)
    libs = set(BASE_3RDPARTY)
    queue = list(selected)
    while queue:
        f = queue.pop()
        if not f.endswith(('.cpp', '.c', '.h', '.hpp', '.hxx', '.ipp', '.inc')):
            continue
        try:
            with open(os.path.join(root, f), encoding='utf-8', errors='replace') as fh:
                text = fh.read()
        except FileNotFoundError:
            raise SystemExit(f'listed source missing from MAME tree: {f}')
        for inc in INCLUDE_RE.findall(text):
            found = resolve_include(root, f, inc)
            new = []
            if found is None:
                new.extend(generated_inputs(root, inc))
            elif found.startswith('3rdparty/'):
                libs.add(found.split('/')[1])
                continue
            else:
                new.append(found)
                if found.startswith(('src/devices/', 'src/mame/', 'src/lib/formats/')):
                    new.extend(companions(root, found))
            for n in new:
                if n not in selected:
                    selected.add(n)
                    queue.append(n)
    return selected, libs


def main():
    if len(sys.argv) < 4:
        raise SystemExit(__doc__)
    root, dest, drivers = sys.argv[1], sys.argv[2], sys.argv[3:]
    project, options, driver_files = run_makedep(root, drivers)

    seed = set(driver_files) | lua_option_files(root, options) | unconditional_formats(root)
    for t in BASE_TREES:
        seed.update(walk(root, t))
    for d in BASE_DIRS_FLAT:
        seed.update(f'{d}/{n}' for n in os.listdir(os.path.join(root, d))
                    if os.path.isfile(os.path.join(root, d, n)))
    selected, libs = close_over_includes(root, seed)

    selected.update(BASE_FILES)
    selected.update(walk(root, LEGAL_TREE))
    for lib in sorted(libs):
        selected.update(walk(root, f'3rdparty/{lib}'))

    if os.path.exists(dest):
        raise SystemExit(f'destination exists: {dest}')
    for rel in sorted(selected):
        src = os.path.join(root, rel)
        if not os.path.isfile(src):
            raise SystemExit(f'selected file missing from MAME tree: {rel}')
        out = os.path.join(dest, rel)
        os.makedirs(os.path.dirname(out), exist_ok=True)
        shutil.copy2(src, out)

    with open(os.path.join(dest, 'KAIRO_IMPORT.txt'), 'w', encoding='utf-8', newline='\n') as f:
        f.write('Generated by tools/import_mame.py\n')
        f.write('Drivers: ' + ' '.join(drivers) + '\n')
        f.write('3rdparty libraries: ' + ' '.join(sorted(libs)) + '\n')
        f.write(f'Files: {len(selected)}\n\n')
        f.write('makedep.py sourcesproject output:\n\n')
        f.write(project)
    print(f'{len(selected)} files, 3rdparty: {" ".join(sorted(libs))}')


if __name__ == '__main__':
    main()
