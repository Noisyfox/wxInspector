# Per-Mixin Inspector Ownership — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the global singleton inspector with per-mixin ownership, renaming `wxInspectorMixin` to `wxInspectable`.

**Architecture:** Each `wxInspectable` instance owns an `InspectionFrame*` created lazily on first `ShowInspector()`. The global namespace `wxInspector` shrinks to `RegisterPlugin()` only. The inspector frame becomes a child of the mixin's parent window, enabling modal dialog support naturally.

**Tech Stack:** C++11, wxWidgets 3.2+

## Global Constraints

- wxWidgets 3.2.0+ required (project uses 3.3)
- C++11 minimum
- Class renamed: `wxInspectorMixin` → `wxInspectable`
- `wxInspector::RegisterPlugin()` is the only remaining namespace function
- Mixin owns `InspectionFrame*`, created lazily, destroyed on dtor
- `Ctrl+Shift+I` toggles per-mixin inspector
- Inspector frame hides on close (vetoes destroy), same as today
- No keyboard shortcut changes
- No visual feedback changes
- Doc files to update: README.md, docs/design.md

---

### Task 1: Refactor inspector.h and inspector.cpp — rename class, move API to mixin, remove globals

**Files:**
- Modify: `include/wx/inspector/inspector.h` — rename class, expand API, shrink namespace
- Modify: `src/inspector.cpp` — remove globals, implement new mixin methods, keep `RegisterPlugin`

**Interfaces:**
- Produces: `wxInspectable` class with `ShowInspector`, `HideInspector`, `IsInspectorVisible`, `RefreshInspectorTree`, `SelectInspectorObject`, `SetupInspectorAccelerator`
- Produces: `wxInspector::RegisterPlugin()` (unchanged, survives)
- Removes: `wxInspector::Init`, `wxInspector::Show`, `wxInspector::Hide`, `wxInspector::Shutdown`, `wxInspector::RefreshTree`, `wxInspector::SelectObject`, `wxInspector::IsVisible`, `g_inspectorFrame`

- [ ] **Step 1: Rewrite inspector.h**

Replace the contents of [include/wx/inspector/inspector.h](include/wx/inspector/inspector.h) with:

```cpp
#ifndef WX_INSPECTOR_INSPECTOR_H
#define WX_INSPECTOR_INSPECTOR_H

#include <wx/object.h>
#include <wx/app.h>
#include "wx/inspector/plugin.h"

class wxConfigBase;
class wxCommandEvent;

namespace wxInspector {

class InspectionFrame;

void RegisterPlugin(wxInspectorPlugin* plugin);

} // namespace wxInspector

class wxInspectable {
public:
    wxInspectable();
    virtual ~wxInspectable();

    wxInspectable(const wxInspectable&) = delete;
    wxInspectable& operator=(const wxInspectable&) = delete;

    void ShowInspector(wxObject* selectObj = nullptr);
    void HideInspector();
    bool IsInspectorVisible() const;
    void RefreshInspectorTree();
    void SelectInspectorObject(wxObject* obj);

protected:
    void SetupInspectorAccelerator(wxWindow* window);

private:
    static const int ID_INSPECTOR_TOGGLE = wxID_HIGHEST + 9999;
    void OnToggleInspector(wxCommandEvent& event);

    wxInspector::InspectionFrame* m_inspectorFrame;
    wxWindow* m_accelWindow;
};

#endif // WX_INSPECTOR_INSPECTOR_H
```

- [ ] **Step 2: Rewrite inspector.cpp**

Replace the contents of [src/inspector.cpp](src/inspector.cpp) with:

```cpp
#include "wx/inspector/inspector.h"
#include "wx/inspector/frame.h"
#include "wx/inspector/object.h"
#include <wx/accel.h>
#include <wx/config.h>

namespace wxInspector {

void RegisterPlugin(wxInspectorPlugin* plugin)
{
    // Plugin registry — keep existing implementation from current inspector.cpp
    // ... (unchanged)
}

} // namespace wxInspector

wxInspectable::wxInspectable()
    : m_inspectorFrame(nullptr)
    , m_accelWindow(nullptr)
{
}

wxInspectable::~wxInspectable()
{
    delete m_inspectorFrame;
}

void wxInspectable::SetupInspectorAccelerator(wxWindow* window)
{
    m_accelWindow = window;
    if (!window)
        return;

    wxAcceleratorEntry entries[] = {
        { wxACCEL_CTRL | wxACCEL_SHIFT, 'I', ID_INSPECTOR_TOGGLE },
    };
    wxAcceleratorTable accel(sizeof(entries) / sizeof(entries[0]), entries);
    window->SetAcceleratorTable(accel);
    window->Bind(wxEVT_MENU, &wxInspectable::OnToggleInspector, this, ID_INSPECTOR_TOGGLE);
}

void wxInspectable::OnToggleInspector(wxCommandEvent&)
{
    if (IsInspectorVisible())
        HideInspector();
    else
        ShowInspector();
}

void wxInspectable::ShowInspector(wxObject* selectObj)
{
    if (!m_inspectorFrame)
    {
        // Find a parent window for the inspector frame
        wxWindow* parent = m_accelWindow;
        m_inspectorFrame = new wxInspector::InspectionFrame(
            parent, wxDefaultPosition, wxSize(800, 600));
    }
    m_inspectorFrame->Show(selectObj);
}

void wxInspectable::HideInspector()
{
    if (m_inspectorFrame)
        m_inspectorFrame->Hide();
}

bool wxInspectable::IsInspectorVisible() const
{
    return m_inspectorFrame && m_inspectorFrame->IsVisible();
}

void wxInspectable::RefreshInspectorTree()
{
    if (m_inspectorFrame)
        m_inspectorFrame->RefreshTree();
}

void wxInspectable::SelectInspectorObject(wxObject* obj)
{
    if (m_inspectorFrame)
        m_inspectorFrame->SelectObject(obj);
}
```

The `RegisterPlugin` implementation should preserve the existing plugin registration logic from the current [src/inspector.cpp](src/inspector.cpp).

- [ ] **Step 3: Build to verify compilation**

Run:
```
cmake --build build --config RelWithDebInfo
```
Expected: Build succeeds with no errors.

- [ ] **Step 4: Commit**

```bash
git add include/wx/inspector/inspector.h src/inspector.cpp
git commit -m "refactor: rename wxInspectorMixin to wxInspectable, move API from namespace to mixin

- Rename class wxInspectorMixin -> wxInspectable
- Move Init/Show/Hide/RefreshTree/SelectObject/IsVisible from wxInspector namespace to mixin instance methods
- Remove g_inspectorFrame global singleton
- Remove wxInspector::Shutdown()
- Keep wxInspector::RegisterPlugin() as the only namespace function
- Mixin owns InspectionFrame*, created lazily on first Show

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 2: Update frame.h and frame.cpp — decouple from global singleton

**Files:**
- Modify: `include/wx/inspector/frame.h` — remove global singleton dependencies
- Modify: `src/frame.cpp` — adjust cleanup logic for per-mixin ownership

**Interfaces:**
- Consumes: `wxInspectable` class, `wxInspector::InspectionFrame*` (from Task 1)

- [ ] **Step 1: Review frame.h for any global singleton references**

Read [include/wx/inspector/frame.h](include/wx/inspector/frame.h). Check for any `friend` declarations referencing the old global API or `g_inspectorFrame`. If the `InspectionFrame` class has no `friend` declarations tied to the old singleton, no changes needed. The `InspectionFrame` interface (`Show`, `Hide`, `RefreshTree`, `SelectObject`, `SaveLayout`, `LoadLayout`) is already instance-based and doesn't need changes.

Expected: No changes to frame.h are needed unless there are `friend` references to the old global functions.

- [ ] **Step 2: Review frame.cpp for any global singleton dependencies**

Read [src/frame.cpp](src/frame.cpp). Check for:
- References to `g_inspectorFrame` — none expected since it was in inspector.cpp
- Any calls to the old `wxInspector::Init()` / `wxInspector::Shutdown()` — none expected
- `OnClose` handler: ensure it only hides + vetoes (it already does), no global state changes needed

If no global dependencies found, no changes to frame.cpp are needed. The existing `InspectionFrame` already operates independently — it was always created and managed externally.

- [ ] **Step 3: If frame.cpp needs changes — adjust shutdown flow**

The `InspectionFrame` destructor already calls `m_auiMgr.UnInit()`. Since `wxInspectable` now owns the frame via `delete m_inspectorFrame`, the existing destructor is sufficient. Confirm that `InspectionFrame::~InspectionFrame()` in [src/frame.cpp](src/frame.cpp) (line 75-78) only calls `m_auiMgr.UnInit()` and nothing that depends on the global singleton.

- [ ] **Step 4: Build to verify compilation**

Run:
```
cmake --build build --config RelWithDebInfo
```
Expected: Build succeeds with no errors.

- [ ] **Step 5: Commit** (only if changes were made; otherwise skip)

```bash
git add include/wx/inspector/frame.h src/frame.cpp
git commit -m "refactor: decouple InspectionFrame from global singleton

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

### Task 3: Update sample app and documentation

**Files:**
- Modify: `samples/minimal/minimal.cpp` — update to new API
- Modify: `README.md` — update Quick Start and API sections
- Modify: `docs/design.md` — update Public API and Mixin sections

**Interfaces:**
- Consumes: `wxInspectable` class with `SetupInspectorAccelerator`, `ShowInspector` (from Task 1)

- [ ] **Step 1: Update minimal.cpp**

Read [samples/minimal/minimal.cpp](samples/minimal/minimal.cpp). Replace all references:

Old:
```cpp
#include <wx/inspector/inspector.h>

class MyApp : public wxApp, public wxInspectorMixin {
    bool OnInit() override {
        wxInspector::Init();
        // ... create UI ...
        wxInspector::Show();
        return true;
    }
};
```

New:
```cpp
#include <wx/inspector/inspector.h>

class MyApp : public wxApp, public wxInspectable {
    bool OnInit() override {
        SetupInspectorAccelerator(frame);
        // ... create UI ...
        ShowInspector();  // optional: auto-open on startup
        return true;
    }
};
```

Make the minimal edits to align with the actual current content of minimal.cpp — change `wxInspectorMixin` to `wxInspectable`, replace `wxInspector::Init()` and `wxInspector::Show()` calls with the new API, and ensure `SetupInspectorAccelerator` is called on the main frame.

- [ ] **Step 2: Build sample app to verify**

Run:
```
cmake --build build --config RelWithDebInfo
```
Expected: `minimal.exe` builds and links successfully.

- [ ] **Step 3: Update README.md**

In [README.md](README.md), replace the Quick Start section:

Old:
```cpp
#include <wx/inspector/inspector.h>

class MyApp : public wxApp, public wxInspectorMixin {
    bool OnInit() override {
        wxInspector::Init();
        // ... create your UI ...
        wxInspector::Show();  // or press Ctrl+Shift+I
        return true;
    }
};
wxIMPLEMENT_APP(MyApp);
```

New:
```cpp
#include <wx/inspector/inspector.h>

class MyApp : public wxApp, public wxInspectable {
    bool OnInit() override {
        SetupInspectorAccelerator(myFrame);
        // ... create your UI ...
        ShowInspector();  // optional, or press Ctrl+Shift+I
        return true;
    }
};
wxIMPLEMENT_APP(MyApp);
```

Also add a modal dialog example:

```cpp
class SettingsDialog : public wxDialog, public wxInspectable {
    SettingsDialog(wxWindow* parent)
        : wxDialog(parent, "Settings"), wxInspectable()
    {
        SetupInspectorAccelerator(this);
        // ... create dialog UI ...
    }
};
```

- [ ] **Step 4: Update docs/design.md**

In [docs/design.md](docs/design.md):

- Replace all occurrences of `wxInspectorMixin` with `wxInspectable`
- In the "Public API" section, replace the old global API with the new mixin API:
  - Remove: `wxInspector::Init(wxPoint, wxSize, wxConfig*)`, `wxInspector::Show()`, `wxInspector::Hide()`, `wxInspector::RefreshTree()`, `wxInspector::SelectObject()`, `wxInspector::IsVisible()`
  - Add: mixin methods `ShowInspector()`, `HideInspector()`, `IsInspectorVisible()`, `RefreshInspectorTree()`, `SelectInspectorObject()`
  - Keep: `wxInspector::RegisterPlugin()`

- In the "Mixin Class" section, rename to "wxInspectable" and show the updated integration pattern
- In "Minimal Integration", show the new mixin-only API
- In "Programmatic API", replace the namespace functions with mixin methods

- [ ] **Step 5: Commit**

```bash
git add samples/minimal/minimal.cpp README.md docs/design.md
git commit -m "docs: update sample and docs for wxInspectable per-mixin API

Co-Authored-By: Claude <noreply@anthropic.com>"
```
