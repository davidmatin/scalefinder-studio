# Premium Features Roadmap - Scale Finder Studio

**Last Updated**: October 11, 2025
**Status**: Planning Phase

---

## Current App Analysis

### Core Concept
Interactive piano-based scale identifier that shows:
- All possible keys/scales from selected notes
- Diatonic chord progressions (roman numerals + chord names)
- Relative major/minor relationships
- Circle of Fifths music theory correctness

### Current Free Features
- Piano keyboard with audio playback (iOS/desktop optimized)
- Scale identification (Major/Minor only)
- Diatonic chord progressions (I-ii-iii-IV-V-vi-vii°)
- History log (localStorage)
- Relative key display
- Clerk authentication

### Current Freemium Limits
For logged-out users:
- **Banner triggers**: 10 piano clicks OR 2 scale clicks OR 2 dropdown selections
- **Modal triggers**: 25 piano clicks OR 10 scale clicks OR 5 dropdown selections

---

## 🎹 Premium Features (Tiered Approach)

### **Tier 1: Practice & Learning Tools** - $4.99/month
*Target: Musicians learning scales and theory*

#### 1. Extended Scale Types
Add beyond Major/Minor:
- **Modes**: Dorian, Phrygian, Lydian, Mixolydian, Locrian, Aeolian
- **Minor variants**: Harmonic Minor, Melodic Minor
- **Blues**: Major Blues (1-2-♭3-3-5-6), Minor Blues (1-♭3-4-♭5-5-♭7)
- **Pentatonic**: Major Pentatonic (1-2-3-5-6), Minor Pentatonic (1-♭3-4-5-♭7)
- **Other**: Whole Tone, Diminished (Half-Whole, Whole-Half)

**Implementation**: Extend `SCALE_INTERVALS` object (line 1079)
```javascript
const SCALE_INTERVALS = {
  Major: [0, 2, 4, 5, 7, 9, 11],
  Minor: [0, 2, 3, 5, 7, 8, 10],
  // Premium scales below
  "Harmonic Minor": [0, 2, 3, 5, 7, 8, 11],
  "Melodic Minor": [0, 2, 3, 5, 7, 9, 11],
  Dorian: [0, 2, 3, 5, 7, 9, 10],
  Phrygian: [0, 1, 3, 5, 7, 8, 10],
  // ... etc
};
```

#### 2. Scale Degree Visualization
- Display scale degree numbers (1, 2, 3, 4, 5, 6, 7) on piano keys
- Show interval names (Major 2nd, Perfect 5th, Minor 7th, etc.)
- Color-coded highlights:
  - **Root** (1): Purple/indigo (brand color)
  - **3rd & 7th**: Blue (character tones)
  - **5th**: Green (stable tone)
  - **2nd, 4th, 6th**: Gray (passing tones)

**Visual Design**: Small badge overlay on piano keys showing degree number

#### 3. Practice Mode
Interactive learning tools:
- **Scale Quiz**: App plays random scale → user identifies name
- **Chord Progression Practice**: Highlight next chord in sequence, user clicks
- **Ear Training**: Play interval → user identifies (m3, P5, M7, etc.)
- **Progress Tracking**: Stats on accuracy, streaks, XP system

**UI**: Modal overlay with quiz interface, timer, score tracking

#### 4. Enhanced History (Cloud-Synced)
- Save history to database (not just localStorage)
- **Named Sessions**: "Gospel Progressions Study - 3/15/24"
- **Session Notes**: Text field for each entry
- **Tags**: #gospel, #jazz, #practice, #composition
- **Export**: PDF chord charts, CSV data export
- **Search & Filter**: Find old sessions by date, tag, or key

**Data Storage**: Requires database (Supabase/Firebase/Convex)

---

### **Tier 2: Advanced Theory** - $9.99/month
*Includes Tier 1 + advanced music theory*

#### 5. Extended Chord Progressions
Beyond triads (Cm, F, G):
- **7th chords**: Cmaj7, Dm7, G7, Bm7♭5, etc.
- **9th chords**: Cmaj9, Dm9, G9
- **11th & 13th**: Extensions for jazz/gospel
- **Suspended**: Csus2, Gsus4, C7sus4
- **Added tones**: Cadd9, Fadd6
- **Alterations**: C7♭9, G7♯11, Dm7♭5

**Implementation**: Extend chord pattern arrays
```javascript
const MAJOR_CHORD_PATTERN_7TH = [
  { roman: 'Imaj7', quality: 'maj7', formula: [0, 4, 7, 11] },
  { roman: 'iim7', quality: 'min7', formula: [0, 3, 7, 10] },
  { roman: 'iiim7', quality: 'min7', formula: [0, 3, 7, 10] },
  { roman: 'IVmaj7', quality: 'maj7', formula: [0, 4, 7, 11] },
  { roman: 'V7', quality: 'dom7', formula: [0, 4, 7, 10] },
  { roman: 'vim7', quality: 'min7', formula: [0, 3, 7, 10] },
  { roman: 'viim7♭5', quality: 'half-dim', formula: [0, 3, 6, 10] }
];
```

#### 6. Chord Voicing Suggestions
Show **how** to play chords on piano:
- **Gospel piano voicings**: Left hand (root-7th), Right hand (3rd-5th-9th)
- **Shell voicings**: Root-3rd-7th only (for walking bass)
- **Rootless voicings**: Jazz style (no root, assume bass plays it)
- **Close vs Open**: Voicing within octave vs spread out
- **Visual piano diagram**: Highlight exact keys to press for each chord

**UI**: Interactive mini-piano showing chord voicing with labeled fingers (1-5)

#### 7. Modulation Helper
Key change suggestions:
- **Pivot Chords**: Chords that exist in both keys
  - Example: C Major → G Major via Em chord (iii in C, vi in G)
- **Common Modulations**:
  - Up a 4th (C → F) - "Truck driver modulation"
  - Up a half-step (C → C# / D♭) - Gospel/soul style
  - Relative major/minor (already shown)
  - Parallel major/minor (C Major → C Minor)
- **Circle of Fifths Visualization**: Interactive circle showing current key + suggested destinations

**UI**: Modal or sidebar showing modulation paths with audio preview

#### 8. Scale Comparison Tool
Side-by-side analysis:
- Select 2+ scales (e.g., C Major vs C Dorian vs C Mixolydian)
- Highlight **shared notes** (green) vs **different notes** (red)
- Show interval differences: "C Dorian differs from C Major only in ♭3 and ♭7"
- Use case: Understanding modal interchange, borrowing chords

**UI**: Split-screen piano with color-coded keys

---

### **Tier 3: Creator Studio** - $19.99/month
*Includes Tier 1 + 2 + composition tools*

#### 9. Progression Builder
Drag-and-drop chord progression creator:
- **Interface**: Horizontal timeline with draggable chord blocks
- **Chord Library**: Click to add chords (Cmaj7 → Dm7 → G7 → Cmaj7)
- **Playback**: Play progression in sequence with adjustable tempo
- **MIDI Export**: Download as .mid file for DAW import
- **Save to Library**: Store custom progressions with names
- **Templates**: Common progressions (I-V-vi-IV, ii-V-I, I-vi-ii-V)

**Tech**: Requires sequencer logic with Web Audio API scheduling

#### 10. Loop & Tempo Control
Practice tools for progressions:
- **Loop Mode**: Repeat progression indefinitely
- **BPM Control**: Slider from 60-180 BPM
- **Metronome**: Click track (on/off toggle)
- **Count-In**: 4-beat count before progression starts
- **Chord Duration**: Adjust how long each chord plays (1 bar, 2 bars, etc.)

**Use Case**: Practice improvising over changes, internalize progressions

#### 11. Multi-Octave Piano
Expand beyond current single-octave view:
- **2-3 octave display** (desktop only, single octave on mobile)
- **Octave shift buttons**: ← → to move viewing window
- **Use case**: Visualize chord voicings across keyboard range
- **Scrollable piano**: Horizontal scroll on tablet/desktop
- **Highlighted register**: Show "middle C" and octave numbers (C3, C4, C5)

**Implementation**: Extend piano component to render 24-36 keys instead of 12

#### 12. Export & Share
Content creation tools:
- **Shareable Links**: Generate URL like `scalefinder.studio/p/xyz123` with embedded scale/progression
- **PNG Export**: Download chord chart as image (for Instagram, teaching materials)
- **PDF Chord Sheets**: Print-friendly lead sheets with roman numerals and chord names
- **Embed Code**: `<iframe>` snippet for blogs/websites
- **Social Preview**: Auto-generate Open Graph image for link sharing

**Tech**: Server-side rendering or client-side canvas rendering for images

---

## 🎯 Implementation Phases

### **Phase 1: Quick Wins** (2-3 weeks)
*Features that align with current architecture, minimal new systems*

1. ✅ **Extended Scale Types** - Add to `SCALE_INTERVALS` (easy)
2. ✅ **Scale Degree Visualization** - Enhance piano keys with degree badges
3. ✅ **Cloud-Synced History** - Use Clerk user metadata or simple DB
4. ✅ **7th Chords** - Extend chord pattern arrays

**Why these first**:
- No new UI paradigms
- Leverage existing piano/chord components
- Immediate value for users
- Test subscription conversion rates

---

### **Phase 2: Practice Tools** (4-6 weeks)
*Interactive learning features*

5. ✅ **Practice Mode / Quiz System** - Modal overlay with quiz logic
6. ✅ **Chord Voicing Visualizations** - Mini-piano diagrams
7. ✅ **Named History Sessions** - Enhanced history UI + DB schema
8. ✅ **Modulation Helper** - Key relationship suggestions

**Why these next**:
- Builds on Phase 1 scales/chords
- High engagement features (quiz, practice)
- Requires more complex UI/UX design

---

### **Phase 3: Advanced Tools** (6-8 weeks)
*Composition and creator tools*

9. ✅ **Progression Builder with MIDI Export** - Timeline UI + sequencer logic
10. ✅ **Loop & Tempo Control** - Audio scheduling system
11. ✅ **Multi-Octave Piano** - Extend piano component architecture
12. ✅ **Scale Comparison Tool** - Split-screen piano UI

**Why these last**:
- Most complex features (sequencer, MIDI export)
- Require significant UI/UX work
- Build on all previous features

---

## 💡 Why These Features Work

### Alignment with Core Principles
- ✅ **Educational**: Learn scales, modes, voicings, theory concepts
- ✅ **Interactive**: Practice mode, progression builder, quizzes
- ✅ **Piano-focused**: All features centered around keyboard visualization
- ✅ **Music theory accurate**: Correct enharmonics, Circle of Fifths, voice leading

### Target Audience Personas

**1. Gospel Piano Players** (Primary audience)
- Needs: Voicings, 7th/9th chords, progressions (ii-V-I, I-vi-ii-V)
- Premium features: Tier 2 (voicings) + Tier 3 (progression builder)
- Willingness to pay: High ($10-20/month for serious players)

**2. Jazz Musicians**
- Needs: Modes, extensions, rootless voicings, modulation, altered chords
- Premium features: All tiers (comprehensive theory tools)
- Willingness to pay: High (professional development)

**3. Music Students**
- Needs: Practice mode, scale comparison, ear training, visual learning
- Premium features: Tier 1 (learning tools) + Tier 2 (theory)
- Willingness to pay: Medium ($5-10/month for homework help)

**4. Songwriters / Composers**
- Needs: Progression builder, MIDI export, loop practice, sharing
- Premium features: Tier 3 (creator tools)
- Willingness to pay: High (professional tool)

### Monetization Strategy

**Tier 1: $4.99/month** - Captures hobbyists and students
- Low barrier to entry
- Educational focus
- Conversion target: 5-10% of free users

**Tier 2: $9.99/month** - Serious musicians
- "Sweet spot" pricing for advanced features
- Includes everything from Tier 1
- Conversion target: 30-40% of Tier 1 users upgrade

**Tier 3: $19.99/month** - Professional creators
- High-value tools (MIDI export, sharing)
- Premium positioning
- Conversion target: 10-20% of Tier 2 users upgrade

**Annual Discounts**:
- $49/year (Tier 1) - Save $10
- $99/year (Tier 2) - Save $20
- $199/year (Tier 3) - Save $40

---

## 📊 Technical Considerations

### Data Storage Requirements

**Current**:
- localStorage only (history, usage stats)
- No persistent user data

**Premium Needs**:
1. **User Settings**: Tier level, preferences, feature flags
2. **Saved Progressions**: Custom chord sequences, names, tags
3. **Practice Stats**: Quiz scores, streaks, accuracy percentages
4. **History Sessions**: Named sessions with notes and tags

**Storage Options**:

| Option | Pros | Cons | Cost |
|--------|------|------|------|
| **Clerk User Metadata** | Free, simple, already integrated | 1KB limit per user | Free |
| **Supabase** | Postgres DB, real-time, auth integration | Learning curve | Free tier + $25/mo |
| **Firebase Firestore** | Easy setup, good docs, generous free tier | NoSQL (complex queries harder) | Free tier + usage |
| **Convex** | Real-time, TypeScript, excellent DX | Newer (less mature) | Free tier + $25/mo |

**Recommendation**: Start with **Clerk User Metadata** for settings/tier, add **Supabase** when history/progressions need full DB.

---

### Audio System Enhancements

**Current**: Hybrid audio (Web Audio API + HTML5 Audio for iOS)

**Premium Needs**:
1. **Sequencer**: Schedule multiple notes in progression
2. **Tempo Control**: Adjust playback speed (BPM)
3. **Looping**: Repeat progressions seamlessly
4. **Metronome**: Click track

**Implementation**:
```javascript
// Web Audio API scheduling for progression playback
function playProgression(chords, bpm) {
  const ctx = audioContext;
  const secondsPerBeat = 60 / bpm;
  let time = ctx.currentTime;

  chords.forEach((chord, i) => {
    // Schedule each chord's notes
    chord.notes.forEach(notePC => {
      playNoteAtTime(notePC, time, secondsPerBeat);
    });
    time += secondsPerBeat * 4; // 4 beats per chord
  });
}
```

**Challenge**: iOS HTML5 Audio doesn't support precise scheduling
**Solution**: Use Web Audio API for progression playback (won't work with silent switch ON, acceptable for premium feature)

---

### MIDI Export

**Library**: [`midi-writer-js`](https://github.com/grimmdude/MidiWriterJS) (13KB, no deps)

**Example**:
```javascript
import MidiWriter from 'midi-writer-js';

function exportProgressionToMIDI(chords, bpm) {
  const track = new MidiWriter.Track();
  track.setTempo(bpm);

  chords.forEach(chord => {
    const notes = chord.notes.map(pc => pcToMIDI(pc));
    track.addEvent(new MidiWriter.NoteEvent({
      pitch: notes,
      duration: '1', // 1 bar
      velocity: 100
    }));
  });

  const write = new MidiWriter.Writer(track);
  return write.buildFile(); // Returns UInt8Array
}
```

**File Download**:
```javascript
const blob = new Blob([midiData], { type: 'audio/midi' });
const url = URL.createObjectURL(blob);
const a = document.createElement('a');
a.href = url;
a.download = 'progression.mid';
a.click();
```

---

### Feature Gating

**Check Tier Level**:
```javascript
// In App component
const userTier = clerk.user?.publicMetadata?.tier || 'free';
const isPremiumTier1 = ['tier1', 'tier2', 'tier3'].includes(userTier);
const isPremiumTier2 = ['tier2', 'tier3'].includes(userTier);
const isPremiumTier3 = userTier === 'tier3';
```

**Conditional Rendering**:
```javascript
// Show premium scales only for Tier 1+
const availableScales = isPremiumTier1
  ? ['Major', 'Minor', 'Dorian', 'Phrygian', 'Lydian', ...]
  : ['Major', 'Minor'];

// Show "Upgrade" badge for locked features
{!isPremiumTier2 &&
  React.createElement('div', { className: 'upgrade-badge' }, '🔒 Tier 2')
}
```

**Setting Tier in Clerk Dashboard**:
- After Stripe payment success webhook
- Update user metadata: `clerk.users.updateUserMetadata(userId, { publicMetadata: { tier: 'tier1' } })`

---

### Single-File Architecture Compatibility

**Can we keep single-file HTML?**
- ✅ **YES** for Phase 1-2 (scale types, 7th chords, practice mode, voicings)
- ⚠️ **MAYBE** for Phase 3 (progression builder, MIDI export will push limits)
- ❌ **NO** long-term (database integration, Stripe webhooks need backend)

**When to migrate**:
- File exceeds ~5000 lines (currently ~3000)
- Need external libraries (MIDI export)
- Need backend API routes (Stripe, database operations)

**Migration Path**:
1. Extract to modules: `music-theory.js`, `audio.js`, `components.js`
2. Add build system: Vite (fast, simple)
3. Keep deployment simple: Still output single `index.html` via `vite build`
4. Add API routes: Next.js or separate Express server

---

## 🚀 Next Steps

### Before Implementation Begins

1. **User Research**
   - Survey current users: "What features would you pay for?"
   - Interview 5-10 gospel pianists / music students
   - Validate pricing with target audience

2. **Competitive Analysis**
   - Compare to: iReal Pro ($15/year), Hooktheory ($100/year), ChordChord (free)
   - Identify gaps in market
   - Refine unique value proposition

3. **Stripe Integration**
   - Set up Stripe account + test mode
   - Create products/prices for 3 tiers
   - Implement Stripe Checkout flow
   - Set up webhooks for subscription events

4. **Database Setup**
   - Choose storage solution (Supabase recommended)
   - Design schema for: users, saved_progressions, practice_stats, history_sessions
   - Set up RLS (Row Level Security) policies

5. **Feature Flagging**
   - Implement tier-based gating in app
   - Add "Upgrade" CTAs for locked features
   - Test upgrade flow end-to-end

### Success Metrics

**Phase 1 (Months 1-2)**:
- 100 beta testers signed up
- 10% conversion to Tier 1 (10 paying users)
- Net Promoter Score (NPS) > 50

**Phase 2 (Months 3-4)**:
- 500 total users
- 50 paying users ($250-500 MRR)
- 30% of Tier 1 users upgrade to Tier 2

**Phase 3 (Months 5-6)**:
- 1000 total users
- 100 paying users ($1000+ MRR)
- 20% of Tier 2 users upgrade to Tier 3

---

## 📝 Open Questions

1. **Freemium Limits**: Should we increase/decrease current limits after premium launch?
2. **Trial Period**: Offer 7-day free trial for Tier 1? Or keep hard paywall?
3. **Educational Discount**: Student pricing (50% off with .edu email)?
4. **Lifetime Pricing**: One-time payment option? (e.g., $299 for lifetime Tier 3)
5. **Collaboration Features**: Allow users to share progressions with each other? (social features)
6. **Mobile App**: Build native iOS/Android apps, or keep web-only?

---

## 📚 Resources & References

### Music Theory References
- Circle of Fifths: https://en.wikipedia.org/wiki/Circle_of_fifths
- Chord Voicings: Mark Levine's "The Jazz Piano Book"
- Gospel Piano: https://www.gospelchops.com

### Technical Libraries
- MIDI Export: https://github.com/grimmdude/MidiWriterJS
- Music Theory: https://github.com/tonaljs/tonal
- Audio Scheduling: https://www.html5rocks.com/en/tutorials/audio/scheduling/

### Competitive Products
- iReal Pro: https://irealpro.com
- Hooktheory: https://www.hooktheory.com
- ChordChord: https://chordchord.com
- Scaler 2 (DAW plugin): https://www.pluginboutique.com/product/3-Studio-Tools/93-Music-Theory-Tools/6439-Scaler-2

---

**End of Roadmap** | Next update after user research phase
