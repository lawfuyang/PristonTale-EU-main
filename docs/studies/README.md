# Studies

Updated on: 2026-03-15

This folder is for deeper investigations that are not yet stable guides or quick references.

Examples of material that belongs here:

- database schema studies by subsystem
- packet and network analysis
- anti-cheat investigation notes
- binary format mapping such as `.chr`
- source versus runtime comparison work

## Available studies

- [game-exe-buildability-and-offline-feasibility.md](game-exe-buildability-and-offline-feasibility.md): verifies whether the client can be compiled, root-causes the build failures, and assesses whether a single serverless `game.exe` is achievable
- [monster-hp-update-latency-analysis.md](monster-hp-update-latency-analysis.md): root-causes the delay between dealing damage and the monster HP bar updating (server-side update-rate throttling, not networking)

Practical rule:

- if the material is exploratory, experimental, or research-oriented, it belongs in `docs/studies/`
- once that work turns into a stable procedure, move or mirror the important parts into `docs/guides/` or `docs/reference/`
