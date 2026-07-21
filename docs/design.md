# wxInspector — C++ Port of wxPython Widget Inspection Tool

## Overview

Port the wxPython `wx.lib.inspection` widget inspection tool to a standalone C++ library called **wxInspector**. The library enables runtime introspection of all live widgets and sizers in any wxWidgets application, equivalent to browser DevTools for native GUI apps.

- **Source:** `wx/lib/inspection.py` (wxPython Phoenix, ~1255 lines)
- **Target:** Standalone C++ project using wxWidgets C++ APIs and C++11
- **Build system:** CMake
- **Repository:** Independent from both wxPython Phoenix and wxWidgets core

---

## Components

### 1. Widget Tree Panel

A `wxTreeCtrl` displaying the live widget/sizer hierarchy.

**Tree building algorithm** (ported directly from Python):

1. Enumerate all top-level windows via `wxTopLevelWindows`
2. For each window, recursively add it and its children
3. If sizer mode is enabled, insert sizer nodes into the tree for each widget that has a sizer
4. Sizer items (windows, sub-sizers, spacers) appear as children of sizer nodes in blue text

Selection changes propagate to the Object Info panel, Method Invoker, and Event Logger.

**Data model:** Tree item data stores a raw `wxObject*`. The `InspectableObject` wrapper (see "Object Model & Type Handling" section) provides type-safe access. Items use `wxDynamicCast` for type discrimination — `wxWindow`, `wxSizer`, and `wxSizerItem` all derive from `wxObject`.

**Features:**
- Refresh (F1) — rebuild the entire tree
- Toggle Sizers (F3) — show/hide sizer nodes
- Expand All (F4) / Collapse All (F5)
- Select and scroll to a specific object

### 2. Object Info Panel

A `wxPropertyGrid` showing categorized, sortable, and **editable** properties of the selected object. This is an upgrade from the Python version's read-only `StyledTextCtrl`.

| Category | Properties |
|---|---|
| Identity | Name, Class, Class Hierarchy, wxWidgets ID, Pointer Address |
| Geometry | Position, Size, MinSize, BestSize, ClientSize, VirtualSize |
| State | Enabled, Shown, Frozen |
| Appearance | Foreground Colour, Background Colour, Label, Title (TLW only), Font |
| Layout | Containing Sizer, Proportion, Flag, Border |
| Sizer-specific | Position, Size, MinSize; Orientation (BoxSizer); Cols/Rows/VGap/HGap (GridSizer); RowHeights/ColWidths/FlexDir/NonFlexGrowMode (FlexGridSizer); EmptyCellSize (GridBagSizer) |
| Value | Value accessor for controls: `wxTextCtrl`, `wxSlider`, `wxSpinCtrl`, `wxCheckBox`, `wxChoice`, `wxComboBox`, `wxRadioButton`, `wxGauge`, `wxButton` |

Properties are discovered via a `PropertyProvider` registry, described in the Plugin Interface section. Editing a property immediately calls the corresponding setter on the live widget. Read-only properties (e.g., Class, Pointer) are marked non-editable. Sizer-specific properties only appear when the selected object is a sizer; Value properties only appear for known control types.

### 3. Method Invoker

Replaces PyCrust (impossible to port since it's an interactive Python interpreter).

- **Method dropdown** — populated from the method registry (described in the Plugin Interface section) filtered by the selected object's `wxClassInfo`. Shows method name and signature.
- **Parameter field** — comma-separated arguments, parsed with type coercion (string → int, bool, wxRect, wxColour, etc.)
- **Invoke button** — calls the method via a registered functor
- **Result area** — read-only text showing the return value or error message
- **History** — last 20 invocations persisted via `wxConfig`

**Default methods for `wxWindow`:**
`Show`, `Hide`, `Enable`, `Disable`, `SetFocus`, `Refresh`, `SetLabel(str)`, `SetSize(w,h)`, `Move(x,y)`, `SetPosition(x,y)`, `SetMinSize(w,h)`, `SetMaxSize(w,h)`, `SetBackgroundColour(r,g,b)`, `SetForegroundColour(r,g,b)`, `SetFont(size,face)`, `Center()`, `CenterOnParent()`, `Layout()`, `Fit()`, `SetSizerAndFit(...)`, `DestroyChildren()`

Additional methods are registered for specific control types (e.g., `SetValue`, `GetValue`, `SetRange` for sliders).

### 4. Event Logger

A tabbed companion to the Method Invoker. Replaces `wx.lib.eventwatcher`.

- **Event type checklist** — populated from a pre-built lookup table mapping `wxClassInfo` to relevant `wxEVT_*` event types. For `wxWindow`, this includes mouse, keyboard, focus, size, paint, and command events. Additional events are added for specific control types (e.g., `wxEVT_TEXT` for `wxTextCtrl`, `wxEVT_SLIDER` for `wxSlider`)
- **Start/Stop capture buttons**
- **Event log** — `wxDataViewCtrl` showing: Timestamp, Event Type Name, Event Data (ID, position for mouse events, key code for keyboard events, etc.)
- **Color-coded rows** by event category (Mouse, Keyboard, Focus, Size, Command, Misc)
- **Auto-scroll** with configurable max entries (default 500)

Event capture uses `wxEvtHandler::Connect()` to dynamically bind to all selected event types. A single dispatcher receives each event, logs it, and calls `evt.Skip()` so the event continues to normal handlers uninterrupted. On stop, all bindings are disconnected via `Disconnect()`.

### 5. Widget Highlighting

Direct port of `_InspectionHighlighter`. Selected items (F6) are highlighted visually on the live application window for ~3 seconds.

- **Windows:** Green outline rectangle. Top-level windows use a flicker effect (hide/show toggle) instead of drawing.
- **Sizers:** Outline of full sizer boundary (green) + internal item boundary lines (red) + item fill rectangles (dark blue).
- **Sizer items (spacers):** Individual boundary highlight.
- **Per-sizer-type logic:**
  - `wxBoxSizer` / `wxStaticBoxSizer`: Partition lines at item boundaries accounting for borders
  - `wxGridSizer`: Computed grid lines based on uniform cell sizes
  - `wxFlexGridSizer` / `wxGridBagSizer`: Uses actual `RowHeights` / `ColWidths` for grid lines
  - `wxWrapSizer`: Individual item rectangles

Rendering uses `wxOverlay` / `wxDCOverlay` on macOS and GTK3; falls back to `wxScreenDC` + `RefreshRect()` on other platforms. Auto-clears after 3 seconds via a one-shot `wxTimer`.

### 6. Find Widget Mode

Click "Find" (F2), the cursor changes, and the next left-click anywhere in the application calls `wxFindWindowAtPointer()` to locate the widget under the cursor. The tree selects it. Mouse capture lost or Escape cancels.

---

## Object Model & Type Handling

### InspectableObject

```cpp
class InspectableObject {
public:
    enum class Kind { Window, Sizer, SizerItem };

    Kind GetKind() const;
    wxString GetDisplayName() const;   // "wxButton ("OK")" or "wxBoxSizer"
    wxString GetClassName() const;     // via GetClassInfo()->GetClassName()
    wxString GetClassHierarchy() const; // walk GetBaseClass1() chain

    wxWindow*    AsWindow();           // nullptr if not Kind::Window
    wxSizer*     AsSizer();            // nullptr if not Kind::Sizer
    wxSizerItem* AsSizerItem();        // nullptr if not Kind::SizerItem
    wxObject*    GetObject() const;    // raw pointer for tree item data
};
```

### Type Checking Equivalents

| Python | C++ |
|---|---|
| `isinstance(obj, wx.Window)` | `obj.GetKind() == Kind::Window` |
| `isinstance(obj, wx.BoxSizer)` | `wxDynamicCast(obj.AsSizer(), wxBoxSizer)` |
| `hasattr(obj, 'GetTitle')` | `wxDynamicCast(obj.AsWindow(), wxTopLevelWindow)` |
| `hasattr(obj, 'GetValue')` | Check class info against a set of "valuable" types |
| `obj.__class__.__name__` | `obj.GetClassName()` |
| `obj.__class__.__bases__` | Walk `GetClassInfo()->GetBaseClass1()` chain |
| `inspect.getmodule(obj)` | Static table mapping class names to library names |
| `repr(obj.this)` | `wxString::Format("%p", obj.GetObject())` |
| `sys.version` / `sys.platform` | `wxGetOsDescription()` / `wxPlatformInfo` |

---

## Configuration & Persistence

Using `wxConfig` (default: `wxInspector` key in system config):

- Window position, size, and maximized state
- AUI pane layout perspective string (2 perspectives: with/without bottom pane)
- Include sizers toggle state
- Info panel property categories expanded/collapsed state
- Method invoker history (last 20 entries with timestamps)
- Event logger column widths and sort order
- Zoom level per pane

---

## Public API

### Minimal Integration

```cpp
#include <wx/inspector/inspector.h>

// In wxApp::OnInit():
wxInspector::Init();
// ... create windows, etc. ...
wxInspector::Show();  // or user presses Ctrl+Shift+I
```

### Mixin Class

```cpp
class MyApp : public wxApp, public wxInspectorMixin {
    bool OnInit() override {
        // wxInspectorMixin auto-registers Ctrl+Shift+I accelerator
        wxInspector::Init();
        return true;
    }
};
```

### Programmatic API

```cpp
wxInspector::Init(wxPoint defaultPos, wxSize defaultSize, wxConfig* config);
wxInspector::Show(wxObject* selectObj = nullptr, bool refreshTree = false);
wxInspector::Hide();
wxInspector::RefreshTree();
wxInspector::SelectObject(wxObject* obj);
wxInspector::IsVisible();
```

---

## Plugin Interface

For extensibility — allows third parties to add inspection support for custom widget types.

```cpp
class wxInspectorPlugin {
public:
    virtual ~wxInspectorPlugin();
    virtual wxString GetName() const = 0;

    // Provide additional properties for the PropertyGrid
    virtual bool CanProvideProperties(wxClassInfo* classInfo);
    virtual wxVector<PropertyDef> GetProperties(InspectableObject& obj);

    // Provide additional methods for the Method Invoker
    virtual wxVector<MethodInfo> GetMethods(wxClassInfo* classInfo);

    // Provide a custom panel (added as an AUI pane or notebook tab)
    virtual wxWindow* CreatePanel(wxWindow* parent);
};

wxInspector::RegisterPlugin(wxInspectorPlugin* plugin);
```

---

## Project Structure

```
wxInspector/
├── include/wx/inspector/
│   ├── inspector.h          -- Public API + wxInspectorMixin
│   ├── frame.h              -- InspectionFrame (AUI layout, toolbar)
│   ├── tree.h               -- InspectionTree (widget/sizer hierarchy)
│   ├── info.h               -- ObjectInfo panel (wxPropertyGrid)
│   ├── invoker.h            -- MethodInvoker panel
│   ├── events.h             -- EventLogger panel
│   ├── highlighter.h        -- _InspectionHighlighter (visual overlays)
│   ├── object.h             -- InspectableObject wrapper
│   ├── property_provider.h  -- PropertyProvider + PropertyDef
│   ├── method_registry.h    -- MethodInfo + MethodRegistry
│   └── plugin.h             -- wxInspectorPlugin interface
├── src/
│   ├── inspector.cpp
│   ├── frame.cpp
│   ├── tree.cpp
│   ├── info.cpp
│   ├── invoker.cpp
│   ├── events.cpp
│   ├── highlighter.cpp
│   ├── object.cpp
│   ├── property_provider.cpp
│   └── method_registry.cpp
├── samples/
│   └── minimal/
│       ├── CMakeLists.txt
│       └── minimal.cpp
├── CMakeLists.txt
├── README.md
└── LICENSE
```

---

## Platform Compatibility

- **Windows:** Full support (wxMSW). `wxOverlay` used for highlighting.
- **macOS:** Full support (wxOSX/Cocoa). `wxOverlay` used for highlighting.
- **Linux:** Full support (wxGTK3). `wxOverlay` used on GTK3.
- **wxWidgets version:** 3.2.0+ (matches Phoenix's bundled version)
- **C++ standard:** C++11 minimum

---

## What Is NOT Ported

| Feature | Reason |
|---|---|
| PyCrust interactive shell | Requires a Python interpreter — impossible in C++ |
| `wx.lib.mixins.inspection` as a standalone Python mixin | Replaced by `wxInspectorMixin` C++ class |
| `wx.lib.eventwatcher` as a standalone module | Integrated into the Event Logger panel |
| `PyEmbeddedImage` icons | Replaced with XPM-in-header or platform-native equivalents |
| `wx.py.editwindow` styles | PropertyGrid replaces StyledTextCtrl for info display |

---

## Verification Plan

1. **Build:** CMake configure + build on all three platforms (Windows, macOS, Linux)
2. **Sample:** Run `samples/minimal/` — a small wxWidgets app with a few buttons, text controls, a sizer, and a menu. Open the inspector via keyboard shortcut and via API call.
3. **Manual checks:**
   - Widget tree shows the correct hierarchy with names and classes
   - Toggle sizers on/off and verify sizer nodes appear/disappear
   - Click any widget in tree → Object Info updates with correct properties
   - Edit a property (e.g., Label) in the grid → widget updates in real time
   - Invoke `Show()` / `Hide()` / `SetSize()` from the Method Invoker → widget responds
   - Highlight a widget (F6) → green rectangle appears for 3 seconds
   - Highlight a sizer → sizer boundary + internal lines drawn
   - Find Widget (F2) → click on another window → tree selects it
   - Event Logger: start capture, interact with widget, see events logged, stop capture
   - Close inspector → window position/size/layout persisted, restored on reopen
4. **Edge cases:**
   - Widget destroyed while selected in tree → graceful "Object destroyed" display
   - Empty application (no windows) → tree shows "No top-level windows"
   - `wxFindWindowAtPointer()` returns null → bell sound, no crash
   - Malformed method parameters → error message in result area, no crash
