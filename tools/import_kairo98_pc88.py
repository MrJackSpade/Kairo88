#!/usr/bin/env python3
"""One-time import of Kairo98's reviewed PC-88 research into Kairo88's decision files.

Usage: import_kairo98_pc88.py <Kairo98 checkout>

Kairo98 matched Neo Kobe PC-88 folders to LaunchBox games, wrote some descriptions, reviewed
display titles, adult content and artwork, and approved some artwork for bundling. This copies
those decisions: folder matches become media decisions for every scanned archive in the folder,
and the approved artwork files are copied with their checksums verified. Existing Kairo88
decisions are never overwritten.
"""
import hashlib
import json
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RESEARCH = ROOT / "catalog/research"


def read(path):
    return json.loads(path.read_text(encoding="utf-8"))


def write(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=1, sort_keys=True) + "\n", encoding="utf-8")


def main():
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    source = Path(sys.argv[1])
    matches = [entry for entry in read(source / "catalog/research/matches-v1.json")["entries"]
               if entry["platform"] == "pc88"]
    folders = {}
    for entry in matches:
        key = f"pc88:{entry['databaseId']}"
        for group in entry["sourceGroups"]:
            if not group.startswith("pc88:"):
                raise SystemExit(f"unexpected PC-88 source group {group}")
            folder = group.removeprefix("pc88:")
            if folders.setdefault(folder, key) != key:
                raise SystemExit(f"folder {folder} matched to two games")
    scan = read(ROOT / ".downloads/scan/neokobe-pc88.json")
    media = {"schemaVersion": 1, "archives": {}}
    if (RESEARCH / "media-matches-v1.json").is_file():
        media = read(RESEARCH / "media-matches-v1.json")
    placed = set()
    for archive in scan["archives"]:
        folder = archive["path"].split("/", 1)[0]
        if folder in folders and archive["status"] == "ready":
            media["archives"].setdefault(f"neokobe:{archive['path']}", folders[folder])
            placed.add(folder)
    missing = sorted(set(folders) - placed)

    keys = {f"pc88:{entry['databaseId']}" for entry in matches}
    reviewed = read(source / "catalog/research/descriptions-v1.json")["descriptions"]
    descriptions = {"schemaVersion": 1, "descriptions": {}}
    if (RESEARCH / "descriptions-v1.json").is_file():
        descriptions = read(RESEARCH / "descriptions-v1.json")
    for entry in matches:
        key = f"pc88:{entry['databaseId']}"
        text = reviewed.get(key, entry.get("description"))
        if text:
            descriptions["descriptions"].setdefault(key, text)

    overrides = {"schemaVersion": 1, "games": {}}
    if (RESEARCH / "overrides-v1.json").is_file():
        overrides = read(RESEARCH / "overrides-v1.json")
    display = read(source / "catalog/research/display-text-v1.json")["games"]
    for key, item in display.items():
        if key in keys:
            overrides["games"].setdefault(key, {}).update(item["fields"])
    adult = read(source / "catalog/research/adult-content-v1.json")["games"]
    policy = read(source / "catalog/core-review-v1.json")
    excluded = {key: value for key, value in policy["excluded"].items() if key in keys}
    for key in keys:
        if adult.get(key, {}).get("status") == "eroge" or key in excluded:
            overrides["games"].setdefault(key, {})["adult"] = True
    artwork = read(source / "catalog/research/artwork-index-v1.json")
    for key in keys:
        if artwork.get(key):
            overrides["games"].setdefault(key, {})["artwork"] = artwork[key]

    approved = {path: sha for path, sha in policy["approvedArtwork"].items() if path.startswith("art/catalog/pc88/")}
    for path, sha in approved.items():
        data = (source / "catalog/artwork" / path).read_bytes()
        if hashlib.sha256(data).hexdigest() != sha:
            raise SystemExit(f"approved artwork checksum differs: {path}")
        target = ROOT / "catalog/artwork" / path
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source / "catalog/artwork" / path, target)
    review = {"schemaVersion": 1, "review": policy.get("review"), "excluded": {}, "approvedArtwork": {}}
    if (ROOT / "catalog/core-review-v1.json").is_file():
        review = read(ROOT / "catalog/core-review-v1.json")
    for key, value in excluded.items():
        review["excluded"].setdefault(key, value)
    for key, entry in overrides["games"].items():
        if entry.get("adult"):
            review["excluded"].setdefault(key, {"title": next(e["title"] for e in matches if f"pc88:{e['databaseId']}" == key),
                                                "reason": "Kairo98 adult-content review"})
    review["approvedArtwork"].update(approved)

    write(RESEARCH / "media-matches-v1.json", media)
    write(RESEARCH / "descriptions-v1.json", descriptions)
    write(RESEARCH / "overrides-v1.json", overrides)
    write(ROOT / "catalog/core-review-v1.json", review)
    print(f"{len(matches)} games, {len(folders)} folders ({len(missing)} without scanned media: {missing[:5]}), "
          f"{len(descriptions['descriptions'])} descriptions, {len(overrides['games'])} overrides, "
          f"{len(review['excluded'])} excluded, {len(approved)} approved images")


if __name__ == "__main__":
    main()
