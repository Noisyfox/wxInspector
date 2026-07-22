# WXINSPECTOR_DISABLE Macro — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a `WXINSPECTOR_DISABLE` compile-time switch that replaces `wxInspectable` with a no-op dummy, requiring no library rebuild.

**Architecture:** Rename the real class to `wxInspectableImpl` (always compiled in the library). The public name `wxInspectable` becomes a `using` alias when enabled, or a header-only dummy when `WXINSPECTOR_DISABLE` is defined. Same pattern for `RegisterPlugin` → `detail::RegisterPluginImpl`.

**Tech Stack:** C++11, wxWidgets 3.2+

## Global Constraints

- wxWidgets 3.2.0+ required
- C++11 minimum
- `WXINSPECTOR_DISABLE` defined → all mixin methods are empty no-ops
- `WXINSPECTOR_DISABLE` not defined → full inspector, requires wxInspector.lib
- No library rebuild needed to switch modes
- `detail::RegisterPluginImpl` only declared when not disabled
- `wxInspectableImpl` only declared when not disabled
- Dummy `wxInspectable` has identical public/protected interface

---

### Task 1: Rename impl + add WXINSPECTOR_DISABLE guard

**Files:**
- Modify: `include/wx/inspector/inspector.h` — rename class, add dummy + alias
- Modify: `src/inspector.cpp` — rename methods and function

- [ ] **Step 1: Rewrite inspector.h**

Replace [include/wx/inspector/inspector.h](include/wx/inspector/inspector.h) entirely:

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

// --- Mixin (real implementation, always in library) ---

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

// --- Public-facing type ---

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

#endif // WX_INSPECTOR_INSPECTOR_H
```

- [ ] **Step 2: Rename in inspector.cpp**

In [src/inspector.cpp](src/inspector.cpp), make three changes:

1. Rename `RegisterPlugin` → `detail::RegisterPluginImpl` (line 12):
```cpp
void detail::RegisterPluginImpl(wxInspectorPlugin* plugin)
{
    PropertyProvider::Get().RegisterPlugin(plugin);
    MethodRegistry::Get().RegisterPlugin(plugin);
}
```

2. Rename constructor `wxInspectable::wxInspectable()` → `wxInspectableImpl::wxInspectableImpl()` (line 20):
```cpp
wxInspectableImpl::wxInspectableImpl()
    : m_inspectorFrame(nullptr)
    , m_accelWindow(nullptr)
{
}
```

3. Rename all remaining `wxInspectable::` method definitions to `wxInspectableImpl::`:
   - `wxInspectableImpl::SetupInspectorAccelerator`
   - `wxInspectableImpl::OnToggleInspector`
   - `wxInspectableImpl::ShowInspector`
   - `wxInspectableImpl::HideInspector`
   - `wxInspectableImpl::IsInspectorVisible`
   - `wxInspectableImpl::RefreshInspectorTree`
   - `wxInspectableImpl::SelectInspectorObject`

- [ ] **Step 3: Build to verify (enabled mode — default)**

Run:
```
cmake --build build --config RelWithDebInfo
```
Expected: Build succeeds with no errors.

- [ ] **Step 4: Build to verify (disabled mode)**

Temporarily add `-DWXINSPECTOR_DISABLE` to the compiler flags and rebuild. The simplest approach: add `target_compile_definitions(wxInspector PUBLIC -DWXINSPECTOR_DISABLE)` to the CMakeLists.txt just for the test, rebuild, verify it compiles, then remove the line.

Or verify by inspection — the dummy class is header-only with trivial inline methods, so it can't fail to compile.

- [ ] **Step 5: Commit**

```bash
git add include/wx/inspector/inspector.h src/inspector.cpp
git commit -m "feat: add WXINSPECTOR_DISABLE macro for zero-overhead inspector removal

- Rename wxInspectable -> wxInspectableImpl (real implementation)
- Rename RegisterPlugin -> detail::RegisterPluginImpl
- Add WXINSPECTOR_DISABLE guard: dummy wxInspectable with no-op methods
- When enabled (default): using wxInspectable = wxInspectableImpl
- No library rebuild needed to switch modes

Co-Authored-By: Claude <noreply@anthropic.com>"
```
