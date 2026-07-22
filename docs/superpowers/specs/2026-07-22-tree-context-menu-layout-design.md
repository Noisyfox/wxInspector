# Tree Context Menu — Layout Actions

Add a right-click context menu to the widget tree with **Layout** and **Layout Parent** actions.

## Motivation

The toolbar Layout button requires selecting an item first, then moving the mouse to the toolbar. A context menu lets the user right-click any tree item and trigger layout immediately — faster workflow, especially when already navigating the tree by mouse.

## Behavior

| Selection type | Layout | Layout Parent |
|---|---|---|
| `wxWindow` with parent | ✅ enabled | ✅ enabled — calls `parent->Layout()` |
| `wxWindow` top-level (no parent) | ✅ enabled | ❌ disabled (greyed out) |
| `wxSizer` | ✅ enabled | ❌ hidden |
| `wxSizerItem` | ✅ enabled | ❌ hidden |
| Nothing selected / click on empty area | Menu does not appear | — |

**Layout** reuses the existing `GetContainerWindow()` tree-walk — identical behavior to the toolbar button.

**Layout Parent** calls `GetContainerWindow()`, then `GetParent()` on the result, then `Layout()` on the parent. Disabled when `GetParent()` returns `nullptr` (top-level windows).

## Implementation

### Files changed

- [include/wx/inspector/tree.h](include/wx/inspector/tree.h) — declare `OnContextMenu()`, `OnLayout()`, `OnLayoutParent()`
- [src/tree.cpp](src/tree.cpp) — bind `wxEVT_CONTEXT_MENU`, build popup menu, implement handlers

### Context menu handler: `OnContextMenu(wxContextMenuEvent&)`

1. Get the tree item under the cursor via `m_tree->HitTest()`
2. If no item or item is root, return (no menu)
3. Select the item if not already selected
4. Read `ObjectTreeItemData` to determine `Kind`
5. Build a `wxMenu`:
   - Always add "Layout" (calls `OnLayout`)
   - If `Kind::Window`:
     - Add "Layout Parent" (calls `OnLayoutParent`)
     - Disable it if `GetContainerWindow()->GetParent()` is `nullptr`
6. `PopupMenu(&menu)`

### Layout handler: `OnLayout(wxCommandEvent&)`

Same logic as `InspectionFrame::OnLayout`:
```cpp
wxWindow* win = GetContainerWindow();
if (win) win->Layout();
```

### Layout Parent handler: `OnLayoutParent(wxCommandEvent&)`

```cpp
wxWindow* win = GetContainerWindow();
if (win) {
    wxWindow* parent = win->GetParent();
    if (parent) parent->Layout();
}
```

No new tree-walking logic — everything delegates to the existing `GetContainerWindow()`.

### Menu item IDs

New enum IDs in [tree.cpp](src/tree.cpp):
```cpp
ID_TREE_LAYOUT,
ID_TREE_LAYOUT_PARENT,
```

## Edge cases

| Case | Outcome |
|---|---|
| Right-click on root item "(root)" | No menu |
| Right-click on empty tree area | No menu |
| Right-click on a top-level window | Layout enabled, Layout Parent disabled |
| Right-click on sizer/sizer-item | Layout enabled, Layout Parent hidden |
| Selected object destroyed between menu open and click | `GetContainerWindow()` returns `nullptr`, silent no-op |

## What this does NOT include

- Keyboard shortcuts for context menu items
- Visual feedback after layout
- Any changes to the frame toolbar
