# Per-Mixin Inspector Ownership

Replace the global singleton inspector with per-mixin ownership, enabling inspection during modal dialogs.

## Motivation

The global singleton inspector (`g_inspectorFrame`) becomes unresponsive when a modal dialog is open because `wxWindowDisabler` disables all other top-level windows and the modal event loop only dispatches to the modal dialog's window hierarchy.

By making each `wxInspectorMixin` instance own its inspector frame, a modal dialog that inherits the mixin gets its own inspector — naturally part of the dialog's window tree — that remains fully interactive during the modal loop.

## Design

### Before (global singleton)

```
wxInspector::g_inspectorFrame  (one for the whole app)
wxInspector::Init() / Show() / Hide()  (static functions)
wxInspectorMixin  (just sets up Ctrl+Shift+I accelerator)
```

### After (per-mixin ownership)

```
wxInspectorMixin::m_inspectorFrame  (one per mixin instance)
mixin.ShowInspector() / HideInspector()  (instance methods)
wxInspector::RegisterPlugin()  (only remaining namespace function)
```

## Mixin API

```cpp
class wxInspectorMixin {
public:
    wxInspectorMixin();
    virtual ~wxInspectorMixin();  // now virtual — destroys inspector

    void ShowInspector(wxObject* selectObj = nullptr);
    void HideInspector();
    bool IsInspectorVisible() const;
    void RefreshInspectorTree();
    void SelectInspectorObject(wxObject* obj);

protected:
    void SetupInspectorAccelerator(wxWindow* window);  // Ctrl+Shift+I

private:
    InspectionFrame* m_inspectorFrame;  // created lazily, destroyed on dtor
    wxWindow* m_accelWindow;
    static const int ID_INSPECTOR_TOGGLE = wxID_HIGHEST + 9999;
    void OnToggleInspector(wxCommandEvent& event);
};
```

### Removed from `wxInspector` namespace

- `Init(wxConfigBase*)`
- `Shutdown()`
- `Show(wxObject*)`
- `Hide()`
- `RefreshTree()`
- `SelectObject(wxObject*)`
- `IsVisible()`

### Kept in `wxInspector` namespace

- `RegisterPlugin(wxInspectorPlugin*)`  — global, stateless, unchanged

## Integration

**Main app frame:**
```cpp
class MyApp : public wxApp, public wxInspectorMixin {
    bool OnInit() override {
        SetupInspectorAccelerator(myFrame);
        return true;
    }
};
```

**Modal dialog:**
```cpp
class SettingsDialog : public wxDialog, public wxInspectorMixin {
    SettingsDialog(wxWindow* parent)
        : wxDialog(parent, "Settings"), wxInspectorMixin()
    {
        SetupInspectorAccelerator(this);
        // ... create dialog UI ...
        ShowInspector();  // optional: auto-open on creation
    }
};
```

Non-modal child windows don't need their own mixin — the parent frame's inspector already sees them.

## How Modal Support Works

When a modal dialog inherits `wxInspectorMixin` and shows its inspector:
- `InspectionFrame` is created with the dialog as parent → naturally in the dialog's window tree
- The modal event loop dispatches to all children of the modal dialog
- The inspector inherits the parent's event processing — no special workarounds

## Lifecycle

| Event | Behavior |
|---|---|
| `SetupInspectorAccelerator(window)` | Registers Ctrl+Shift+I on `window` |
| `ShowInspector()` | Creates `InspectionFrame` lazily if needed, rebuilds tree, shows |
| `HideInspector()` | Hides the frame (does not destroy) |
| Close button on inspector | Frame hides (same as today), vetoes close |
| Parent window/dialog destroyed | `~wxInspectorMixin()` destroys `m_inspectorFrame` |
| Multiple mixin instances | Each owns its independent inspector — no interference |

## Files Changed

| File | Change |
|---|---|
| `include/wx/inspector/inspector.h` | Expand mixin with ownership API, shrink namespace to `RegisterPlugin` only |
| `src/inspector.cpp` | Remove `g_inspectorFrame` global, remove `Init`/`Show`/`Hide`/`Shutdown`, implement new mixin methods, keep `RegisterPlugin` |
| `include/wx/inspector/frame.h` | Remove dependency on global singleton concepts |
| `src/frame.cpp` | Adjust cleanup — emit close signal to mixin instead of relying on global state |
| `samples/minimal/minimal.cpp` | Update to new mixin-only API |
| `README.md` | Update Quick Start and API sections |
| `docs/design.md` | Update Public API and Mixin sections |

## Edge Cases

| Case | Outcome |
|---|---|
| No mixin on modal dialog | Inspector not available during that modal (same as before) |
| Mixin dtor called before inspector dtor | `delete m_inspectorFrame` — frame is explicitly destroyed |
| Frame closed by user (X button) | `Hide()` is called, frame persists in memory, `ShowInspector()` re-shows it |
| Ctrl+Shift+I with no parent window set | No-op (m_accelWindow is null) |
| `SetupInspectorAccelerator` called twice | Second call replaces the accelerator on the new window |

## What This Does NOT Change

- `InspectionFrame` internals — AUI layout, panels, toolbar, highlighting, tree, context menu
- `RegisterPlugin` — remains global and stateless
- The tree building algorithm — still enumerates `wxTopLevelWindows`
- Any panel code (tree, info, invoker, events, highlighter, property_provider, method_registry)
