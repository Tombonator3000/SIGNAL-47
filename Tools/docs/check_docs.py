#!/usr/bin/env python3
"""Check local links in repository Markdown without fetching external URLs.

Record existing issues before a migration with --write-baseline FILE. Afterwards,
--baseline FILE reports only new issues; --map-json FILE maps moved paths back to
old paths so harmless renames and line-number changes do not hide new failures.
"""
import argparse
from collections import Counter
import html
import json
import os
from pathlib import Path
import re
import subprocess
from urllib.parse import unquote, urlsplit


FENCE = re.compile(r'^\s{0,3}(`{3,}|~{3,})')
INLINE = re.compile(r'!?\[[^\n\]]*\]\(')
HTML_LINK = re.compile(r'\b(?:href|src)\s*=\s*([\"\'])(.*?)\1', re.I)
REFERENCE = re.compile(r'^\s{0,3}\[[^\]]+\]:\s*(?:<([^>]+)>|(\S+))')
HOST_PATH = re.compile(r'^(?:/(?:home|tmp|usr|mnt|Users|opt|var)/|[A-Za-z]:[\\/])')


def prose_lines(text):
    fence = None
    for number, line in enumerate(text.splitlines(), 1):
        match = FENCE.match(line)
        if match:
            token = match.group(1)
            if fence is None:
                fence = token[0], len(token)
            elif token[0] == fence[0] and len(token) >= fence[1]:
                fence = None
            continue
        if fence is None:
            yield number, line


def destination(value):
    value = value.strip()
    if value.startswith('<'):
        return value[1:value.find('>')] if '>' in value else value[1:]
    # A whitespace-separated title is not part of an unbracketed destination.
    return re.split(r'\s+[\"\']', value, maxsplit=1)[0].strip()


def links(text):
    for number, raw_line in prose_lines(text):
        line = re.sub(r'(`+).*?\1', '', raw_line)
        match = REFERENCE.match(line)
        if match:
            yield number, match.group(1) or match.group(2)
        for match in HTML_LINK.finditer(line):
            yield number, html.unescape(match.group(2))
        for match in INLINE.finditer(line):
            start, depth, escaped = match.end(), 1, False
            for index in range(start, len(line)):
                char = line[index]
                if escaped:
                    escaped = False
                elif char == '\\':
                    escaped = True
                elif char == '(':
                    depth += 1
                elif char == ')':
                    depth -= 1
                    if not depth:
                        yield number, destination(line[start:index])
                        break


def anchors(text):
    found, counts = set(), Counter()
    for _, line in prose_lines(text):
        for match in re.finditer(r'\b(?:id|name)\s*=\s*([\"\'])(.*?)\1', line, re.I):
            found.add(html.unescape(match.group(2)))
        heading = re.match(r'^\s{0,3}#{1,6}\s+(.+?)\s*#*\s*$', line)
        if not heading:
            continue
        title = re.sub(r'<[^>]*>', '', heading.group(1))
        title = re.sub(r'!?\[([^\]]*)\]\([^)]*\)', r'\1', title)
        slug = re.sub(r'[^\w\- ]', '', html.unescape(title).lower()).replace(' ', '-')
        suffix = counts[slug]
        counts[slug] += 1
        found.add(slug + ('-' + str(suffix) if suffix else ''))
    return found


def read_map(path):
    if not path:
        return []
    data = json.loads(path.read_text())
    if isinstance(data, dict):
        data = next((data[k] for k in ('documents', 'moves', 'records') if k in data), [])
    pairs = []
    for row in data:
        old, new = row.get('old_path', row.get('old')), row.get('new_path', row.get('new'))
        if old and new and old != new:
            pairs.append((str(new).strip('/'), str(old).strip('/')))
    return sorted(pairs, key=lambda item: len(item[0]), reverse=True)


def original(path, mapping):
    for new, old in mapping:
        if path == new or path.startswith(new + '/'):
            return old + path[len(new):]
    return path


def signature(issue, mapping):
    return (issue['kind'], original(issue['source'], mapping),
            original(issue['target'], mapping), issue.get('fragment', ''))


def scan(root):
    result = subprocess.run(['git', 'ls-files', '--cached', '--others', '--exclude-standard', '-z'],
                            cwd=root, capture_output=True, text=True, check=True)
    paths = sorted({p for p in result.stdout.split('\0') if p.lower().endswith(('.md', '.markdown'))
                    and (root / p).is_file()})
    issues, anchor_cache, checked = [], {}, 0
    for source in paths:
        for line, url in links((root / source).read_text(encoding='utf-8', errors='replace')):
            if not url or url.startswith(('#', '//')):
                continue
            host = bool(HOST_PATH.match(url)) or url.lower().startswith('file:')
            if host:
                if source.startswith('Docs/Active/') or source in ('README.md', 'Docs/README.md'):
                    issues.append(dict(kind='host-local', source=source, line=line, url=url,
                                       target=unquote(url), fragment=''))
                continue
            try:
                split = urlsplit(url)
            except ValueError:
                issues.append(dict(kind='invalid-link', source=source, line=line, url=url,
                                   target=url, fragment=''))
                continue
            if split.scheme or not split.path:
                continue
            value = unquote(split.path).replace('\\ ', ' ')
            target = os.path.normpath(value.lstrip('/') if value.startswith('/') else
                                      str(Path(source).parent / value)).replace(os.sep, '/')
            fragment, kind = unquote(split.fragment), None
            checked += 1
            if target == '..' or target.startswith('../'):
                kind = 'escaped-root'
            elif not (root / target).exists():
                kind = 'missing-target'
            elif fragment and (root / target).is_file() and target.lower().endswith(('.md', '.markdown')):
                if target not in anchor_cache:
                    anchor_cache[target] = anchors((root / target).read_text(encoding='utf-8', errors='replace'))
                if fragment not in anchor_cache[target]:
                    kind = 'missing-fragment'
            if kind:
                issues.append(dict(kind=kind, source=source, line=line, url=url,
                                   target=target, fragment=fragment))
    return len(paths), checked, issues


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--write-baseline', type=Path)
    parser.add_argument('--baseline', type=Path)
    parser.add_argument('--map-json', type=Path)
    parser.add_argument('--json', type=Path, help='Write the full current report')
    parser.add_argument('--limit', type=int, default=30, help='Maximum issues printed; 0 prints all')
    args = parser.parse_args()
    root, mapping = args.root.resolve(), read_map(args.map_json)
    files, checked, issues = scan(root)
    old = set()
    if args.baseline:
        data = json.loads(args.baseline.read_text())
        old = {signature(row, mapping) for row in data['issues']}
    new = [row for row in issues if signature(row, mapping) not in old]
    report = dict(version=1, markdown_files=files, local_links_checked=checked,
                  total_issues=len(issues), baseline_issues=len(old), new_issues=len(new),
                  issues=issues, new_issue_details=new)
    if args.write_baseline:
        args.write_baseline.parent.mkdir(parents=True, exist_ok=True)
        args.write_baseline.write_text(json.dumps(report, indent=2, ensure_ascii=False) + '\n')
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(report, indent=2, ensure_ascii=False) + '\n')
    print(f'Markdown: {files}; local links: {checked}; issues: {len(issues)}; new: {len(new)}')
    for row in new[:args.limit or None]:
        print(f"{row['source']}:{row['line']}: {row['kind']}: {row['url']}")
    if args.limit and len(new) > args.limit:
        print(f'... {len(new) - args.limit} more; use --json FILE or --limit 0 for all')
    return 0 if args.write_baseline or not new else 1


if __name__ == '__main__':
    raise SystemExit(main())
