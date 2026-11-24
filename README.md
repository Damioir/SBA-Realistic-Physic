# SBA-Realistic-Physic

Native Android mod project for Super Bear Adventure.

## Costume map editor work

The `costume-map-editor` branch now contains the first editor core:

- Map elements for props, chairs, instant-death blocks, text messages, triggers, and animation events.
- Validation for IDs, element counts, text sizes, positions, rotations, and scales.
- A versioned, escaped `.sbamap` document format with encode/decode functions.
- In-memory editor operations for selecting, adding, moving, rotating, scaling, deleting, grid snapping, undo, and redo.

This is still source code, not an installable or playable mod. It does not yet show an editor in-game, spawn Unity objects, or save files to device storage. The next work is the in-game UI, map storage, Unity object adapter, and packaging as an `.sbapack` for SBAModLoader. Target game version: Super Bear Adventure 13.0.9.

See `src/costume_map.h` and `src/editor_session.h` for the document and editing APIs.
