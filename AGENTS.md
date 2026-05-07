# AGENTS.md

## Project Overview

RedLight is a Windows C++/Win32 project intended to provide a strict red-only display filter. The core behavior is not a translucent red tint. It should transform display output so that black remains black, white becomes pure red, and green/blue output is eliminated.

Do not replace the core behavior with a translucent overlay unless explicitly instructed.

## Development Guidance

- Keep changes small, focused, and reviewable.
- Prefer native C++/Win32 unless explicitly instructed otherwise.
- Update `CHANGELOG.md` under `[Unreleased]` for user-visible changes, display backend changes, release-related changes, and behavior changes.
- Add a changelog entry for internal refactors when they affect safety, restore behavior, backend selection, diagnostics, or user-visible reliability.
- Preserve the strict red-only output requirement:
  - Black remains black.
  - White becomes pure red.
  - Green and blue output should be eliminated.
- Treat display-affecting code as safety-sensitive.
- Check Win32 API return values.
- Avoid silently changing internal application state after failed Win32 API calls.
- Use diagnostics and logging for display backend changes.
- Do not perform broad rewrites unless the task explicitly asks for one.

## Build Instructions

Use the following commands to build the project:

```sh
cmake -S . -B build && cmake --build build --config Release
```

## Display-Affecting Changes

Any change that affects display output, display state, color transformation, gamma ramps, monitor handling, recovery behavior, or related Win32 display APIs should be handled carefully.

For display-affecting changes:

- Include diagnostics or logging where appropriate.
- Ensure failure states are visible and recoverable.
- Restore prior display state where possible.
- Do not report the filter as active unless the display backend call actually succeeded.
- Include manual Windows test notes in the PR summary.

Manual test notes should cover, when relevant:

- Enabling and disabling the red-only filter.
- Confirming black remains black and white becomes pure red.
- Confirming green and blue output are eliminated.
- Normal app exit and display restoration.
- Failure/recovery behavior.
- Multi-monitor behavior.
- Sleep/wake, lock/unlock, display hotplug, or display settings changes if the change touches those areas.
