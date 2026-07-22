# View Menu Panel Toggles Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add View menu check items to toggle visibility of the four AUI dockable panels, and move Expand All/Collapse All out of the View menu (they stay on the tree toolbar).

**Architecture:** Direct AUI management in `InspectionFrame::SetupMenuBar()`. Add four `AppendCheckItem` calls to the View menu, bind `wxEVT_MENU` handlers that call `m_auiMgr.GetPane(name).Show(!current)` + `m_auiMgr.Update()`, and bind `wxEVT_UPDATE_UI` handlers to keep checkmarks in sync with actual pane visibility.

**Tech Stack:** C++11, wxWidgets 3.3, wxAUI

## Global Constraints

- wxWidgets 3.3 minimum
- C++11
- No new files, no header changes — only `src/frame.cpp` is modified
- Follow existing code patterns: enum for IDs, lambdas for menu binds, `wxAuiManager::GetPane()` for pane access

---

### Task 1: Add panel toggle items to View menu

**Files:**
- Modify: `src/frame.cpp:17-26` (enum), `src/frame.cpp:87-90` (View menu section), add new binds after line 121

**Interfaces:**
- Consumes: existing `m_auiMgr`, pane names `"Tree"`, `"Info"`, `"Invoker"`, `"Events"` from `SetupAUI()`
- Produces: four new ID constants (`ID_VIEW_TREE`, `ID_VIEW_INFO`, `ID_VIEW_INVOKER`, `ID_VIEW_EVENTS`)

#### Step 1: Add four new ID constants to the enum

In `src/frame.cpp`, the enum block at lines 17-27. Add a trailing comma to `ID_QUIT` and add four new IDs after it:

```cpp
    ID_QUIT,
    ID_VIEW_TREE,
    ID_VIEW_INFO,
    ID_VIEW_INVOKER,
    ID_VIEW_EVENTS,
```

#### Step 2: Replace the View menu section in `SetupMenuBar()`

In `src/frame.cpp`, lines 87-90. Replace:

```cpp
    wxMenu* viewMenu = new wxMenu();
    viewMenu->Append(ID_EXPAND_ALL, "&Expand All\tF4");
    viewMenu->Append(ID_COLLAPSE_ALL, "&Collapse All\tF5");
    mb->Append(viewMenu, "&View");
```

With:

```cpp
    wxMenu* viewMenu = new wxMenu();
    viewMenu->AppendCheckItem(ID_VIEW_TREE, "&Widget Tree");
    viewMenu->AppendCheckItem(ID_VIEW_INFO, "&Object Info");
    viewMenu->AppendCheckItem(ID_VIEW_INVOKER, "M&ethod Invoker");
    viewMenu->AppendCheckItem(ID_VIEW_EVENTS, "&Event Logger");
    mb->Append(viewMenu, "&View");
```

#### Step 3: Add toggle handler binds

In `SetupMenuBar()`, add after the existing `Bind(...)` calls (after the `ID_ABOUT` bind block, currently ending around line 121). Add:

```cpp
    // View menu — panel toggles
    auto togglePane = [this](const wxString& name) {
        wxAuiPaneInfo& pane = m_auiMgr.GetPane(name);
        pane.Show(!pane.IsShown());
        m_auiMgr.Update();
    };
    Bind(wxEVT_MENU, [togglePane](wxCommandEvent&) { togglePane("Tree"); }, ID_VIEW_TREE);
    Bind(wxEVT_MENU, [togglePane](wxCommandEvent&) { togglePane("Info"); }, ID_VIEW_INFO);
    Bind(wxEVT_MENU, [togglePane](wxCommandEvent&) { togglePane("Invoker"); }, ID_VIEW_INVOKER);
    Bind(wxEVT_MENU, [togglePane](wxCommandEvent&) { togglePane("Events"); }, ID_VIEW_EVENTS);
```

#### Step 4: Add update UI binds to keep checkmarks in sync

Add after the toggle binds from Step 3:

```cpp
    Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) {
        e.Check(m_auiMgr.GetPane("Tree").IsShown());
    }, ID_VIEW_TREE);
    Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) {
        e.Check(m_auiMgr.GetPane("Info").IsShown());
    }, ID_VIEW_INFO);
    Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) {
        e.Check(m_auiMgr.GetPane("Invoker").IsShown());
    }, ID_VIEW_INVOKER);
    Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) {
        e.Check(m_auiMgr.GetPane("Events").IsShown());
    }, ID_VIEW_EVENTS);
```

#### Step 5: Build and verify compilation

```bash
cd build && cmake --build . --config Debug
```

Expected: clean compile, no errors.

#### Step 6: Manual smoke test

Build and run the sample app. Perform the spec's test checklist:

1. Open wxInspector — confirm all four panels are visible and their View menu items are checked.
2. Close the Widget Tree via its caption X. Confirm View → Widget Tree becomes unchecked.
3. Click View → Widget Tree. Confirm the tree panel reappears in its last position.
4. Repeat for Object Info, Method Invoker, Event Logger.
5. Confirm F4 / F5 still expand and collapse the tree (accelerator table is untouched).
6. Confirm Ctrl+I still closes (hides) the inspector frame.

#### Step 7: Commit

```bash
git add src/frame.cpp
git commit -m "feat: add View menu panel toggles for AUI dockable panels"
```
