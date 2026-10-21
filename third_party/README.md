# Third-party source snapshots

These directories contain copied, pinned source snapshots. They are not Git submodules or forks, and this repository has no upstream remote.

- `mame/`: the MAME 0.289 sources the PC-8801 driver needs, chosen by `tools/import_mame.py`, with MAME's license texts and each included 3rdparty library's license preserved.
- `quasi88/`: QUASI88 0.7.4 without its MAME/XMAME and fmgen sound output section.
- `ymfm/`: ymfm at commit `81aec25ccbb98f4873a255f7551ac4dadac59b4a`, with its BSD-3-Clause license preserved.

See [source import](../docs/source-import.md) for URLs, hashes, dates, and what was included or removed. Do not replace these snapshots through a Git remote or automated sync.
