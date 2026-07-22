# View Menu Panel Toggles Design

**Date:** 2026-07-22
**Status:** Approved

## Problem

When a user closes an AUI dockable panel (Tree, Info, Invoker, Events) via its caption X button, there is no way to bring it back. The panels are hidden but still managed by `wxAuiManager` — they just need a UI affordance to re-show them.

## Design

### Menu restructuring

The current View menu contains only tree-specific commands (Expand All / Collapse All). Those move to the tree panel's own toolbar, which already has buttons for both. The View menu becomes the panel visibility control point.

### View menu items

Four check items added to the View menu:

| Pane name | Menu label | ID constant |
|-----------|-----------|-------------|
| `"Tree"` | `&Widget Tree` | `ID_VIEW_TREE` |
| `"Info"` | `&Object Info` | `ID_VIEW_INFO` |
| `"Invoker"` | `M&ethod Invoker` | `ID_VIEW_INVOKER` |
| `"Events"` | `&Event Logger` | `ID_VIEW_EVENTS` |

No keyboard accelerators for these items — they supplement mouse-driven caption-close, not replace a frequent keyboard workflow.

### Toggle behavior

Each item is bound to a handler that flips the corresponding AUI pane's visibility:

1. `m_auiMgr.GetPane(name).IsShown()` to read current state
2. `pane.Show(!current)` to flip
3. `m_auiMgr.Update()` to apply

Check state is kept in sync via `wxEVT_UPDATE_UI` handlers, which are evaluated by wxWidgets before the menu is shown. No manual state tracking needed.

### F4/F5 accelerators

Expand All (F4) and Collapse All (F5) are wired through `wxAcceleratorTable` on the frame, not through menu items. Removing them from the View menu has no effect on the shortcuts. The tree toolbar retains both buttons.

### What does NOT change

- Panel constructors, AUI pane setup, and `SetupAUI()` — unchanged.
- Frame event table, accelerator table — unchanged.
- All other menus (File, Tools, Help) — unchanged.
- The `InspectionFrame` still owns all four panels for their lifetime; they are never destroyed.

## Files changed

Only `src/frame.cpp` — no header changes, no new files.

- `SetupMenuBar()`: remove the two `viewMenu->Append` lines for Expand All / Collapse All; add four check items; add `Bind(wxEVT_MENU, ...)` for toggling and `Bind(wxEVT_UPDATE_UI, ...)` for check state.

## Testing

Manual smoke test:
1. Open wxInspector, confirm all four panels are visible and their View menu items are checked.
2. Close the Widget Tree via its caption X. Confirm View → Widget Tree becomes unchecked.
3. Click View → Widget Tree. Confirm the tree panel reappears in its last position.
4. Repeat for Object Info, Method Invoker, Event Logger.
5. Confirm F4 / F5 still expand and collapse the tree.
6. Confirm Ctrl+I still closes (hides) the inspector frame.
