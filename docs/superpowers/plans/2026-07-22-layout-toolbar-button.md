# Layout Toolbar Button Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a Layout toolbar button that calls `Layout()` on the nearest ancestor window of the current tree selection.

**Architecture:** A new `GetContainerWindow()` method on `InspectionTree` walks up the tree from the selected item to find the nearest `wxWindow` ancestor. A new `OnLayout` handler in `InspectionFrame` calls it and invokes `Layout()`. The button lives on the frame toolbar alongside Highlight.

**Tech Stack:** C++11, wxWidgets 3.2+

## Global Constraints

- wxWidgets 3.2.0+ required
- C++11 minimum
- No keyboard shortcut for the Layout action
- No visual feedback after Layout executes
- Silent no-op when nothing is selected

---

### Task 1: Add `GetContainerWindow()` to InspectionTree

**Files:**
- Modify: `include/wx/inspector/tree.h` — add method declaration
- Modify: `src/tree.cpp` — add method implementation

**Interfaces:**
- Produces: `wxWindow* InspectionTree::GetContainerWindow() const` — returns the nearest ancestor `wxWindow*` for the current selection, or `nullptr` if nothing selected or no window ancestor found

- [ ] **Step 1: Declare `GetContainerWindow()` in tree.h**

In [include/wx/inspector/tree.h](include/wx/inspector/tree.h), add the declaration after `GetSelectedObject()` (line 23):

```cpp
wxWindow* GetContainerWindow() const;
```

Insert it on line 24, after `InspectableObject GetSelectedObject() const;`:

```cpp
InspectableObject GetSelectedObject() const;
wxWindow* GetContainerWindow() const;
wxTreeCtrl* GetTreeCtrl() { return m_tree; }
```

- [ ] **Step 2: Implement `GetContainerWindow()` in tree.cpp**

In [src/tree.cpp](src/tree.cpp), add the implementation after `GetSelectedObject()` (after line 270). Insert:

```cpp
wxWindow* InspectionTree::GetContainerWindow() const
{
    wxTreeItemId item = m_tree->GetSelection();
    if (!item.IsOk())
        return nullptr;

    // Walk up from the selected item to find the nearest window ancestor
    while (item.IsOk())
    {
        ObjectTreeItemData* d = dynamic_cast<ObjectTreeItemData*>(
            m_tree->GetItemData(item));
        if (d && d->m_object && d->m_kind == InspectableObject::Kind::Window)
        {
            return static_cast<wxWindow*>(d->m_object);
        }
        item = m_tree->GetItemParent(item);
    }
    return nullptr;
}
```

- [ ] **Step 3: Build to verify compilation**

Run:
```
cmake --build build --config RelWithDebInfo
```
Expected: Build succeeds with no errors.

- [ ] **Step 4: Commit**

```bash
git add include/wx/inspector/tree.h src/tree.cpp
git commit -m "feat: add GetContainerWindow() to InspectionTree

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 2: Add Layout toolbar button and handler to InspectionFrame

**Files:**
- Modify: `include/wx/inspector/frame.h` — declare `OnLayout` handler
- Modify: `src/frame.cpp` — add `ID_LAYOUT`, toolbar button, menu binding, handler implementation

**Interfaces:**
- Consumes: `wxWindow* InspectionTree::GetContainerWindow() const` (from Task 1)

- [ ] **Step 1: Declare `OnLayout` in frame.h**

In [include/wx/inspector/frame.h](include/wx/inspector/frame.h), add the declaration after `OnFindWidget` (line 40):

```cpp
void OnFindWidget(wxCommandEvent& event);
void OnLayout(wxCommandEvent& event);
void OnClose(wxCloseEvent& event);
```

- [ ] **Step 2: Add `ID_LAYOUT` to the enum in frame.cpp**

In [src/frame.cpp](src/frame.cpp), add `ID_LAYOUT` to the enum block (line 20-29):

```cpp
enum {
    ID_HIGHLIGHT = wxID_HIGHEST + 400,
    ID_FIND_WIDGET,
    ID_REFRESH_TREE,
    ID_TOGGLE_SIZERS,
    ID_EXPAND_ALL,
    ID_COLLAPSE_ALL,
    ID_LAYOUT,
    ID_ABOUT,
    ID_QUIT
};
```

- [ ] **Step 3: Add toolbar button in `SetupToolBar()`**

In [src/frame.cpp](src/frame.cpp), in `SetupToolBar()` after the Highlight button (after line 138), add:

```cpp
tb->AddTool(ID_LAYOUT, "Layout",
            wxArtProvider::GetBitmap(wxART_EXECUTABLE), "Layout");
```

- [ ] **Step 4: Add menu binding in `SetupMenuBar()`**

In [src/frame.cpp](src/frame.cpp), in `SetupMenuBar()` after the Highlight binding (after line 116), add:

```cpp
Bind(wxEVT_MENU, &InspectionFrame::OnLayout, this, ID_LAYOUT);
```

- [ ] **Step 5: Implement `OnLayout` handler in frame.cpp**

In [src/frame.cpp](src/frame.cpp), add after `OnFindWidget` (after line 240):

```cpp
void InspectionFrame::OnLayout(wxCommandEvent&)
{
    wxWindow* win = m_tree->GetContainerWindow();
    if (win)
        win->Layout();
}
```

- [ ] **Step 6: Build to verify compilation**

Run:
```
cmake --build build --config RelWithDebInfo
```
Expected: Build succeeds with no errors.

- [ ] **Step 7: Manual smoke test with the sample app**

Run the sample app and verify:
```
build\samples\minimal\RelWithDebInfo\minimal.exe
```
- Open wxInspector (Ctrl+Shift+I)
- Select a widget in the tree → click Layout → no crash, widget re-lays out
- Select a sizer (enable Sizers first) → click Layout → no crash
- Deselect everything → click Layout → silent no-op, no crash

- [ ] **Step 8: Commit**

```bash
git add include/wx/inspector/frame.h src/frame.cpp
git commit -m "feat: add Layout toolbar button to InspectionFrame

Co-Authored-By: Claude <noreply@anthropic.com>"
```
