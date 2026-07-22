# Layout Toolbar Button

Add a **Layout** toolbar button to the frame-level toolbar that triggers `Layout()` on the appropriate window for the current tree selection.

## Motivation

Developers frequently tweak widget properties (size, label, font, etc.) in the inspector and need to force a layout recalculation to see the result. Currently they must use the Method Invoker to call `Layout()` manually. A one-click toolbar button makes this instant.

## Behavior

| Selection type | Action |
|---|---|
| `wxWindow` | `window->Layout()` |
| `wxSizer` | Walk up tree to nearest ancestor window, call `Layout()` |
| `wxSizerItem` (window, sub-sizer, or spacer) | Walk up tree to nearest ancestor window, call `Layout()` |
| Nothing selected | Silent no-op |

No visual feedback. No keyboard shortcut (room for F7 later).

## Implementation

### Files changed

- [include/wx/inspector/tree.h](include/wx/inspector/tree.h) — declare `GetContainerWindow()`
- [src/tree.cpp](src/tree.cpp) — implement `GetContainerWindow()`
- [src/frame.cpp](src/frame.cpp) — `ID_LAYOUT` enum, toolbar button, menu binding, `OnLayout` handler

### New method: `InspectionTree::GetContainerWindow()`

```cpp
wxWindow* InspectionTree::GetContainerWindow() const;
```

Algorithm:
1. Get the currently selected tree item via `m_tree->GetSelection()`
2. If not valid, return `nullptr`
3. Read the `ObjectTreeItemData` to get the stored `m_kind` and `m_object`
4. If `m_kind == Kind::Window`, return `static_cast<wxWindow*>(m_object)`
5. Otherwise, loop: set item = `m_tree->GetItemParent(item)` until the item's data has `m_kind == Kind::Window`, then return that `wxWindow*`
6. If the root is reached without finding a window, return `nullptr`

### New handler: `InspectionFrame::OnLayout()`

```cpp
void InspectionFrame::OnLayout(wxCommandEvent&)
{
    wxWindow* win = m_tree->GetContainerWindow();
    if (win)
        win->Layout();
}
```

### Toolbar button

Added in `SetupToolBar()` after the Highlight button:

```cpp
tb->AddTool(ID_LAYOUT, "Layout",
            wxArtProvider::GetBitmap(wxART_EXECUTABLE), "Layout");
```

### Menu binding

Added in `SetupMenuBar()`:

```cpp
Bind(wxEVT_MENU, &InspectionFrame::OnLayout, this, ID_LAYOUT);
```

### New enum ID

```cpp
ID_LAYOUT,
```

in the existing `enum` block in [src/frame.cpp](src/frame.cpp).

## Edge cases

| Case | Outcome |
|---|---|
| Nothing selected | `GetContainerWindow()` returns `nullptr`; silent no-op |
| Sizer is nested N levels deep | Walk-up handles arbitrary depth |
| Spacer sizer item selected | Walks up through sizer-item → sizer → window |
| Window destroyed after selection | `Layout()` on a stale but still-valid pointer is harmless; tree is rebuilt on next Show() anyway |

## What this does NOT include

- Keyboard shortcut (left for future — F7 is available)
- Visual highlight feedback
- Any changes to the tree panel's internal toolbar
