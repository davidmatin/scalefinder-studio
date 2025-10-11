# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Scale Finder Studio** (scalefinder.studio) - An interactive piano-based music theory tool that identifies possible musical keys and scales based on selected notes. Live at https://scalefinder.studio

## Architecture

### Technology Stack
- **Single-file HTML application** - All code in `index.html` (~2869 lines)
- **React 18** via CDN (UMD build, no build system)
- **Tailwind CSS** via CDN
- **Clerk Authentication** (Browser SDK) for user auth
- **Web Audio API** for piano samples and audio playback
- **No package.json** - This is intentionally a standalone HTML file

### Code Structure (within index.html)

The application is organized in this order:

1. **Styles** (lines 21-812)
   - Custom CSS for piano keys, cards, animations
   - Dark theme with purple/indigo gradients
   - Auth button hover states
   - Freemium banner and modal styles
   - Responsive breakpoints: 768px, 390px, 375px

2. **React/Dependencies** (lines 814-832)
   - React 18 UMD from unpkg
   - Clerk Browser SDK from `superb-bream-93.clerk.accounts.dev`

3. **Main Application Script** (lines 833-2869)
   - Async IIFE wrapper for Clerk initialization
   - Music theory data structures (lines 859-994)
   - Custom React hooks (lines 1029-1884)
   - Audio engine (lines 1029-1274)
   - React components (lines 1276-1884)
   - Main App component (line 1886)
   - ReactDOM render (line 2857)

### Key Music Theory Data Structures

- **PITCHES** (line 859): 12 chromatic pitches with internal names (sharp-based) and display names (uses flat symbols for certain pitches like E♭, A♭, B♭)
- **ENHARMONICS** (line 875): Maps pitch class numbers to their sharp/flat equivalents (C#/D♭, D#/E♭, F#/G♭, G#/A♭, A#/B♭)
- **getKeyDisplayName()** (line 884): Converts internal key names to display names with proper flat symbols
- **MAJOR_CHORD_PATTERN** (line 902): Chord quality patterns for major scale degrees [I, ii, iii, IV, V, vi, vii°]
- **MINOR_CHORD_PATTERN** (line 912): Chord quality patterns for minor scale degrees [i, ii°, III, iv, v, VI, VII]
- **SCALE_INTERVALS** (line 966): Interval patterns for Major [0,2,4,5,7,9,11] and Minor [0,2,3,5,7,8,10] scales
- **RELATIVE_KEYS** (line 971): Bidirectional mapping between all 24 relative major/minor key pairs

### Core Functions

- **getChordProgressions(keyName)** (line 923): Calculates diatonic chords for a given key with roman numerals and chord qualities
- **getPossibleKeys(selected)** (line 1015): Main algorithm that filters all 24 keys based on selected pitch classes
- **usePianoSamples(masterVolume)** (line 1029): Audio engine with Web Audio API, piano samples, reverb convolver, and volume control
- **usePitchSelection()** (line 1276): Custom hook managing selected notes state with localStorage persistence
- **App()** (line 1886): Main React component containing all UI logic

### UI Components

- **Piano component**: Interactive piano keyboard with white and black keys, hover effects, and click handling
- **Volume control slider**: Positioned under mute button, appears on click, range 0-100 mapped to gain 0.0-1.0
- **Mute button**: Toggles audio on/off with visual icon feedback
- **Auth section** (line 2428): Sign In/Sign Up buttons when logged out, User button when authenticated
- **Freemium banner**: Fixed top banner prompting sign-up after usage limit reached
- **Freemium modal**: Centered modal overlay for sign-up prompts

### State Management

- **localStorage** for:
  - `keyclicker_history`: Saved scale history (array of objects with selected pitch classes and timestamps)
  - Usage tracking for freemium limits
- **Clerk** for authentication state (user sessions managed server-side)

### Authentication

- **Clerk publishable key**: `pk_test_c3VwZXJiLWJyZWFtLTkzLmNsZXJrLmFjY291bnRzLmRldiQ`
- **Clerk domain**: `superb-bream-93.clerk.accounts.dev`
- User button mounted in upper-right header after sign-in (line 2132)
- Sign In/Sign Up buttons appear when logged out (lines 2428-2506)
- Clerk SDK loaded async before React app initialization (lines 833-861)

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
- **Never remove Tailwind CSS CDN** - the app depends on it for all layout and styling

### Audio System Notes

The piano samples are loaded from `assets/samples/` directory:
- Files named by pitch class: `C.mp3`, `Cs.mp3` (sharp = 's'), `D.mp3`, `Ds.mp3`, etc.
- Multiple fallback paths attempted in loadNote() function (line 1122): tries `assets/samples/`, `./assets/samples/`, `/assets/samples/`, `/nodebox/assets/samples/`
- Uses Web Audio API with reverb convolver for spatial depth (line 1059)
- Reverb impulse response: 1.2 second decay with exponential decay curve (lines 1062-1073)
- Master volume control with gain node (lines 1076-1077)
- Volume slider range: 0-100, mapped to gain values 0.0-1.0 (line 1289)

#### iOS Silent Switch Bypass
**Critical for iPhone users**: Implemented silent audio unlock technique (lines 1095-1120) to bypass iOS silent switch:
- Creates a silent looping HTML5 audio element on first user interaction
- Forces Web Audio API to use "media" category instead of "ringer" category
- Allows piano sounds to play even when iPhone silent switch is ON
- Triggered automatically on first piano key press or scale playback
- Only runs once per session (tracked with `iosUnlockedRef`)
- Console logs: "iOS audio unlocked - Web Audio will now bypass silent switch"

This solves the common UX issue where new users on iPhone with silent mode enabled hear no sound and abandon the site.

### Responsive Design

Mobile breakpoints:
- **768px**: Stacks header vertically, centers auth buttons below logo
- **390px**: Enables horizontal piano scrolling, mobile-optimized overflow handling
- **375px**: Full mobile optimization (iPhone SE), horizontal touch scrolling for piano card

Media query sections: lines 745-809

### Common Tasks

**Add new scale type**: Update `SCALE_INTERVALS` (line 966) with interval pattern, add chord pattern if needed for major/minor variants

**Modify authentication UI**: Edit auth section in header (lines 2428-2506), Clerk mount logic (lines 2122-2147)

**Change theme colors**: Update CSS custom properties and inline styles (search for `#6366f1`, `#8b5cf6`, `#a855f7`)

**Adjust audio settings**: Modify `usePianoSamples()` hook (line 1029), reverb settings (lines 1040-1056), volume mapping (line 1250)

**Update piano layout**: Modify Piano component rendering logic, adjust key widths based on viewport calculations

**Modify freemium limits**: Update usage tracking logic and modal/banner display conditions in App component

## Clerk Integration Notes

- Script must load from `https://superb-bream-93.clerk.accounts.dev/npm/@clerk/clerk-js@latest/dist/clerk.browser.js`
- Uses `data-clerk-publishable-key` attribute for auto-initialization
- Wait for `window.Clerk` before app initialization (async IIFE at line 833)
- User button mounted via `clerk.mountUserButton(element)` in useEffect (line 2132)
- Sign In and Sign Up flows handled by Clerk's hosted pages

### Required Clerk Dashboard Configuration

In your Clerk Dashboard (https://dashboard.clerk.com), you must whitelist these URLs:

**For Local Development:**
- `http://localhost:8000` - allowed redirect URL
- `http://localhost:8000/` - allowed redirect URL (with trailing slash)

**For Production:**
- `https://scalefinder.studio` - allowed redirect URL
- Add to allowed origins for CORS

Without these settings, sign-in/sign-out will redirect to Clerk's hosted page but fail to return to your app.

## Known Constraints

- Single HTML file architecture is intentional (easy deployment, no build complexity)
- No TypeScript (vanilla JS only)
- No module system (everything in global scope within IIFE)
- localStorage only (no database yet - user data not persisted to Clerk)
- Tailwind CSS must remain on CDN (no local build system)
- No npm/package.json by design

## File Structure

```
keyclick/
├── index.html                      # Entire application (~2869 lines)
├── assets/
│   └── samples/                    # Piano audio samples
│       ├── C.mp3
│       ├── Cs.mp3                  # C# (sharp notation in filename)
│       ├── D.mp3
│       ├── Ds.mp3                  # D# / E♭
│       ├── E.mp3
│       ├── F.mp3
│       ├── Fs.mp3                  # F#
│       ├── G.mp3
│       ├── Gs.mp3                  # G# / A♭
│       ├── A.mp3
│       ├── As.mp3                  # A# / B♭
│       └── B.mp3
├── .gitignore
└── CLAUDE.md                       # This file
```

## Music Theory Implementation Notes

### Scale Detection Algorithm
The `getPossibleKeys()` function uses a simple but effective filtering approach:
1. If no notes selected → show all 24 keys
2. If all 12 notes selected → show "chromatic" message
3. Otherwise → filter keys to only those containing all selected pitch classes

### Enharmonic Equivalents & Music Theory Correctness
The app uses **sharp notation internally** for pitch class names (C#, D#, F#, G#, A#) but displays proper music theory notation following Circle of Fifths conventions:

**Key Name Conversions:**
- D# Major → E♭ Major
- G# Major → A♭ Major
- A# Minor → B♭ Minor
- D# Minor → E♭ Minor

**Chord Progression Accidentals:**

**Sharp Keys** (right side of Circle of Fifths) use sharps (♯):
- Major: C → G → D → A → E → B → F♯ → C♯
- Minor: A → E → B → F♯ → C♯ → G♯ → D♯ → A♯
- Example: E Major shows F♯m, G♯m, C♯m (not Gbm, Abm, Dbm)

**Flat Keys** (left side of Circle of Fifths) use flats (♭):
- Major: F → B♭ → E♭ → A♭
- Minor: D → G → C → F
- Example: E♭ Major shows A♭, B♭ (not G#, A#)

**Implementation:** Functions `usesSharpAccidentals()`, `usesFlatAccidentals()`, and `getNoteNameForKey()` (lines 933-972) determine correct accidentals based on key signature. Critical for gospel piano players and music theory accuracy.

### Chord Progressions
Diatonic chords are calculated by:
1. Finding scale degree roots using interval patterns
2. Applying chord quality patterns (major/minor/diminished) based on scale type
3. Displaying with roman numeral notation (I, ii, iii, IV, V, vi, vii°)

## Quality Assurance & Debugging

### Debug Agents
Multi-agent debug system available at `/Users/adrm/Desktop/debug-agents-package`:

```bash
cd /Users/adrm/Desktop/debug-agents-package
npm install
node cli.js https://scalefinder.studio
```

**Agents included:**
1. **Frontend Debug Agent** - Checks for broken images, accessibility issues, performance metrics
2. **Backend Fix Agent** - Generates fix plans and code suggestions

**Output:**
- Screenshots at multiple breakpoints (iPhone SE, iPhone 12/13/14, iPad, Desktop)
- JSON reports in `debug-output/` directory
- Fix recommendations in `code-fixes.md`

### Known Issues (Intentional)

**Tailwind CSS CDN Warning**
- Debug agents will flag `cdn.tailwindcss.com` usage in production
- This is **intentional** for the single-file HTML architecture
- Acceptable trade-off for deployment simplicity

**Logo Image Element**
- Logo `<img>` has empty display but valid placeholder `src` (data URI)
- Text fallback shown by default via JavaScript (line 2178)
- Prevents broken image warnings from automated tools
