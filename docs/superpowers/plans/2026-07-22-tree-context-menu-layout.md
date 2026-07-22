# Tree Context Menu Layout Actions — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a right-click context menu to the widget tree with Layout and Layout Parent actions.

**Architecture:** Bind `wxEVT_TREE_ITEM_MENU` on the tree control. The context menu handler builds a popup `wxMenu` based on the selected item's kind. Both actions delegate to the existing `GetContainerWindow()` method. Layout Parent additionally calls `GetParent()` on the result.

**Tech Stack:** C++11, wxWidgets 3.2+

## Global Constraints

- wxWidgets 3.2.0+ required (project uses 3.3)
- C++11 minimum
- No keyboard shortcuts for context menu items
- No visual feedback after layout
- Layout Parent hidden for non-window selections
- Layout Parent disabled for top-level windows (no parent)
- No menu when right-clicking empty area or root item

---

### Task 1: Add context menu with Layout and Layout Parent

**Files:**
- Modify: `include/wx/inspector/tree.h` — declare `OnContextMenu`, `OnLayout`, `OnLayoutParent`
- Modify: `src/tree.cpp` — new enum IDs, event table entry, context menu handler, two action handlers

**Interfaces:**
- Consumes: `wxWindow* InspectionTree::GetContainerWindow() const` (already exists)
- Produces: `void OnContextMenu(wxTreeEvent&)`, `void OnLayout(wxCommandEvent&)`, `void OnLayoutParent(wxCommandEvent&)`

- [ ] **Step 1: Add enum IDs in tree.cpp**

In [src/tree.cpp](src/tree.cpp), add two new IDs to the existing enum block (after `ID_TREE_FIND_WIDGET`):

```cpp
enum {
    ID_TREE_REFRESH = wxID_HIGHEST + 100,
    ID_TREE_TOGGLE_SIZERS,
    ID_TREE_EXPAND_ALL,
    ID_TREE_COLLAPSE_ALL,
    ID_TREE_FIND_WIDGET,
    ID_TREE_LAYOUT,
    ID_TREE_LAYOUT_PARENT
};
```

- [ ] **Step 2: Add event table entry in tree.cpp**

In [src/tree.cpp](src/tree.cpp), add `EVT_TREE_ITEM_MENU` to the event table (after `EVT_TREE_SEL_CHANGED`):

```cpp
wxBEGIN_EVENT_TABLE(InspectionTree, wxPanel)
    EVT_TREE_SEL_CHANGED(wxID_ANY, InspectionTree::OnTreeSelChanged)
    EVT_TREE_ITEM_MENU(wxID_ANY, InspectionTree::OnContextMenu)
wxEND_EVENT_TABLE()
```

- [ ] **Step 3: Add method declarations in tree.h**

In [include/wx/inspector/tree.h](include/wx/inspector/tree.h), add three declarations in the `private:` section, after `OnKeyDown` (line 35):

```cpp
void OnKeyDown(wxKeyEvent& event);
void OnContextMenu(wxTreeEvent& event);
void OnLayout(wxCommandEvent& event);
void OnLayoutParent(wxCommandEvent& event);
```

- [ ] **Step 4: Implement `OnContextMenu` in tree.cpp**

In [src/tree.cpp](src/tree.cpp), add after `OnKeyDown` (after line 331):

```cpp
void InspectionTree::OnContextMenu(wxTreeEvent& event)
{
    wxTreeItemId item = event.GetItem();
    if (!item.IsOk() || item == m_tree->GetRootItem())
        return;

    // Select the right-clicked item
    m_tree->SelectItem(item);

    ObjectTreeItemData* d = dynamic_cast<ObjectTreeItemData*>(
        m_tree->GetItemData(item));
    if (!d || !d->m_object)
        return;

    bool isWindow = (d->m_kind == InspectableObject::Kind::Window);

    wxMenu menu;

    menu.Append(ID_TREE_LAYOUT, "Layout");

    if (isWindow)
    {
        wxWindow* container = GetContainerWindow();
        wxWindow* parent = container ? container->GetParent() : nullptr;
        wxMenuItem* layoutParentItem = menu.Append(ID_TREE_LAYOUT_PARENT, "Layout Parent");
        if (!parent)
            layoutParentItem->Enable(false);
    }

    PopupMenu(&menu);
}
```

- [ ] **Step 5: Implement `OnLayout` in tree.cpp**

In [src/tree.cpp](src/tree.cpp), add after `OnContextMenu`:

```cpp
void InspectionTree::OnLayout(wxCommandEvent&)
{
    wxWindow* win = GetContainerWindow();
    if (win)
        win->Layout();
}
```

- [ ] **Step 6: Implement `OnLayoutParent` in tree.cpp**

In [src/tree.cpp](src/tree.cpp), add after `OnLayout`:

```cpp
void InspectionTree::OnLayoutParent(wxCommandEvent&)
{
    wxWindow* win = GetContainerWindow();
    if (win)
    {
        wxWindow* parent = win->GetParent();
        if (parent)
            parent->Layout();
    }
}
```

- [ ] **Step 7: Bind menu item events in InspectionTree constructor**

In [src/tree.cpp](src/tree.cpp), in the `InspectionTree` constructor (after the existing `Bind` calls around line 66), add:

```cpp
Bind(wxEVT_MENU, &InspectionTree::OnLayout, this, ID_TREE_LAYOUT);
Bind(wxEVT_MENU, &InspectionTree::OnLayoutParent, this, ID_TREE_LAYOUT_PARENT);
```

- [ ] **Step 8: Build to verify compilation**

Run:
```
cmake --build build --config RelWithDebInfo
```
Expected: Build succeeds with no errors.

- [ ] **Step 9: Commit**

```bash
git add include/wx/inspector/tree.h src/tree.cpp
git commit -m "feat: add Layout and Layout Parent to tree context menu

Co-Authored-By: Claude <noreply@anthropic.com>"
```
