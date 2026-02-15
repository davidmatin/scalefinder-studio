# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Scale Finder Studio** (scalefinder.studio) - An interactive piano-based music theory tool that identifies possible musical keys and scales based on selected notes. Live at https://scalefinder.studio

## Architecture

### Technology Stack
- **Single-file HTML application** - All code in `index.html` (~3400+ lines)
- **React 18** via CDN (UMD build, no build system)
- **Tailwind CSS** via CDN
- **Clerk Authentication** (Browser SDK) for user auth
- **Hybrid Audio System** - Web Audio API (desktop/Android) + HTML5 Audio (iOS with silent switch bypass)
- **Automatic platform detection** - iOS vs desktop/Android via user agent
- **Cloudflare Pages Functions** - Serverless API endpoint for analytics
- **Cloudflare D1** - SQLite database for custom event tracking
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

**Hybrid Audio Architecture**: Automatic platform-specific audio systems for optimal experience.

#### Desktop & Android (Web Audio API)
- High-quality playback with reverb convolver for spatial depth (line 1147)
- Reverb impulse response: 1.2 second decay with exponential decay curve (lines 1151-1162)
- Master volume control with gain node (lines 1165-1166)
- Smooth fade-in/fade-out envelopes (50ms attack, 100ms release) at lines 1394-1397
- Console logs: Standard Web Audio API messages

#### iOS (HTML5 Audio)
- HTML5 Audio elements bypass silent switch (works with switch ON)
- Pre-loaded audio pool with stored duration metadata
- 300ms exponential fade-out via `requestAnimationFrame` (60fps smooth)
- No reverb (trade-off for silent switch compatibility)
- Console logs: "[iOS Mode] Playing: C.mp3 (duration: 2.47s, fade: 0.3s exponential)"

#### Piano Samples
- Files located in `assets/samples/` directory
- Named by pitch class: `C.mp3`, `Cs.mp3` (sharp = 's'), `D.mp3`, `Ds.mp3`, etc.
- Multiple fallback paths attempted in loadNote() function (line 1242): tries `assets/samples/`, `./assets/samples/`, `/Users/adrm/Desktop/keyclick/assets/samples/`, `samples/`, `./samples/`
- Volume slider range: 0-100, mapped to gain values 0.0-1.0 (line 1434 for HTML5, masterGain for Web Audio)

#### iOS Audio System - Automatic Silent Switch Bypass
**Critical for iPhone users**: Automatic platform detection and dual audio system ensures audio works with iPhone silent switch ON:

**Platform Detection (line ~2130):**
- Automatically detects iOS devices via user agent: `/iPhone|iPad|iPod/.test(navigator.userAgent)`
- No user interaction required - completely transparent
- iOS users get HTML5 Audio, desktop/Android users get Web Audio API

**iOS Silent Switch Bypass (lines 1186-1232):**
- Creates brief silent audio on first user interaction to unlock media playback
- **Triggered automatically on FIRST touch/click anywhere on page** (global document listener at lines 1544-1560)
- Only runs once per session (tracked with `iosUnlockedRef`)
- Console logs: "[iOS Bypass] ✅ UNLOCKED - AudioContext should stay unlocked for session"

**HTML5 Audio System for iOS (lines 1410-1503):**
- Pre-loads all 12 piano samples with duration metadata (lines 1521-1542)
- Uses `cloneNode()` for instant playback with natural note overlap
- **Exponential fade-out** eliminates clicking sounds at note end (300ms fade)
- Fade algorithm: `volume = startVolume × 0.01^progress` (natural piano decay)
- Uses `requestAnimationFrame` for smooth 60fps fade-out
- Console logs: "[iOS Mode] Playing: C.mp3 (duration: 2.47s, fade: 0.3s exponential)"

**Technical Details:**
- Duration captured via `loadedmetadata` event and stored in pool
- Fixes issue where `cloneNode()` doesn't inherit metadata
- No reverb on iOS (trade-off for silent switch bypass)
- See `audio-click-fix-plan.md` for complete technical deep-dive

This solves the common UX issue where new users on iPhone with silent mode enabled hear no sound and abandon the site. The automatic detection ensures zero user friction.

### Detected Keys Section Layout

**Title and Label Alignment:**
- "DETECTED KEYS" heading: Left-aligned with `text-left` class and `marginLeft: "0.75rem"` (line 1788-1789)
- "MAJOR" and "MINOR" labels: Left-aligned via CSS `text-align: left` in `.scale-group-label` (line 529)
- Section labels have `margin-left: 0.75rem` for consistent left indentation

**Button Alignment:**
- Key buttons (e.g., "C Major", "D Minor"): Center-aligned horizontally using `justify-center` class (line 1805, 1894)
- Maintains centered appearance for button rows while keeping headings left-aligned

**Container Alignment:**
- Keys column uses `items-center` for vertical centering of button groups (line 3068)
- Grid container uses `items-start` to prevent unwanted vertical centering when fewer keys displayed (line 3048)
- Full width (`w-full`) on containers ensures proper title alignment regardless of content amount

**Spacing Between Sections:**
- `.scale-group` margins: `margin-top: 1rem`, `margin-bottom: 0.75rem` (line 474-475)
- Balanced 12-20px vertical spacing between MAJOR and MINOR sections
- First section uses `.no-separator` class to remove bottom padding/margin (line 495-498)

### Responsive Design

Mobile breakpoints:
- **768px**: Stacks header vertically, centers auth buttons below logo
- **390px**: Enables horizontal piano scrolling, mobile-optimized overflow handling
- **375px**: Full mobile optimization (iPhone SE), horizontal touch scrolling for piano card

Media query sections: lines 745-809

#### Mobile UI Alignment - Relative Key Pairs

When only major/minor relative pair is displayed (`isRelativePairOnly` condition):

**Piano Card Centering (line 2829):**
- `transform: "translateX(-20px)"` - Shifts piano left to compensate for viewport asymmetry on mobile
- Ensures visual centering on small screens

**Button Container Layout (line 3032):**
- Fixed `height: "165px"` for relative pairs (auto for multiple keys)
- Provides consistent vertical spacing and prevents container expansion

**Keys Column Vertical Centering (line 3068):**
- Uses `items-center` for vertical centering of buttons within container
- `marginTop: "-2px"` applied when relative pair to fine-tune vertical position
- `justifyContent: "center"` for horizontal centering of relative pair buttons

**Button Positioning (line 1780):**
- `paddingTop: "0"` - Removes top padding for tighter layout
- `paddingBottom: "1rem"` - Maintains bottom spacing
- `paddingLeft: "1.5rem"` - Shifts buttons right to center them within their container
- Full width (`w-full`) ensures proper left-alignment of section titles

**X Button Vertical Alignment:**
- Major button X (line 1854): `marginTop: "-35px"` - Moves X up relative to button
- Minor button X (line 1955): `marginTop: "-25px"` - Adjusted for tighter subtitle spacing
- Ensures both X buttons align at same vertical position despite different button structures

**Subtitle Spacing (line 1907):**
- Button wrapper uses `gap-0` (no gap between button and subtitle)
- Minor button wrapper: `marginTop: "0.5rem"` for tighter vertical spacing
- "relative minor" text uses `height: "auto"` and `marginTop: "0.25rem"` for closer positioning to button

**Button Height Matching:**
- Major button (line 1838): 30px invisible placeholder (`visibility: hidden`) maintains vertical space
- Minor button (line 1940): Auto height with `marginTop: "0.25rem"` for tight spacing
- Balanced spacing ensures clean visual alignment

**Clerk User Button on Mobile (lines 768-783):**
- Changed from centered-below-logo to upper-right corner via absolute positioning
- `position: absolute`, `top: 0`, `right: 0` for consistency with desktop layout
- Provides familiar auth UI pattern across all devices

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

## Analytics

### Cloudflare Web Analytics (Pageviews)
- Auto-injected by Cloudflare Pages — no manual beacon script needed
- Dashboard: Cloudflare Dashboard > Analytics & Logs > Web Analytics > scalefinder.studio
- Tracks: pageviews, referrers, countries, devices, Core Web Vitals

### Custom Event Tracking (D1 + Pages Functions)
- **Endpoint**: `POST /api/track` — implemented in `functions/api/track.js`
- **Database**: Cloudflare D1 `scalefinder_analytics` (ID: `3f86c335-ebfb-4345-a47d-ea4d224327a4`)
- **Binding**: `DB` configured in Pages project settings and `wrangler.toml`
- **Client-side**: `trackEvent(name, properties)` utility with batching (5 events or 5s flush interval)
- **Transport**: `navigator.sendBeacon()` with `fetch({ keepalive: true })` fallback
- **Session tracking**: UUID v4 generated per page load (`_analyticsSessionId`)
- **Auto-flush**: On `visibilitychange` (hidden) and `pagehide` events

#### Tracked Events
| Event | Properties | Location |
|-------|-----------|----------|
| `piano_click` | `pitch_class`, `note_name`, `action` (select/deselect) | `handleKey()` |
| `scale_selected` | `key_name`, `source` (button/dropdown) | `applyScale()`, dropdown onChange |
| `mute_toggle` | `muted` (new state) | Speaker button onClick |
| `volume_change` | `volume` (0.0–1.0, debounced 500ms) | `handleVolumeChange()` |
| `selection_reset` | `previous_count` | `resetSelection()` |
| `history_cleared` | `count` | `onClear` handler |

#### Querying Analytics
```bash
# Via wrangler CLI
npx wrangler d1 execute scalefinder_analytics --remote --command "SELECT * FROM analytics_events ORDER BY timestamp DESC LIMIT 20;"

# Or via Cloudflare Dashboard > Workers & Pages > D1 > scalefinder_analytics > Console
```

## SEO & LLM Optimization

### Meta Tags (in `<head>`)
- **Canonical URL**: `<link rel="canonical" href="https://scalefinder.studio" />`
- **Favicon**: SVG emoji favicon (🎹) via data URI
- **Keywords**: scale finder, piano scales, music theory, key identifier, chord progressions, etc.
- **Open Graph**: Full set (og:type, og:url, og:title, og:description, og:image with 1200x630 image)
- **Twitter Card**: summary_large_image with title, description, and image
- **JSON-LD**: WebApplication schema with features list, free pricing, MusicApplication category

### Static SEO Files
- **`robots.txt`**: Allows all crawlers, points to sitemap
- **`sitemap.xml`**: Single-page sitemap with lastmod date
- **`llms.txt`**: Structured description for LLM crawlers (features, how it works, audience)
- **`_headers`**: Cloudflare Pages security headers (X-Content-Type-Options, X-Frame-Options, Referrer-Policy) + cache control for assets
- **`assets/og-image.png`**: 1200x630 social share image (dark gradient with piano keys)

### Noscript Fallback
- `<noscript>` block provides crawlable HTML content for search engines that don't execute JavaScript
- Contains app description, feature list, and "enable JavaScript" message

## Known Constraints

- Single HTML file architecture is intentional (easy deployment, no build complexity)
- No TypeScript (vanilla JS only)
- No module system (everything in global scope within IIFE)
- localStorage for user data (not persisted to Clerk); Cloudflare D1 for analytics only
- Tailwind CSS must remain on CDN (no local build system)
- No npm/package.json by design

## File Structure

```
keyclick/
├── index.html                      # Entire application (~3400+ lines)
├── assets/
│   ├── og-image.png                # 1200x630 social share image
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
├── functions/
│   └── api/
│       └── track.js                # Pages Function: POST /api/track (analytics)
├── migrations/
│   └── 0001_create_analytics_events.sql  # D1 table schema
├── wrangler.toml                   # D1 database binding config
├── robots.txt                      # Search engine crawl directives
├── sitemap.xml                     # Single-page sitemap
├── llms.txt                        # LLM crawler guidance
├── _headers                        # Cloudflare Pages security + cache headers
├── .claude/
│   └── settings.local.json         # Claude Code permissions config
├── .gitignore
├── CLAUDE.md                       # This file
├── CHANGELOG-2025-10-14.md         # UI alignment fixes and enhancements (Oct 14)
├── CHANGELOG-2025-10-21.md         # Matching count, notes redesign, volume controls (Oct 21)
└── audio-click-fix-plan.md         # Technical deep-dive: iOS audio click fix
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
