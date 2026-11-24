# SBA-Realistic-Physic

Native Android mod project for Super Bear Adventure. The current native entry point logs when the mod is loaded.

## Costume map editor work

The `costume-map-editor` branch adds the first data layer for a build-your-own-map feature:

- Map elements for props, chairs, instant-death blocks, text messages, triggers, and animation events.
- Validation for IDs, element counts, text sizes, positions, rotations, and scales.
- A versioned, escaped `.sbamap` document format with encode/decode functions.

This is the foundation only. It does not yet show an editor in-game, spawn Unity objects, or save files to device storage. Those parts need a runtime UI and game-object adapter for the target game build. The target discussed so far is Super Bear Adventure 13.0.9.

See `src/costume_map.h` for the document API and `src/costume_map.cpp` for the format and validation rules.
