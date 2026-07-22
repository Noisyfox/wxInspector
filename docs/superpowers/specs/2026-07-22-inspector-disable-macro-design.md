# Compile-Time Inspector Disable via `WXINSPECTOR_DISABLE`

Allow users to compile out the entire inspector system with a single `#define`, no library rebuild required.

## Motivation

Users want zero-overhead inspector removal in release builds. The `wxInspectable` mixin and `RegisterPlugin` should compile to nothing when `WXINSPECTOR_DISABLE` is defined, with no changes to integration code.

## Design

The real implementation class is renamed to `wxInspectableImpl` and always compiled into the library. The public name `wxInspectable` becomes:

- **Enabled (default):** a `using` alias for `wxInspectableImpl`
- **Disabled (`-DWXINSPECTOR_DISABLE`):** a header-only dummy with the same interface, all methods empty inlines

Same pattern for `RegisterPlugin`:
- **Enabled:** inline wrapper calls `detail::RegisterPluginImpl()` in the library
- **Disabled:** inline no-op

No library rebuild needed either way. The real code always ships in `wxInspector.lib`.

## Header structure

```cpp
// include/wx/inspector/inspector.h

namespace wxInspector {

// --- Registration ---

#ifdef WXINSPECTOR_DISABLE
    inline void RegisterPlugin(wxInspectorPlugin*) {}
#else
    namespace detail {
        void RegisterPluginImpl(wxInspectorPlugin* plugin);
    }
    inline void RegisterPlugin(wxInspectorPlugin* plugin) {
        detail::RegisterPluginImpl(plugin);
    }
#endif

} // namespace wxInspector

// --- Mixin ---

#ifndef WXINSPECTOR_DISABLE
class wxInspectableImpl {
public:
    wxInspectableImpl();

    wxInspectableImpl(const wxInspectableImpl&) = delete;
    wxInspectableImpl& operator=(const wxInspectableImpl&) = delete;

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
#endif

#ifdef WXINSPECTOR_DISABLE
class wxInspectable {
public:
    void ShowInspector(wxObject* = nullptr) {}
    void HideInspector() {}
    bool IsInspectorVisible() const { return false; }
    void RefreshInspectorTree() {}
    void SelectInspectorObject(wxObject*) {}
protected:
    void SetupInspectorAccelerator(wxWindow*) {}
};
#else
using wxInspectable = wxInspectableImpl;
#endif
```

## Library changes

In [src/inspector.cpp](src/inspector.cpp):
- Rename `wxInspectable::` method definitions → `wxInspectableImpl::`
- Rename `RegisterPlugin` → `detail::RegisterPluginImpl`

The real implementation is always compiled — no `#ifdef` in the .cpp.

## Usage

**Development (default):**
```cpp
#include <wx/inspector/inspector.h>

class MyDialog : public wxDialog, public wxInspectable {
    MyDialog(wxWindow* parent) : wxDialog(parent, "Settings") {
        SetupInspectorAccelerator(this);
    }
};
```
Full inspector works, links against `wxInspector.lib`.

**Release:**
```sh
g++ -DWXINSPECTOR_DISABLE -Ipath/to/wxInspector/include ...
```
Same integration code compiles to nothing. `wxInspector.lib` not needed.

## Files changed

| File | Change |
|---|---|
| `include/wx/inspector/inspector.h` | Rename class to `wxInspectableImpl`, wrap in `#ifndef WXINSPECTOR_DISABLE`. Add dummy `wxInspectable` class and `using` alias guarded by `#ifdef`. Add `RegisterPlugin` inline wrapper over `detail::RegisterPluginImpl`. |
| `src/inspector.cpp` | Rename `wxInspectable::` → `wxInspectableImpl::`, `RegisterPlugin` → `detail::RegisterPluginImpl`. No `#ifdef` needed. |

## Edge cases

| Case | Outcome |
|---|---|
| `WXINSPECTOR_DISABLE` not defined (default) | Full inspector, same behavior as today |
| `WXINSPECTOR_DISABLE` defined | All mixin methods are empty no-ops, zero overhead |
| `RegisterPlugin` called when disabled | No-op, no symbols pulled from library |
| `SetupInspectorAccelerator` called when disabled | No-op |
| `ShowInspector()` called when disabled | No-op, no window created, no crash |
| `IsInspectorVisible()` when disabled | Returns `false` |
| User mixes enabled/disabled TUs | Safe — dummy is header-only with no ABI surface |

## What this does NOT change

- `wxInspector::InspectionFrame` — always in library, used by `wxInspectableImpl` only
- All panel classes (tree, info, invoker, events, highlighter) — unchanged
- Sample app — unchanged
- Plugin interface — unchanged, plugins register through `RegisterPlugin`
