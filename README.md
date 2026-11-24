# SBA-Realistic-Physic

Native SBAModLoader mod project for Super Bear Adventure 13.0.9.

## Costume Map Maker

The `costume-map-editor` branch now builds an `.sbapack` with an in-game grid editor. Open **MAP EDITOR**, choose an item type, tap grid cells to place it, then save or load the map. Maps are stored at the game's app-specific `CostumeMaps/map1.sbamap` path. On the next launch, the native adapter creates Unity objects from the saved map.

The current runtime creates cube props, decorative chair shapes, red death-block markers, and floating text labels. Chair sitting, actual instant-death behavior, trigger actions, and animation events are not implemented yet. Those need game-specific behavior and still need implementation and device testing.

The map data layer includes validation, versioned serialization, selection/edit operations, grid snapping, undo/redo, and local file save/load. The GitHub Actions workflow builds the native library, compiles the editor activity, and packages both into an SBAModLoader `.sbapack`.

Use the **SBA-Costume-Map-Maker** artifact from the branch's latest successful Actions run to get the pack. It is not an APK.
