# SBA-Realistic-Physic

Native SBAModLoader mod project for Super Bear Adventure 13.0.9.

## Costume Map Maker

The `costume-map-editor` branch builds an `.sbapack` with an in-game grid editor. Open **MAP EDITOR**, choose an item type, and tap grid cells to place items. Select an item and tap an empty cell to move it. Use **EDIT** to set size, height, rotation, message text, event keys, and animation keys. Save or load maps from the editor.

Maps are stored at the game's app-specific `CostumeMaps/map1.sbamap` path. The native adapter loads the saved map on the next game launch and creates Unity objects for props, chairs, death blocks, triggers, messages, and animation actions. Red death blocks use a player trigger hook. Green triggers can reveal linked messages and play the selected animation through SBA's animation controller.

## Current limits

Chair objects are visual furniture; sitting and getting up are not implemented yet. Animation names are entered manually and may not exist in SBA 13.0.9. The mod builds successfully, but gameplay behavior still needs testing in the game on a device.

The map format includes validation, versioned serialization, selection, move/delete, grid placement, undo, and local save/load. GitHub Actions builds the arm64 native library, compiles the editor activity, and packages both as an SBAModLoader `.sbapack`.

Download the **SBA-Costume-Map-Maker** artifact from the latest successful Actions run on the `costume-map-editor` branch. The artifact is not an APK.
