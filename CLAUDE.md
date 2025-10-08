# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Scale Finder Studio** (scalefinder.studio) - An interactive piano-based music theory tool that identifies possible musical keys and scales based on selected notes. Live at https://scalefinder.studio

## Architecture

### Technology Stack
- **Single-file HTML application** - All code in `index.html` (~2600 lines)
- **React 18** via CDN (UMD build, no build system)
- **Tailwind CSS** via CDN
- **Clerk Authentication** (Browser SDK) for user auth
- **Web Audio API** for piano samples and audio playback
- **No package.json** - This is intentionally a standalone HTML file

### Code Structure (within index.html)

The application is organized in this order:

1. **Styles** (lines 21-609)
   - Custom CSS for piano keys, cards, animations
   - Dark theme with purple/indigo gradients
   - Auth button hover states

2. **React/Dependencies** (lines 611-628)
   - React 18 UMD from unpkg
   - Clerk Browser SDK from `superb-bream-93.clerk.accounts.dev`

3. **Main Application Script** (lines 639-2612)
   - Async IIFE wrapper for Clerk initialization
   - Music theory data structures
   - Custom React hooks
   - Audio engine
   - Main App component
   - ReactDOM render

### Key Music Theory Data Structures

- **PITCHES** (line ~685): 12 chromatic pitches with display names (uses flat symbols for display)
- **SCALE_INTERVALS** (line ~792): Interval patterns for Major and Minor scales
- **RELATIVE_KEYS** (line ~797): Mapping between relative major/minor keys
- **MAJOR_CHORD_PATTERN / MINOR_CHORD_PATTERN** (lines ~728-746): Chord quality patterns for scale degrees

### Core Functions

- **getPossibleKeys(selected)** (line ~841): Main algorithm that filters all 24 keys based on selected pitch classes
- **usePitchSelection()** (line ~1102): Custom hook managing selected notes state
- **usePianoSamples()** (line ~855): Audio engine with Web Audio API, piano samples, reverb
- **getChordProgressions(keyName)** (line ~749): Calculates diatonic chords for a given key
- **App()** (line ~1687): Main React component

### State Management

- **localStorage** for:
  - `keyclicker_history`: Saved scale history (array of objects)
  - `instagram_gate_seen`: Whether user dismissed the Instagram gate modal
- **Clerk** for authentication state (user sessions managed server-side)

### Authentication

- **Clerk publishable key**: `pk_test_c3VwZXJiLWJyZWFtLTkzLmNsZXJrLmFjY291bnRzLmRldiQ`
- User button mounted in upper-right header after sign-in
- Sign In/Sign Up buttons appear when logged out
- Clerk SDK loaded async before React app initialization (lines 656-664)

## Development

### Local Testing
```bash
# Start local server (Python 3)
python3 -m http.server 8000

# Or with Node.js
npx http-server -p 8000
```

Visit `http://localhost:8000`

### Deployment
Currently deployed to **Cloudflare Pages** at https://scalefinder.studio

### Making Changes

**Important**: This is a single HTML file with inline React code using `React.createElement()`. There is no JSX, no build step, no transpilation.

When editing:
- All React components use `React.createElement()` syntax
- Maintain the existing music theory algorithms (they're mathematically precise)
- Preserve the dark theme color palette (purple/indigo: `#6366f1`, `#8b5cf6`, `#a855f7`)
- Test audio features across browsers (Safari, Chrome, Firefox handle Web Audio differently)

### Audio System Notes

The piano samples are loaded from `assets/piano-samples/` directory:
- Files named by pitch class: `C.mp3`, `Cs.mp3` (sharp = 's'), `D.mp3`, etc.
- Multiple fallback paths attempted (assets/, ./assets/, /assets/, /nodebox/assets/)
- Uses Web Audio API with reverb convolver for spatial depth
- Master volume control with gain node

### Common Tasks

**Add new scale type**: Update `SCALE_INTERVALS` with interval pattern, update chord patterns if needed

**Modify authentication UI**: Edit auth section in header (lines ~2108-2168)

**Change theme colors**: Update CSS variables and inline styles (search for `#6366f1`, `#8b5cf6`, `#a855f7`)

## Clerk Integration Notes

- Script must load from `https://superb-bream-93.clerk.accounts.dev/npm/@clerk/clerk-js@latest/dist/clerk.browser.js`
- Uses `data-clerk-publishable-key` attribute for auto-initialization
- Wait for `window.Clerk` before app initialization
- User button mounted via `clerk.mountUserButton(element)` in useEffect
- Production domain `scalefinder.studio` must be whitelisted in Clerk dashboard

## Known Constraints

- Single HTML file architecture is intentional (easy deployment, no build complexity)
- No TypeScript (vanilla JS only)
- No module system (everything in global scope within IIFE)
- localStorage only (no database yet - user data not persisted to Clerk)
