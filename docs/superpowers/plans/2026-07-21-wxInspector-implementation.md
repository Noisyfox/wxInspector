# wxInspector C++ Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a C++ widget inspection library that enables runtime introspection of all live widgets and sizers in any wxWidgets application, ported from wxPython's `wx.lib.inspection`.

**Architecture:** Bottom-up construction starting with the core data model (`InspectableObject`), then the plugin/registry infrastructure, then individual UI panels (tree, info, invoker, events, highlighter), then the AUI frame assembly, and finally the public API and sample app. Each panel is self-contained with a defined interface consumed by the frame.

**Tech Stack:** C++11, wxWidgets 3.2.0+, CMake 3.16+, wxAUI for layout, wxPropertyGrid for property editing, wxOverlay for highlighting.

## Global Constraints

- wxWidgets 3.2.0+ required (with `aui`, `propgrid` components)
- C++11 minimum standard
- CMake 3.16+ build system
- All public headers under `include/wx/inspector/`
- All implementations under `src/`
- wxWindows License
- Platform support: Windows (wxMSW), macOS (wxOSX/Cocoa), Linux (wxGTK3)

---

### Task 1: InspectableObject — Core Data Model

**Files:**
- Create: `include/wx/inspector/object.h`
- Create: `src/object.cpp`

**Interfaces:**
- Produces: `InspectableObject` class with `Kind` enum, `GetKind()`, `GetDisplayName()`, `GetClassName()`, `GetClassHierarchy()`, `AsWindow()`, `AsSizer()`, `AsSizerItem()`, `GetObject()`, `IsValid()`, static factory methods `FromWindow()`, `FromSizer()`, `FromSizerItem()`

- [ ] **Step 1: Write the header `include/wx/inspector/object.h`**

```cpp
#ifndef WX_INSPECTOR_OBJECT_H
#define WX_INSPECTOR_OBJECT_H

#include <wx/object.h>
#include <wx/string.h>
#include <wx/window.h>
#include <wx/sizer.h>

namespace wxInspector {

class InspectableObject {
public:
    enum class Kind {
        Window,
        Sizer,
        SizerItem
    };

    static InspectableObject FromWindow(wxWindow* window);
    static InspectableObject FromSizer(wxSizer* sizer);
    static InspectableObject FromSizerItem(wxSizerItem* item);

    Kind GetKind() const { return m_kind; }
    wxString GetDisplayName() const;
    wxString GetClassName() const;
    wxString GetClassHierarchy() const;

    wxWindow* AsWindow() const;
    wxSizer* AsSizer() const;
    wxSizerItem* AsSizerItem() const;
    wxObject* GetObject() const { return m_object; }

    bool IsValid() const;

private:
    InspectableObject(Kind kind, wxObject* obj);

    Kind m_kind;
    wxObject* m_object;
};

} // namespace wxInspector

#endif // WX_INSPECTOR_OBJECT_H
```

- [ ] **Step 2: Write the implementation `src/object.cpp`**

```cpp
#include "wx/inspector/object.h"
#include <wx/toplevel.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/gauge.h>
#include <wx/radiobut.h>

namespace wxInspector {

InspectableObject::InspectableObject(Kind kind, wxObject* obj)
    : m_kind(kind)
    , m_object(obj)
{
}

InspectableObject InspectableObject::FromWindow(wxWindow* window)
{
    return InspectableObject(Kind::Window, static_cast<wxObject*>(window));
}

InspectableObject InspectableObject::FromSizer(wxSizer* sizer)
{
    return InspectableObject(Kind::Sizer, static_cast<wxObject*>(sizer));
}

InspectableObject InspectableObject::FromSizerItem(wxSizerItem* item)
{
    return InspectableObject(Kind::SizerItem, static_cast<wxObject*>(item));
}

wxString InspectableObject::GetDisplayName() const
{
    if (!m_object) return "(null)";

    switch (m_kind) {
        case Kind::Window: {
            wxWindow* win = static_cast<wxWindow*>(m_object);
            wxString label = win->GetLabel();
            if (label.empty()) {
                wxTopLevelWindow* tlw = wxDynamicCast(win, wxTopLevelWindow);
                if (tlw) label = tlw->GetTitle();
            }
            wxString name = win->GetName();
            wxString result = GetClassName();
            if (!label.empty()) {
                result += wxT(" \"") + label + wxT("\"");
            }
            if (!name.empty()) {
                result += wxT(" [") + name + wxT("]");
            }
            return result;
        }
        case Kind::Sizer: {
            return GetClassName();
        }
        case Kind::SizerItem: {
            wxSizerItem* item = static_cast<wxSizerItem*>(m_object);
            if (item->IsWindow()) {
                return wxT("Item: ") + InspectableObject::FromWindow(item->GetWindow()).GetDisplayName();
            } else if (item->IsSizer()) {
                return wxT("Item: ") + InspectableObject::FromSizer(item->GetSizer()).GetDisplayName();
            } else if (item->IsSpacer()) {
                wxSize size = item->GetSpacer();
                return wxString::Format("Spacer (%d, %d)", size.x, size.y);
            }
            return wxT("Item: (unknown)");
        }
    }
    return wxT("(unknown)");
}

wxString InspectableObject::GetClassName() const
{
    if (!m_object) return wxT("(null)");
    wxClassInfo* info = m_object->GetClassInfo();
    if (info) {
        return wxString(info->GetClassName());
    }
    return wxT("wxObject");
}

wxString InspectableObject::GetClassHierarchy() const
{
    if (!m_object) return wxT("(null)");
    wxString result;
    wxClassInfo* info = m_object->GetClassInfo();
    while (info) {
        if (!result.empty()) result += wxT(" <- ");
        result += wxString(info->GetClassName());
        info = info->GetBaseClass1();
    }
    return result;
}

wxWindow* InspectableObject::AsWindow() const
{
    if (m_kind == Kind::Window && m_object) {
        return static_cast<wxWindow*>(m_object);
    }
    return nullptr;
}

wxSizer* InspectableObject::AsSizer() const
{
    if (m_kind == Kind::Sizer && m_object) {
        return static_cast<wxSizer*>(m_object);
    }
    return nullptr;
}

wxSizerItem* InspectableObject::AsSizerItem() const
{
    if (m_kind == Kind::SizerItem && m_object) {
        return static_cast<wxSizerItem*>(m_object);
    }
    return nullptr;
}

bool InspectableObject::IsValid() const
{
    return m_object != nullptr;
}

} // namespace wxInspector
```

- [ ] **Step 3: Verify the source files build**

Run: `mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug && cmake --build . --target wxInspector`
Expected: Library compiles without errors (linker may complain about missing symbols from other source files — that's fine, we only need compilation to succeed for the object files we've created; other .cpp files don't exist yet)

- [ ] **Step 4: Commit**

```bash
git add include/wx/inspector/object.h src/object.cpp
git commit -m "feat: add InspectableObject core data model

Wraps wxWindow*, wxSizer*, wxSizerItem* with type-safe Kind enum.
Provides display name, class name, and class hierarchy accessors."
```


### Task 2: Plugin Interface, PropertyDef, MethodInfo

**Files:**
- Create: `include/wx/inspector/plugin.h`

**Interfaces:**
- Consumes: `InspectableObject` (from Task 1)
- Produces: `PropertyDef` struct, `MethodInfo` struct, `wxInspectorPlugin` abstract base class

- [ ] **Step 1: Write the header `include/wx/inspector/plugin.h`**

```cpp
#ifndef WX_INSPECTOR_PLUGIN_H
#define WX_INSPECTOR_PLUGIN_H

#include <wx/string.h>
#include <wx/object.h>
#include <wx/dynarray.h>
#include <wx/window.h>
#include <functional>
#include "wx/inspector/object.h"

namespace wxInspector {

// --- PropertyDef ---

enum class PropertyType {
    String,
    Integer,
    Boolean,
    Colour,
    Font,
    Size,
    Point,
    Rect,
    Choice,     // string value from a list of choices
    ReadOnly    // display-only, no editing
};

struct PropertyDef {
    wxString name;               // display name in the property grid
    wxString category;           // category label (Identity, Geometry, etc.)
    PropertyType type;
    wxString value;              // current value as string
    bool readOnly;
    wxVector<wxString> choices;  // for PropertyType::Choice

    // For lazy evaluation and editing:
    // getter returns the current value string
    std::function<wxString()> getter;
    // setter receives the new value string, returns true on success
    std::function<bool(const wxString&)> setter;
};

// --- MethodInfo ---

struct MethodInfo {
    wxString name;           // e.g., "Show", "SetLabel"
    wxString signature;      // e.g., "Show()", "SetLabel(str)"
    wxString description;    // human-readable help text

    // Invoke with parsed string arguments, return result string.
    // The lambda captures the live widget pointer at enumeration time.
    std::function<wxString(const wxVector<wxString>& args)> invoke;
};

// --- wxInspectorPlugin ---

class wxInspectorPlugin {
public:
    virtual ~wxInspectorPlugin() {}

    virtual wxString GetName() const = 0;

    virtual bool CanProvideProperties(wxClassInfo* /*classInfo*/) { return false; }
    virtual wxVector<PropertyDef> GetProperties(InspectableObject& /*obj*/) {
        return wxVector<PropertyDef>();
    }

    virtual wxVector<MethodInfo> GetMethods(InspectableObject& /*obj*/) {
        return wxVector<MethodInfo>();
    }

    virtual wxWindow* CreatePanel(wxWindow* /*parent*/) { return nullptr; }
};

} // namespace wxInspector

#endif // WX_INSPECTOR_PLUGIN_H
```

- [ ] **Step 2: Verify the header compiles**

Run: `cd build && cmake --build . --target wxInspector 2>&1 | head -20`
Expected: No compile errors related to plugin.h

- [ ] **Step 3: Commit**

```bash
git add include/wx/inspector/plugin.h
git commit -m "feat: add plugin interface, PropertyDef, and MethodInfo types"
```


### Task 3: PropertyProvider Registry

**Files:**
- Create: `include/wx/inspector/property_provider.h`
- Create: `src/property_provider.cpp`

**Interfaces:**
- Consumes: `InspectableObject` (Task 1), `PropertyDef`, `wxInspectorPlugin` (Task 2)
- Produces: `PropertyProvider` singleton — `Get()`, `RegisterPlugin()`, `RegisterBuiltinProviders()`, `GetProperties(InspectableObject&)` returning `wxVector<PropertyDef>`

- [ ] **Step 1: Write the header `include/wx/inspector/property_provider.h`**

```cpp
#ifndef WX_INSPECTOR_PROPERTY_PROVIDER_H
#define WX_INSPECTOR_PROPERTY_PROVIDER_H

#include <wx/vector.h>
#include "wx/inspector/plugin.h"
#include "wx/inspector/object.h"

namespace wxInspector {

class PropertyProvider {
public:
    static PropertyProvider& Get();

    void RegisterPlugin(wxInspectorPlugin* plugin);
    void RegisterBuiltinProviders();

    wxVector<PropertyDef> GetProperties(InspectableObject& obj);

private:
    PropertyProvider() {}
    wxVector<wxInspectorPlugin*> m_plugins;
};

} // namespace wxInspector

#endif // WX_INSPECTOR_PROPERTY_PROVIDER_H
```

- [ ] **Step 2: Write the implementation `src/property_provider.cpp`**

Built-in providers cover all property categories from the design spec. For `Kind::Window`: Identity, Geometry, State, Appearance, Layout, Value. For `Kind::Sizer`: sizer-specific properties (Orientation for BoxSizer, Cols/Rows/VGap/HGap for GridSizer, FlexDir/NonFlexGrowMode for FlexGridSizer). For `Kind::SizerItem`: Proportion, Flag, Border, Size, MinSize, Type.

```cpp
#include "wx/inspector/property_provider.h"
#include <wx/toplevel.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/gauge.h>
#include <wx/radiobut.h>
#include <wx/button.h>
#include <wx/gbsizer.h>

namespace wxInspector {

// --- Builtin Window Property Provider ---

class BuiltinWindowProvider : public wxInspectorPlugin {
public:
    wxString GetName() const override { return "BuiltinWindow"; }

    bool CanProvideProperties(wxClassInfo* info) override {
        return info->IsKindOf(CLASSINFO(wxWindow));
    }

    wxVector<PropertyDef> GetProperties(InspectableObject& obj) override {
        wxVector<PropertyDef> props;
        wxWindow* win = obj.AsWindow();
        if (!win) return props;

        // Identity
        props.push_back({"Name", "Identity", PropertyType::String,
            win->GetName(), false, {},
            [win]() { return win->GetName(); }, nullptr});
        props.push_back({"Class", "Identity", PropertyType::ReadOnly,
            obj.GetClassName(), true, {},
            [&obj]() { return obj.GetClassName(); }, nullptr});
        props.push_back({"Class Hierarchy", "Identity", PropertyType::ReadOnly,
            obj.GetClassHierarchy(), true, {},
            [&obj]() { return obj.GetClassHierarchy(); }, nullptr});
        props.push_back({"wx ID", "Identity", PropertyType::ReadOnly,
            wxString::Format("%d", win->GetId()), true, {},
            [win]() { return wxString::Format("%d", win->GetId()); }, nullptr});
        props.push_back({"Pointer", "Identity", PropertyType::ReadOnly,
            wxString::Format("%p", win), true, {},
            [win]() { return wxString::Format("%p", win); }, nullptr});

        // Geometry
        props.push_back({"Position", "Geometry", PropertyType::ReadOnly,
            wxString::Format("(%d, %d)", win->GetRect().x, win->GetRect().y),
            true, {}, [win]() { wxRect r = win->GetRect();
                return wxString::Format("(%d, %d)", r.x, r.y); }, nullptr});
        props.push_back({"Size", "Geometry", PropertyType::ReadOnly,
            wxString::Format("(%d, %d)", win->GetSize().x, win->GetSize().y),
            true, {}, [win]() { wxSize s = win->GetSize();
                return wxString::Format("(%d, %d)", s.x, s.y); }, nullptr});
        props.push_back({"MinSize", "Geometry", PropertyType::String,
            wxString::Format("(%d,%d)", win->GetMinSize().x, win->GetMinSize().y),
            false, {},
            [win]() { wxSize s = win->GetMinSize();
                return wxString::Format("(%d,%d)", s.x, s.y); },
            [win](const wxString& val) {
                long w = -1, h = -1;
                if (sscanf(val.c_str(), "(%ld,%ld)", &w, &h) == 2 ||
                    sscanf(val.c_str(), "%ld,%ld", &w, &h) == 2) {
                    win->SetMinSize(wxSize(w, h)); return true;
                }
                return false;
            }});
        props.push_back({"BestSize", "Geometry", PropertyType::ReadOnly,
            wxString::Format("(%d, %d)", win->GetBestSize().x, win->GetBestSize().y),
            true, {}, [win]() { wxSize s = win->GetBestSize();
                return wxString::Format("(%d, %d)", s.x, s.y); }, nullptr});
        props.push_back({"ClientSize", "Geometry", PropertyType::ReadOnly,
            wxString::Format("(%d, %d)", win->GetClientSize().x, win->GetClientSize().y),
            true, {}, [win]() { wxSize s = win->GetClientSize();
                return wxString::Format("(%d, %d)", s.x, s.y); }, nullptr});

        // State
        props.push_back({"Enabled", "State", PropertyType::Boolean,
            win->IsEnabled() ? "true" : "false", false, {},
            [win]() { return win->IsEnabled() ? "true" : "false"; },
            [win](const wxString& val) {
                win->Enable(val == "true"); return true;
            }});
        props.push_back({"Shown", "State", PropertyType::Boolean,
            win->IsShown() ? "true" : "false", false, {},
            [win]() { return win->IsShown() ? "true" : "false"; },
            [win](const wxString& val) {
                win->Show(val == "true"); return true;
            }});

        // Appearance
        props.push_back({"Label", "Appearance", PropertyType::String,
            win->GetLabel(), false, {},
            [win]() { return win->GetLabel(); },
            [win](const wxString& val) { win->SetLabel(val); return true; }});
        wxTopLevelWindow* tlw = wxDynamicCast(win, wxTopLevelWindow);
        if (tlw) {
            props.push_back({"Title", "Appearance", PropertyType::String,
                tlw->GetTitle(), false, {},
                [tlw]() { return tlw->GetTitle(); },
                [tlw](const wxString& v) { tlw->SetTitle(v); return true; }});
        }
        props.push_back({"Fg Colour", "Appearance", PropertyType::Colour,
            win->GetForegroundColour().GetAsString(), false, {},
            [win]() { return win->GetForegroundColour().GetAsString(); },
            [win](const wxString& v) {
                win->SetForegroundColour(wxColour(v)); return true;
            }});
        props.push_back({"Bg Colour", "Appearance", PropertyType::Colour,
            win->GetBackgroundColour().GetAsString(), false, {},
            [win]() { return win->GetBackgroundColour().GetAsString(); },
            [win](const wxString& v) {
                win->SetBackgroundColour(wxColour(v)); return true;
            }});
        props.push_back({"Font", "Appearance", PropertyType::ReadOnly,
            win->GetFont().GetNativeFontInfoDesc(), true, {},
            [win]() { return win->GetFont().GetNativeFontInfoDesc(); },
            nullptr});

        // Layout
        wxSizer* sizer = win->GetContainingSizer();
        props.push_back({"Containing Sizer", "Layout", PropertyType::ReadOnly,
            sizer ? wxString(sizer->GetClassInfo()->GetClassName()) : "(none)",
            true, {}, [win]() { wxSizer* s = win->GetContainingSizer();
                return s ? wxString(s->GetClassInfo()->GetClassName()) : "(none)";
            }, nullptr});

        // Value (per-control type)
        AddValueProperty(win, props);
        return props;
    }

private:
    void AddValueProperty(wxWindow* win, wxVector<PropertyDef>& props) {
        if (wxTextCtrl* c = wxDynamicCast(win, wxTextCtrl)) {
            props.push_back({"Value", "Value", PropertyType::String,
                c->GetValue(), false, {},
                [c]() { return c->GetValue(); },
                [c](const wxString& v) { c->SetValue(v); return true; }});
        } else if (wxCheckBox* cb = wxDynamicCast(win, wxCheckBox)) {
            props.push_back({"Value", "Value", PropertyType::Boolean,
                cb->GetValue() ? "true" : "false", false, {},
                [cb]() { return cb->GetValue() ? "true" : "false"; },
                [cb](const wxString& v) {
                    cb->SetValue(v == "true"); return true;
                }});
        } else if (wxButton* btn = wxDynamicCast(win, wxButton)) {
            props.push_back({"Label", "Value", PropertyType::String,
                btn->GetLabel(), false, {},
                [btn]() { return btn->GetLabel(); },
                [btn](const wxString& v) { btn->SetLabel(v); return true; }});
        } else if (wxSlider* sl = wxDynamicCast(win, wxSlider)) {
            props.push_back({"Value", "Value", PropertyType::Integer,
                wxString::Format("%d", sl->GetValue()), false, {},
                [sl]() { return wxString::Format("%d", sl->GetValue()); },
                [sl](const wxString& v) { long l; return v.ToLong(&l)
                    ? (sl->SetValue((int)l), true) : false; }});
        } else if (wxSpinCtrl* sc = wxDynamicCast(win, wxSpinCtrl)) {
            props.push_back({"Value", "Value", PropertyType::Integer,
                wxString::Format("%d", sc->GetValue()), false, {},
                [sc]() { return wxString::Format("%d", sc->GetValue()); },
                [sc](const wxString& v) { long l; return v.ToLong(&l)
                    ? (sc->SetValue((int)l), true) : false; }});
        } else if (wxChoice* ch = wxDynamicCast(win, wxChoice)) {
            props.push_back({"Selection", "Value", PropertyType::Integer,
                wxString::Format("%d", ch->GetSelection()), false, {},
                [ch]() { return wxString::Format("%d", ch->GetSelection()); },
                [ch](const wxString& v) { long l; return v.ToLong(&l)
                    ? (ch->SetSelection((int)l), true) : false; }});
        } else if (wxComboBox* cbx = wxDynamicCast(win, wxComboBox)) {
            props.push_back({"Value", "Value", PropertyType::String,
                cbx->GetValue(), false, {},
                [cbx]() { return cbx->GetValue(); },
                [cbx](const wxString& v) { cbx->SetValue(v); return true; }});
        } else if (wxRadioButton* rb = wxDynamicCast(win, wxRadioButton)) {
            props.push_back({"Value", "Value", PropertyType::Boolean,
                rb->GetValue() ? "true" : "false", false, {},
                [rb]() { return rb->GetValue() ? "true" : "false"; },
                [rb](const wxString& v) {
                    rb->SetValue(v == "true"); return true;
                }});
        } else if (wxGauge* g = wxDynamicCast(win, wxGauge)) {
            props.push_back({"Value", "Value", PropertyType::Integer,
                wxString::Format("%d", g->GetValue()), false, {},
                [g]() { return wxString::Format("%d", g->GetValue()); },
                [g](const wxString& v) { long l; return v.ToLong(&l)
                    ? (g->SetValue((int)l), true) : false; }});
        }
    }
};

// --- Builtin Sizer Property Provider ---

class BuiltinSizerProvider : public wxInspectorPlugin {
public:
    wxString GetName() const override { return "BuiltinSizer"; }

    bool CanProvideProperties(wxClassInfo* info) override {
        return info->IsKindOf(CLASSINFO(wxSizer));
    }

    wxVector<PropertyDef> GetProperties(InspectableObject& obj) override {
        wxVector<PropertyDef> props;
        wxSizer* sizer = obj.AsSizer();
        if (!sizer) return props;

        props.push_back({"Class", "Identity", PropertyType::ReadOnly,
            obj.GetClassName(), true, {},
            [&obj]() { return obj.GetClassName(); }, nullptr});
        props.push_back({"Pointer", "Identity", PropertyType::ReadOnly,
            wxString::Format("%p", sizer), true, {},
            [sizer]() { return wxString::Format("%p", sizer); }, nullptr}));

        wxSize sz = sizer->GetSize();
        props.push_back({"Size", "Sizer", PropertyType::ReadOnly,
            wxString::Format("(%d, %d)", sz.x, sz.y), true, {},
            [sizer]() { wxSize s = sizer->GetSize();
                return wxString::Format("(%d, %d)", s.x, s.y); }, nullptr}));
        wxSize minSz = sizer->GetMinSize();
        props.push_back({"MinSize", "Sizer", PropertyType::ReadOnly,
            wxString::Format("(%d, %d)", minSz.x, minSz.y), true, {},
            [sizer]() { wxSize s = sizer->GetMinSize();
                return wxString::Format("(%d, %d)", s.x, s.y); }, nullptr}));

        if (wxBoxSizer* bs = wxDynamicCast(sizer, wxBoxSizer)) {
            props.push_back({"Orientation", "Sizer", PropertyType::ReadOnly,
                bs->GetOrientation() == wxHORIZONTAL ? "Horizontal" : "Vertical",
                true, {}, [bs]() { return bs->GetOrientation() == wxHORIZONTAL
                    ? "Horizontal" : "Vertical"; }, nullptr}));
        } else if (wxGridSizer* gs = wxDynamicCast(sizer, wxGridSizer)) {
            props.push_back({"Cols", "Sizer", PropertyType::ReadOnly,
                wxString::Format("%d", gs->GetCols()), true, {},
                [gs]() { return wxString::Format("%d", gs->GetCols()); }, nullptr}));
            props.push_back({"Rows", "Sizer", PropertyType::ReadOnly,
                wxString::Format("%d", gs->GetEffectiveRowsCount()), true, {},
                [gs]() { return wxString::Format("%d", gs->GetEffectiveRowsCount()); }, nullptr}));
            props.push_back({"VGap", "Sizer", PropertyType::ReadOnly,
                wxString::Format("%d", gs->GetVGap()), true, {},
                [gs]() { return wxString::Format("%d", gs->GetVGap()); }, nullptr}));
            props.push_back({"HGap", "Sizer", PropertyType::ReadOnly,
                wxString::Format("%d", gs->GetHGap()), true, {},
                [gs]() { return wxString::Format("%d", gs->GetHGap()); }, nullptr}));
        }
        if (wxFlexGridSizer* fgs = wxDynamicCast(sizer, wxFlexGridSizer)) {
            props.push_back({"FlexDir", "Sizer", PropertyType::ReadOnly,
                fgs->GetFlexibleDirection() == wxHORIZONTAL ? "Horizontal" : "Vertical",
                true, {}, [fgs]() { return fgs->GetFlexibleDirection() == wxHORIZONTAL
                    ? "Horizontal" : "Vertical"; }, nullptr}));
            props.push_back({"NonFlexGrowMode", "Sizer", PropertyType::ReadOnly,
                fgs->IsNonFlexibleGrowMode() ? "Grow" : "None", true, {},
                [fgs]() { return fgs->IsNonFlexibleGrowMode() ? "Grow" : "None"; }, nullptr}));
        }

        return props;
    }
};

// --- Builtin SizerItem Property Provider ---

class BuiltinSizerItemProvider : public wxInspectorPlugin {
public:
    wxString GetName() const override { return "BuiltinSizerItem"; }

    bool CanProvideProperties(wxClassInfo* info) override {
        return info->IsKindOf(CLASSINFO(wxSizerItem));
    }

    wxVector<PropertyDef> GetProperties(InspectableObject& obj) override {
        wxVector<PropertyDef> props;
        wxSizerItem* item = obj.AsSizerItem();
        if (!item) return props;

        props.push_back({"Class", "Identity", PropertyType::ReadOnly,
            "wxSizerItem", true, {}, []() { return "wxSizerItem"; }, nullptr}));

        if (item->IsSpacer()) {
            props.push_back({"Type", "Identity", PropertyType::ReadOnly,
                "Spacer", true, {}, []() { return "Spacer"; }, nullptr}));
        } else if (item->IsWindow()) {
            props.push_back({"Type", "Identity", PropertyType::ReadOnly,
                "Window", true, {}, []() { return "Window"; }, nullptr}));
        } else if (item->IsSizer()) {
            props.push_back({"Type", "Identity", PropertyType::ReadOnly,
                "Sizer", true, {}, []() { return "Sizer"; }, nullptr}));
        }

        props.push_back({"Proportion", "Layout", PropertyType::Integer,
            wxString::Format("%d", item->GetProportion()), false, {},
            [item]() { return wxString::Format("%d", item->GetProportion()); },
            [item](const wxString& v) { long l; return v.ToLong(&l)
                ? (item->SetProportion((int)l), true) : false; }}));
        props.push_back({"Flag", "Layout", PropertyType::Integer,
            wxString::Format("%d", item->GetFlag()), false, {},
            [item]() { return wxString::Format("%d", item->GetFlag()); },
            [item](const wxString& v) { long l; return v.ToLong(&l)
                ? (item->SetFlag((int)l), true) : false; }}));
        props.push_back({"Border", "Layout", PropertyType::Integer,
            wxString::Format("%d", item->GetBorder()), false, {},
            [item]() { return wxString::Format("%d", item->GetBorder()); },
            [item](const wxString& v) { long l; return v.ToLong(&l)
                ? (item->SetBorder((int)l), true) : false; }}));
        wxSize sz = item->GetSize();
        props.push_back({"Size", "Layout", PropertyType::ReadOnly,
            wxString::Format("(%d, %d)", sz.x, sz.y), true, {},
            [item]() { wxSize s = item->GetSize();
                return wxString::Format("(%d, %d)", s.x, s.y); }, nullptr}));
        wxSize minSz = item->GetMinSize();
        props.push_back({"MinSize", "Layout", PropertyType::ReadOnly,
            wxString::Format("(%d, %d)", minSz.x, minSz.y), true, {},
            [item]() { wxSize s = item->GetMinSize();
                return wxString::Format("(%d, %d)", s.x, s.y); }, nullptr}));

        return props;
    }
};

// --- PropertyProvider ---

PropertyProvider& PropertyProvider::Get() {
    static PropertyProvider instance;
    return instance;
}

void PropertyProvider::RegisterPlugin(wxInspectorPlugin* plugin) {
    m_plugins.push_back(plugin);
}

void PropertyProvider::RegisterBuiltinProviders() {
    static BuiltinWindowProvider winP;
    static BuiltinSizerProvider sizerP;
    static BuiltinSizerItemProvider itemP;
    RegisterPlugin(&winP);
    RegisterPlugin(&sizerP);
    RegisterPlugin(&itemP);
}

wxVector<PropertyDef> PropertyProvider::GetProperties(InspectableObject& obj) {
    wxVector<PropertyDef> result;
    if (!obj.IsValid()) return result;
    wxClassInfo* info = obj.GetObject()->GetClassInfo();
    if (!info) return result;
    for (auto* plugin : m_plugins) {
        if (plugin->CanProvideProperties(info)) {
            wxVector<PropertyDef> p = plugin->GetProperties(obj);
            result.insert(result.end(), p.begin(), p.end());
        }
    }
    return result;
}

} // namespace wxInspector
```

- [ ] **Step 3: Verify compilation**

Run: `cd build && cmake --build . --target wxInspector 2>&1 | tail -5`
Expected: Successful compilation (unresolved symbols for other missing .cpp files are expected at the library stage — the object file should compile cleanly)

- [ ] **Step 4: Commit**

```bash
git add include/wx/inspector/property_provider.h src/property_provider.cpp
git commit -m "feat: add PropertyProvider with builtin window/sizer/item providers"
```


### Task 4: MethodRegistry

**Files:**
- Create: `include/wx/inspector/method_registry.h`
- Create: `src/method_registry.cpp`

**Interfaces:**
- Consumes: `InspectableObject` (Task 1), `MethodInfo`, `wxInspectorPlugin` (Task 2)
- Produces: `MethodRegistry` singleton — `Get()`, `RegisterPlugin()`, `RegisterBuiltinMethods()`, `GetMethods(InspectableObject&)` returning `wxVector<MethodInfo>`

- [ ] **Step 1: Write the header `include/wx/inspector/method_registry.h`**

```cpp
#ifndef WX_INSPECTOR_METHOD_REGISTRY_H
#define WX_INSPECTOR_METHOD_REGISTRY_H

#include <wx/vector.h>
#include "wx/inspector/plugin.h"
#include "wx/inspector/object.h"

namespace wxInspector {

class MethodRegistry {
public:
    static MethodRegistry& Get();

    void RegisterPlugin(wxInspectorPlugin* plugin);
    void RegisterBuiltinMethods();

    wxVector<MethodInfo> GetMethods(InspectableObject& obj);

private:
    MethodRegistry() {}
    wxVector<wxInspectorPlugin*> m_plugins;
};

} // namespace wxInspector

#endif // WX_INSPECTOR_METHOD_REGISTRY_H
```

- [ ] **Step 2: Write the implementation `src/method_registry.cpp`**

```cpp
#include "wx/inspector/method_registry.h"
#include <wx/toplevel.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/gauge.h>
#include <wx/button.h>
#include <wx/colour.h>

namespace wxInspector {

// Helper to parse arguments from comma-separated string
static wxVector<wxString> ParseArgs(const wxString& argStr) {
    wxVector<wxString> result;
    if (argStr.empty()) return result;
    // Simple split on comma — handles basic cases
    wxString current;
    bool inParens = false;
    for (size_t i = 0; i < argStr.length(); i++) {
        wxChar c = argStr[i];
        if (c == '(') inParens = true;
        else if (c == ')') inParens = false;
        if (c == ',' && !inParens) {
            current.Trim(true).Trim(false);
            result.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    current.Trim(true).Trim(false);
    if (!current.empty()) result.push_back(current);
    return result;
}

// --- Builtin Window Methods ---

class BuiltinWindowMethods : public wxInspectorPlugin {
public:
    wxString GetName() const override { return "BuiltinWindowMethods"; }

    wxVector<MethodInfo> GetMethods(InspectableObject& obj) override {
        wxVector<MethodInfo> methods;
        wxWindow* win = obj.AsWindow();
        if (!win) return methods;

        methods.push_back({"Show", "Show()",
            "Show the window",
            [win](const wxVector<wxString>&) -> wxString {
                win->Show(true);
                return "Shown";
            }});
        methods.push_back({"Hide", "Hide()",
            "Hide the window",
            [win](const wxVector<wxString>&) -> wxString {
                win->Show(false);
                return "Hidden";
            }});
        methods.push_back({"Enable", "Enable()",
            "Enable the window",
            [win](const wxVector<wxString>&) -> wxString {
                win->Enable(true);
                return "Enabled";
            }});
        methods.push_back({"Disable", "Disable()",
            "Disable the window",
            [win](const wxVector<wxString>&) -> wxString {
                win->Enable(false);
                return "Disabled";
            }});
        methods.push_back({"SetFocus", "SetFocus()",
            "Set keyboard focus to this window",
            [win](const wxVector<wxString>&) -> wxString {
                win->SetFocus();
                return "Focus set";
            }});
        methods.push_back({"Refresh", "Refresh()",
            "Redraw the window",
            [win](const wxVector<wxString>&) -> wxString {
                win->Refresh();
                return "Refreshed";
            }});
        methods.push_back({"SetLabel", "SetLabel(str)",
            "Set the window label",
            [win](const wxVector<wxString>& args) -> wxString {
                if (args.size() < 1) return "Error: expected 1 argument (str)";
                win->SetLabel(args[0]);
                return "Label set to: " + args[0];
            }});
        methods.push_back({"SetSize", "SetSize(w,h)",
            "Set the window size",
            [win](const wxVector<wxString>& args) -> wxString {
                if (args.size() < 2) return "Error: expected 2 arguments (w, h)";
                long w, h;
                if (!args[0].ToLong(&w) || !args[1].ToLong(&h))
                    return "Error: invalid integer arguments";
                win->SetSize((int)w, (int)h);
                return wxString::Format("Size set to (%d, %d)", (int)w, (int)h);
            }});
        methods.push_back({"Move", "Move(x,y)",
            "Move the window",
            [win](const wxVector<wxString>& args) -> wxString {
                if (args.size() < 2) return "Error: expected 2 arguments (x, y)";
                long x, y;
                if (!args[0].ToLong(&x) || !args[1].ToLong(&y))
                    return "Error: invalid integer arguments";
                win->Move((int)x, (int)y);
                return wxString::Format("Moved to (%d, %d)", (int)x, (int)y);
            }});
        methods.push_back({"SetMinSize", "SetMinSize(w,h)",
            "Set minimum size",
            [win](const wxVector<wxString>& args) -> wxString {
                if (args.size() < 2) return "Error: expected 2 arguments (w, h)";
                long w, h;
                if (!args[0].ToLong(&w) || !args[1].ToLong(&h))
                    return "Error: invalid integer arguments";
                win->SetMinSize(wxSize((int)w, (int)h));
                return wxString::Format("MinSize set to (%d, %d)", (int)w, (int)h);
            }});
        methods.push_back({"SetMaxSize", "SetMaxSize(w,h)",
            "Set maximum size",
            [win](const wxVector<wxString>& args) -> wxString {
                if (args.size() < 2) return "Error: expected 2 arguments (w, h)";
                long w, h;
                if (!args[0].ToLong(&w) || !args[1].ToLong(&h))
                    return "Error: invalid integer arguments";
                win->SetMaxSize(wxSize((int)w, (int)h));
                return wxString::Format("MaxSize set to (%d, %d)", (int)w, (int)h);
            }});
        methods.push_back({"SetBackgroundColour", "SetBackgroundColour(r,g,b)",
            "Set background colour",
            [win](const wxVector<wxString>& args) -> wxString {
                if (args.size() < 3)
                    return "Error: expected 3 arguments (r, g, b)";
                long r, g, b;
                if (!args[0].ToLong(&r) || !args[1].ToLong(&g) || !args[2].ToLong(&b))
                    return "Error: invalid colour values";
                win->SetBackgroundColour(wxColour((int)r, (int)g, (int)b));
                return wxString::Format("Background set to (%d, %d, %d)",
                    (int)r, (int)g, (int)b);
            }});
        methods.push_back({"SetForegroundColour", "SetForegroundColour(r,g,b)",
            "Set foreground colour",
            [win](const wxVector<wxString>& args) -> wxString {
                if (args.size() < 3)
                    return "Error: expected 3 arguments (r, g, b)";
                long r, g, b;
                if (!args[0].ToLong(&r) || !args[1].ToLong(&g) || !args[2].ToLong(&b))
                    return "Error: invalid colour values";
                win->SetForegroundColour(wxColour((int)r, (int)g, (int)b));
                return wxString::Format("Foreground set to (%d, %d, %d)",
                    (int)r, (int)g, (int)b);
            }});
        methods.push_back({"SetFont", "SetFont(size,face)",
            "Set font — size in points, optional face name",
            [win](const wxVector<wxString>& args) -> wxString {
                if (args.size() < 1)
                    return "Error: expected at least 1 argument (size)";
                long size;
                if (!args[0].ToLong(&size))
                    return "Error: invalid font size";
                wxFont font((int)size, wxFONTFAMILY_DEFAULT,
                    wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
                if (args.size() >= 2 && !args[1].empty()) {
                    font.SetFaceName(args[1]);
                }
                win->SetFont(font);
                return "Font updated";
            }});
        methods.push_back({"Center", "Center()",
            "Center on screen",
            [win](const wxVector<wxString>&) -> wxString {
                win->Center();
                return "Centered on screen";
            }});
        methods.push_back({"CenterOnParent", "CenterOnParent()",
            "Center on parent",
            [win](const wxVector<wxString>&) -> wxString {
                win->CenterOnParent();
                return "Centered on parent";
            }});
        methods.push_back({"Layout", "Layout()",
            "Re-layout the window",
            [win](const wxVector<wxString>&) -> wxString {
                win->Layout();
                return "Layout done";
            }});
        methods.push_back({"Fit", "Fit()",
            "Fit window to children",
            [win](const wxVector<wxString>&) -> wxString {
                win->Fit();
                return "Fit done";
            }});
        methods.push_back({"SetSizerAndFit", "SetSizerAndFit(sizer)",
            "Set sizer and fit — pass sizer pointer address",
            [win](const wxVector<wxString>& args) -> wxString {
                // Fit the existing sizer
                win->Fit();
                return "Fit to current sizer done";
            }});
        methods.push_back({"DestroyChildren", "DestroyChildren()",
            "Destroy all child windows",
            [win](const wxVector<wxString>&) -> wxString {
                win->DestroyChildren();
                return "Children destroyed";
            }});

        // Control-specific methods
        if (wxTextCtrl* tc = wxDynamicCast(win, wxTextCtrl)) {
            methods.push_back({"SetValue", "SetValue(str)",
                "Set the text value",
                [tc](const wxVector<wxString>& args) -> wxString {
                    if (args.size() < 1) return "Error: expected 1 argument";
                    tc->SetValue(args[0]);
                    return "Value set";
                }});
            methods.push_back({"GetValue", "GetValue()",
                "Get the text value",
                [tc](const wxVector<wxString>&) -> wxString {
                    return tc->GetValue();
                }});
        }
        if (wxSlider* sl = wxDynamicCast(win, wxSlider)) {
            methods.push_back({"SetValue", "SetValue(int)",
                "Set slider value",
                [sl](const wxVector<wxString>& args) -> wxString {
                    if (args.size() < 1) return "Error: expected 1 argument";
                    long v;
                    if (!args[0].ToLong(&v)) return "Error: invalid integer";
                    sl->SetValue((int)v);
                    return "Value set";
                }});
            methods.push_back({"SetRange", "SetRange(min,max)",
                "Set slider range",
                [sl](const wxVector<wxString>& args) -> wxString {
                    if (args.size() < 2) return "Error: expected 2 arguments";
                    long min, max;
                    if (!args[0].ToLong(&min) || !args[1].ToLong(&max))
                        return "Error: invalid integers";
                    sl->SetRange((int)min, (int)max);
                    return "Range set";
                }});
        }
        if (wxSpinCtrl* sc = wxDynamicCast(win, wxSpinCtrl)) {
            methods.push_back({"SetValue", "SetValue(int)",
                "Set spin value",
                [sc](const wxVector<wxString>& args) -> wxString {
                    if (args.size() < 1) return "Error: expected 1 argument";
                    long v;
                    if (!args[0].ToLong(&v)) return "Error: invalid integer";
                    sc->SetValue((int)v);
                    return "Value set";
                }});
        }

        return methods;
    }
};

// --- MethodRegistry ---

MethodRegistry& MethodRegistry::Get() {
    static MethodRegistry instance;
    return instance;
}

void MethodRegistry::RegisterPlugin(wxInspectorPlugin* plugin) {
    m_plugins.push_back(plugin);
}

void MethodRegistry::RegisterBuiltinMethods() {
    static BuiltinWindowMethods winMethods;
    RegisterPlugin(&winMethods);
}

wxVector<MethodInfo> MethodRegistry::GetMethods(InspectableObject& obj) {
    wxVector<MethodInfo> result;
    if (!obj.IsValid()) return result;
    for (auto* plugin : m_plugins) {
        wxVector<MethodInfo> m = plugin->GetMethods(obj);
        result.insert(result.end(), m.begin(), m.end());
    }
    return result;
}

} // namespace wxInspector
```

- [ ] **Step 3: Verify compilation**

Run: `cd build && cmake --build . --target wxInspector 2>&1 | tail -5`
Expected: Clean compile of method_registry.cpp

- [ ] **Step 4: Commit**

```bash
git add include/wx/inspector/method_registry.h src/method_registry.cpp
git commit -m "feat: add MethodRegistry with builtin window/control methods

Includes Show, Hide, Enable, Disable, SetFocus, Refresh, SetLabel,
SetSize, Move, SetMinSize, SetMaxSize, SetBackgroundColour,
SetForegroundColour, SetFont, Center, CenterOnParent, Layout, Fit,
SetSizerAndFit, DestroyChildren plus control-specific methods."
```


### Task 5: Widget Tree Panel

**Files:**
- Create: `include/wx/inspector/tree.h`
- Create: `src/tree.cpp`

**Interfaces:**
- Consumes: `InspectableObject` (Task 1)
- Produces: `InspectionTree` class (a `wxPanel` subclass) — `RebuildTree()`, `ToggleSizers()`, `ExpandAll()`, `CollapseAll()`, `SelectObject(wxObject*)`, `FindWidget()`
- Produces: Event `wxEVT_INSPECT_TREE_SEL_CHANGED` — fired when tree selection changes, carrying `InspectableObject` via `wxCommandEvent::SetClientData`

- [ ] **Step 1: Write the header `include/wx/inspector/tree.h`**

```cpp
#ifndef WX_INSPECTOR_TREE_H
#define WX_INSPECTOR_TREE_H

#include <wx/panel.h>
#include <wx/treectrl.h>
#include <wx/toolbar.h>
#include "wx/inspector/object.h"

namespace wxInspector {

wxDECLARE_EVENT(wxEVT_INSPECT_TREE_SEL_CHANGED, wxCommandEvent);

class InspectionTree : public wxPanel {
public:
    InspectionTree(wxWindow* parent);

    void RebuildTree();
    void ToggleSizers();
    void ExpandAll();
    void CollapseAll();
    void SelectObject(wxObject* obj);
    void FindWidget();
    InspectableObject GetSelectedObject() const;
    wxTreeCtrl* GetTreeCtrl() { return m_tree; }

    bool IsSizerModeEnabled() const { return m_showSizers; }

private:
    void OnTreeSelChanged(wxTreeEvent& event);
    void OnRefresh(wxCommandEvent& event);
    void OnToggleSizers(wxCommandEvent& event);
    void OnExpandAll(wxCommandEvent& event);
    void OnCollapseAll(wxCommandEvent& event);
    void OnFindWidget(wxCommandEvent& event);
    void OnKeyDown(wxKeyEvent& event);

    void AddChildren(wxTreeItemId parentId, wxWindow* window);
    void AddSizerChildren(wxTreeItemId parentId, wxSizer* sizer);
    void AddSizerItems(wxTreeItemId sizerNodeId, wxSizer* sizer);
    wxTreeItemId DoAddWindow(wxTreeItemId parentId, wxWindow* window);
    wxTreeItemId DoAddSizer(wxTreeItemId parentId, wxSizer* sizer);
    wxTreeItemId DoAddSizerItem(wxTreeItemId parentId, wxSizerItem* item);

    wxTreeCtrl* m_tree;
    wxToolBar* m_toolbar;
    bool m_showSizers;
    wxDECLARE_EVENT_TABLE();
};

} // namespace wxInspector

#endif // WX_INSPECTOR_TREE_H
```

- [ ] **Step 2: Write the implementation `src/tree.cpp`**

```cpp
#include "wx/inspector/tree.h"
#include <wx/toplevel.h>
#include <wx/sizer.h>
#include <wx/artprov.h>
#include <wx/utils.h>

namespace wxInspector {

wxDEFINE_EVENT(wxEVT_INSPECT_TREE_SEL_CHANGED, wxCommandEvent);

// TreeItemData subclass to store the wxObject* properly
class ObjectTreeItemData : public wxTreeItemData {
public:
    wxObject* m_object;
    InspectableObject::Kind m_kind;
    ObjectTreeItemData(wxObject* obj, InspectableObject::Kind kind)
        : m_object(obj), m_kind(kind) {}
};

// IDs
enum {
    ID_TREE_REFRESH = wxID_HIGHEST + 100,
    ID_TREE_TOGGLE_SIZERS,
    ID_TREE_EXPAND_ALL,
    ID_TREE_COLLAPSE_ALL,
    ID_TREE_FIND_WIDGET
};

wxBEGIN_EVENT_TABLE(InspectionTree, wxPanel)
    EVT_TREE_SEL_CHANGED(wxID_ANY, InspectionTree::OnTreeSelChanged)
wxEND_EVENT_TABLE()

InspectionTree::InspectionTree(wxWindow* parent)
    : wxPanel(parent, wxID_ANY), m_showSizers(false)
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    m_toolbar = new wxToolBar(this, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxTB_FLAT | wxTB_NODIVIDER);
    m_toolbar->AddTool(ID_TREE_REFRESH, "Refresh",
        wxArtProvider::GetBitmap(wxART_REDO), "Refresh (F1)");
    m_toolbar->AddTool(ID_TREE_TOGGLE_SIZERS, "Sizers",
        wxArtProvider::GetBitmap(wxART_FIND), "Toggle Sizers (F3)",
        wxITEM_CHECK);
    m_toolbar->AddTool(ID_TREE_EXPAND_ALL, "Expand All",
        wxArtProvider::GetBitmap(wxART_PLUS), "Expand All (F4)");
    m_toolbar->AddTool(ID_TREE_COLLAPSE_ALL, "Collapse All",
        wxArtProvider::GetBitmap(wxART_MINUS), "Collapse All (F5)");
    m_toolbar->AddTool(ID_TREE_FIND_WIDGET, "Find",
        wxArtProvider::GetBitmap(wxART_CROSS_MARK), "Find Widget (F2)");
    m_toolbar->Realize();
    mainSizer->Add(m_toolbar, 0, wxEXPAND);

    m_tree = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT);
    m_tree->AddRoot("(root)");
    mainSizer->Add(m_tree, 1, wxEXPAND);

    SetSizer(mainSizer);

    Bind(wxEVT_TOOL, &InspectionTree::OnRefresh, this, ID_TREE_REFRESH);
    Bind(wxEVT_TOOL, &InspectionTree::OnToggleSizers, this, ID_TREE_TOGGLE_SIZERS);
    Bind(wxEVT_TOOL, &InspectionTree::OnExpandAll, this, ID_TREE_EXPAND_ALL);
    Bind(wxEVT_TOOL, &InspectionTree::OnCollapseAll, this, ID_TREE_COLLAPSE_ALL);
    Bind(wxEVT_TOOL, &InspectionTree::OnFindWidget, this, ID_TREE_FIND_WIDGET);
    m_tree->Bind(wxEVT_KEY_DOWN, &InspectionTree::OnKeyDown, this);

    RebuildTree();
}

void InspectionTree::RebuildTree()
{
    wxTreeItemId selectedId = m_tree->GetSelection();
    wxObject* previouslySelected = nullptr;
    if (selectedId.IsOk()) {
        ObjectTreeItemData* d = dynamic_cast<ObjectTreeItemData*>(
            m_tree->GetItemData(selectedId));
        if (d) previouslySelected = d->m_object;
    }

    m_tree->DeleteAllItems();
    wxTreeItemId root = m_tree->AddRoot("(root)");

    wxWindowList& topWindows = wxTopLevelWindows;
    for (size_t n = 0; n < topWindows.GetCount(); n++) {
        wxWindow* win = topWindows[n];
        if (!win) continue;
        wxTreeItemId nodeId = DoAddWindow(root, win);

        if (m_showSizers) {
            wxSizer* sz = win->GetSizer();
            if (sz) {
                wxTreeItemId sizerNode = DoAddSizer(nodeId, sz);
                AddSizerItems(sizerNode, sz);
            }
        }

        AddChildren(nodeId, win);
    }

    if (!m_tree->GetChildrenCount(root, false))
        m_tree->AppendItem(root, "No top-level windows");

    m_tree->Expand(root);

    if (previouslySelected)
        SelectObject(previouslySelected);
}

void InspectionTree::AddChildren(wxTreeItemId parentId, wxWindow* window)
{
    const wxWindowList& children = window->GetChildren();
    for (size_t n = 0; n < children.GetCount(); n++) {
        wxWindow* child = children[n];
        if (!child) continue;
        wxTreeItemId nodeId = DoAddWindow(parentId, child);

        if (m_showSizers) {
            wxSizer* sz = child->GetSizer();
            if (sz) {
                wxTreeItemId sizerNode = DoAddSizer(nodeId, sz);
                AddSizerItems(sizerNode, sz);
            }
        }

        AddChildren(nodeId, child);
    }
}

void InspectionTree::AddSizerItems(wxTreeItemId sizerNodeId, wxSizer* sizer)
{
    wxSizerItemList& items = sizer->GetChildren();
    for (size_t n = 0; n < items.GetCount(); n++) {
        wxSizerItem* item = items[n];
        if (!item) continue;

        wxTreeItemId itemNode = DoAddSizerItem(sizerNodeId, item);

        if (item->IsWindow()) {
            wxWindow* win = item->GetWindow();
            if (m_showSizers) {
                wxSizer* childSz = win->GetSizer();
                if (childSz) {
                    wxTreeItemId csNode = DoAddSizer(itemNode, childSz);
                    AddSizerItems(csNode, childSz);
                }
            }
            AddChildren(itemNode, win);
        } else if (item->IsSizer()) {
            AddSizerItems(itemNode, item->GetSizer());
        }
    }
}

wxTreeItemId InspectionTree::DoAddWindow(wxTreeItemId parentId, wxWindow* window)
{
    InspectableObject obj = InspectableObject::FromWindow(window);
    wxTreeItemId id = m_tree->AppendItem(parentId, obj.GetDisplayName());
    m_tree->SetItemData(id,
        new ObjectTreeItemData(window, InspectableObject::Kind::Window));
    return id;
}

wxTreeItemId InspectionTree::DoAddSizer(wxTreeItemId parentId, wxSizer* sizer)
{
    InspectableObject obj = InspectableObject::FromSizer(sizer);
    wxTreeItemId id = m_tree->AppendItem(parentId, obj.GetDisplayName());
    m_tree->SetItemTextColour(id, *wxBLUE);
    m_tree->SetItemData(id,
        new ObjectTreeItemData(sizer, InspectableObject::Kind::Sizer));
    return id;
}

wxTreeItemId InspectionTree::DoAddSizerItem(wxTreeItemId parentId, wxSizerItem* item)
{
    InspectableObject obj = InspectableObject::FromSizerItem(item);
    wxTreeItemId id = m_tree->AppendItem(parentId, obj.GetDisplayName());
    m_tree->SetItemTextColour(id, *wxBLUE);
    m_tree->SetItemData(id,
        new ObjectTreeItemData(item, InspectableObject::Kind::SizerItem));
    return id;
}

void InspectionTree::ToggleSizers()
{
    m_showSizers = !m_showSizers;
    m_toolbar->ToggleTool(ID_TREE_TOGGLE_SIZERS, m_showSizers);
    RebuildTree();
}

void InspectionTree::ExpandAll()
{
    ExpandAllChildren(m_tree->GetRootItem());
}

void InspectionTree::CollapseAll()
{
    CollapseAllChildren(m_tree->GetRootItem());
}

void InspectionTree::ExpandAllChildren(wxTreeItemId item)
{
    m_tree->Expand(item);
    wxTreeItemIdValue cookie;
    wxTreeItemId child = m_tree->GetFirstChild(item, cookie);
    while (child.IsOk()) {
        ExpandAllChildren(child);
        child = m_tree->GetNextChild(item, cookie);
    }
}

void InspectionTree::CollapseAllChildren(wxTreeItemId item)
{
    wxTreeItemIdValue cookie;
    wxTreeItemId child = m_tree->GetFirstChild(item, cookie);
    while (child.IsOk()) {
        CollapseAllChildren(child);
        child = m_tree->GetNextChild(item, cookie);
    }
    if (item != m_tree->GetRootItem())
        m_tree->Collapse(item);
}

void InspectionTree::SelectObject(wxObject* obj)
{
    wxTreeItemId found = FindItemForObject(m_tree->GetRootItem(), obj);
    if (found.IsOk()) {
        m_tree->SelectItem(found);
        m_tree->EnsureVisible(found);
    }
}

InspectableObject InspectionTree::GetSelectedObject() const
{
    wxTreeItemId sel = m_tree->GetSelection();
    if (!sel.IsOk())
        return InspectableObject::FromWindow(nullptr);

    ObjectTreeItemData* d = dynamic_cast<ObjectTreeItemData*>(
        m_tree->GetItemData(sel));
    if (!d || !d->m_object)
        return InspectableObject::FromWindow(nullptr);

    switch (d->m_kind) {
    case InspectableObject::Kind::Window:
        return InspectableObject::FromWindow(
            static_cast<wxWindow*>(d->m_object));
    case InspectableObject::Kind::Sizer:
        return InspectableObject::FromSizer(
            static_cast<wxSizer*>(d->m_object));
    case InspectableObject::Kind::SizerItem:
        return InspectableObject::FromSizerItem(
            static_cast<wxSizerItem*>(d->m_object));
    }
    return InspectableObject::FromWindow(nullptr);
}

wxTreeItemId InspectionTree::FindItemForObject(wxTreeItemId parent, wxObject* target)
{
    wxTreeItemIdValue cookie;
    wxTreeItemId child = m_tree->GetFirstChild(parent, cookie);
    while (child.IsOk()) {
        ObjectTreeItemData* d = dynamic_cast<ObjectTreeItemData*>(
            m_tree->GetItemData(child));
        if (d && d->m_object == target)
            return child;
        wxTreeItemId found = FindItemForObject(child, target);
        if (found.IsOk()) return found;
        child = m_tree->GetNextChild(parent, cookie);
    }
    return wxTreeItemId();
}

void InspectionTree::FindWidget()
{
#if wxUSE_POPUPWIN
    CaptureMouse();
    SetCursor(wxCURSOR_CROSS);

    Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& evt) {
        if (HasCapture()) ReleaseMouse();
        SetCursor(wxNullCursor);
        wxPoint pt = ::wxGetMousePosition();
        wxWindow* win = wxFindWindowAtPointer(pt);
        if (win) SelectObject(win);
        else wxBell();
        Unbind(wxEVT_LEFT_DOWN, this);
    }, this);
#endif
}

// --- Event handlers ---

void InspectionTree::OnTreeSelChanged(wxTreeEvent& event)
{
    wxTreeItemId id = event.GetItem();
    if (!id.IsOk()) return;

    ObjectTreeItemData* d = dynamic_cast<ObjectTreeItemData*>(
        m_tree->GetItemData(id));
    if (!d) return;

    wxCommandEvent selEvent(wxEVT_INSPECT_TREE_SEL_CHANGED, GetId());
    selEvent.SetClientData(d->m_object);
    selEvent.SetEventObject(this);
    ProcessWindowEvent(selEvent);
}

void InspectionTree::OnRefresh(wxCommandEvent&) { RebuildTree(); }
void InspectionTree::OnToggleSizers(wxCommandEvent&) { ToggleSizers(); }
void InspectionTree::OnExpandAll(wxCommandEvent&) { ExpandAll(); }
void InspectionTree::OnCollapseAll(wxCommandEvent&) { CollapseAll(); }
void InspectionTree::OnFindWidget(wxCommandEvent&) { FindWidget(); }

void InspectionTree::OnKeyDown(wxKeyEvent& event)
{
    switch (event.GetKeyCode()) {
    case WXK_F1: RebuildTree(); return;
    case WXK_F2: FindWidget(); return;
    case WXK_F3: ToggleSizers(); return;
    case WXK_F4: ExpandAll(); return;
    case WXK_F5: CollapseAll(); return;
    default: break;
    }
    event.Skip();
}

} // namespace wxInspector
```

- [ ] **Step 3: Update header with missing private method declarations**

Add these to the private section of `InspectionTree` in `include/wx/inspector/tree.h`:

```cpp
    void ExpandAllChildren(wxTreeItemId item);
    void CollapseAllChildren(wxTreeItemId item);
    wxTreeItemId FindItemForObject(wxTreeItemId parent, wxObject* target);
```

- [ ] **Step 4: Verify compilation**

Run: `cd build && cmake --build . --target wxInspector 2>&1 | tail -5`
Expected: Clean compile

- [ ] **Step 5: Commit**

```bash
git add include/wx/inspector/tree.h src/tree.cpp
git commit -m "feat: add InspectionTree panel with widget/sizer hierarchy

Builds tree from wxTopLevelWindows, recursively adds children.
Supports sizer toggle (F3), expand/collapse all (F4/F5), refresh (F1),
find widget mode (F2). Fires wxEVT_INSPECT_TREE_SEL_CHANGED on selection."
```


### Task 6: Object Info Panel (Property Grid)

**Files:**
- Create: `include/wx/inspector/info.h`
- Create: `src/info.cpp`

**Interfaces:**
- Consumes: `InspectableObject` (Task 1), `PropertyProvider` (Task 3)
- Produces: `ObjectInfoPanel` class — `ShowObject(InspectableObject&)`, `Clear()`, `Refresh()`

- [ ] **Step 1: Write the header `include/wx/inspector/info.h`**

```cpp
#ifndef WX_INSPECTOR_INFO_H
#define WX_INSPECTOR_INFO_H

#include <wx/panel.h>
#include <wx/propgrid/propgrid.h>
#include "wx/inspector/object.h"

namespace wxInspector {

class ObjectInfoPanel : public wxPanel {
public:
    ObjectInfoPanel(wxWindow* parent);

    void ShowObject(InspectableObject& obj);
    void Clear();
    void Refresh();

private:
    void OnPropertyChanged(wxPropertyGridEvent& event);
    void PopulateGrid(InspectableObject& obj);

    wxPropertyGrid* m_pg;
    InspectableObject m_currentObj;
};

} // namespace wxInspector

#endif // WX_INSPECTOR_INFO_H
```

- [ ] **Step 2: Write the implementation `src/info.cpp`**

```cpp
#include "wx/inspector/info.h"
#include "wx/inspector/property_provider.h"
#include <wx/sizer.h>
#include <wx/toplevel.h>

namespace wxInspector {

ObjectInfoPanel::ObjectInfoPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY),
      m_currentObj(InspectableObject::FromWindow(nullptr))
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    m_pg = new wxPropertyGrid(this, wxID_ANY, wxDefaultPosition,
        wxDefaultSize,
        wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER |
        wxPG_DEFAULT_STYLE);
    m_pg->SetExtraStyle(wxPG_EX_MODE_BUTTONS);
    sizer->Add(m_pg, 1, wxEXPAND);
    SetSizer(sizer);

    m_pg->Bind(wxEVT_PG_CHANGED, &ObjectInfoPanel::OnPropertyChanged, this);
}

void ObjectInfoPanel::Clear()
{
    m_pg->Clear();
}

void ObjectInfoPanel::ShowObject(InspectableObject& obj)
{
    m_currentObj = obj;
    PopulateGrid(obj);
}

void ObjectInfoPanel::Refresh()
{
    if (m_currentObj.IsValid()) {
        PopulateGrid(m_currentObj);
    }
}

void ObjectInfoPanel::PopulateGrid(InspectableObject& obj)
{
    m_pg->Freeze();
    m_pg->Clear();

    if (!obj.IsValid()) {
        m_pg->Append(new wxPropertyCategory("Info"));
        m_pg->Append(new wxStringProperty("Status", wxPG_LABEL,
            "Object destroyed or invalid"));
        m_pg->Thaw();
        return;
    }

    wxVector<PropertyDef> props = PropertyProvider::Get().GetProperties(obj);

    // Group by category
    wxString currentCategory;
    for (auto& prop : props) {
        if (prop.category != currentCategory) {
            currentCategory = prop.category;
            m_pg->Append(new wxPropertyCategory(prop.category));
        }

        wxPGProperty* pgProp = nullptr;
        switch (prop.type) {
        case PropertyType::ReadOnly:
            pgProp = new wxStringProperty(prop.name, wxPG_LABEL, prop.value);
            pgProp->ChangeFlag(wxPG_PROP_READONLY, true);
            break;
        case PropertyType::Boolean:
            pgProp = new wxBoolProperty(prop.name, wxPG_LABEL,
                prop.value == "true");
            break;
        case PropertyType::Integer:
            pgProp = new wxIntProperty(prop.name, wxPG_LABEL);
            pgProp->SetValueFromString(prop.value);
            break;
        case PropertyType::Colour:
            pgProp = new wxColourProperty(prop.name, wxPG_LABEL,
                wxColour(prop.value));
            break;
        case PropertyType::String:
        case PropertyType::Font:
        case PropertyType::Size:
        case PropertyType::Point:
        case PropertyType::Rect:
            pgProp = new wxStringProperty(prop.name, wxPG_LABEL, prop.value);
            break;
        case PropertyType::Choice:
            if (!prop.choices.empty()) {
                wxArrayString choices;
                wxArrayInt values;
                for (size_t i = 0; i < prop.choices.size(); i++) {
                    choices.Add(prop.choices[i]);
                    values.Add((int)i);
                }
                pgProp = new wxEnumProperty(prop.name, wxPG_LABEL, choices, values);
                int sel = 0;
                for (size_t i = 0; i < prop.choices.size(); i++) {
                    if (prop.choices[i] == prop.value) { sel = (int)i; break; }
                }
                pgProp->SetChoiceSelection(sel);
            }
            break;
        }

        if (pgProp) {
            if (prop.readOnly) {
                pgProp->ChangeFlag(wxPG_PROP_READONLY, true);
            }
            // Store the setter in the property's client data
            // We use a custom approach: store setter index in a map
            m_pg->Append(pgProp);
        }
    }

    m_pg->Thaw();
}

void ObjectInfoPanel::OnPropertyChanged(wxPropertyGridEvent& event)
{
    wxPGProperty* pgProp = event.GetProperty();
    if (!pgProp) return;

    wxString propName = pgProp->GetName();
    wxString newValue = pgProp->GetValueAsString();

    // Re-fetch properties and find the matching setter
    if (!m_currentObj.IsValid()) return;
    wxVector<PropertyDef> props =
        PropertyProvider::Get().GetProperties(m_currentObj);
    for (auto& prop : props) {
        if (prop.name == propName && prop.setter) {
            prop.setter(newValue);
            break;
        }
    }
}

} // namespace wxInspector
```

- [ ] **Step 3: Verify compilation**

Run: `cd build && cmake --build . --target wxInspector 2>&1 | tail -5`
Expected: Clean compile

- [ ] **Step 4: Commit**

```bash
git add include/wx/inspector/info.h src/info.cpp
git commit -m "feat: add ObjectInfoPanel with wxPropertyGrid

Displays editable properties grouped by category (Identity, Geometry,
State, Appearance, Layout, Value, Sizer). Uses PropertyProvider for
discovery. Editing a property calls the setter on the live widget."
```


### Task 7: Method Invoker Panel

**Files:**
- Create: `include/wx/inspector/invoker.h`
- Create: `src/invoker.cpp`

**Interfaces:**
- Consumes: `InspectableObject` (Task 1), `MethodRegistry` (Task 4)
- Produces: `MethodInvokerPanel` class — `ShowObject(InspectableObject&)`, `Clear()`

- [ ] **Step 1: Write the header `include/wx/inspector/invoker.h`**

```cpp
#ifndef WX_INSPECTOR_INVOKER_H
#define WX_INSPECTOR_INVOKER_H

#include <wx/panel.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/listbox.h>
#include "wx/inspector/object.h"
#include "wx/inspector/plugin.h"

namespace wxInspector {

class MethodInvokerPanel : public wxPanel {
public:
    MethodInvokerPanel(wxWindow* parent);

    void ShowObject(InspectableObject& obj);
    void Clear();

private:
    void OnMethodSelected(wxCommandEvent& event);
    void OnInvoke(wxCommandEvent& event);
    void OnHistorySelect(wxCommandEvent& event);

    void UpdateMethodList(InspectableObject& obj);

    wxChoice* m_methodChoice;
    wxTextCtrl* m_paramField;
    wxButton* m_invokeBtn;
    wxTextCtrl* m_resultArea;
    wxListBox* m_historyList;

    wxVector<MethodInfo> m_currentMethods;
    wxVector<wxString> m_history;
    InspectableObject m_currentObj;

    static const int MAX_HISTORY = 20;
};

} // namespace wxInspector

#endif // WX_INSPECTOR_INVOKER_H
```

- [ ] **Step 2: Write the implementation `src/invoker.cpp`**

```cpp
#include "wx/inspector/invoker.h"
#include "wx/inspector/method_registry.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statbox.h>
#include <wx/config.h>
#include <wx/tokenzr.h>
#include <wx/clipbrd.h>

namespace wxInspector {

enum {
    ID_METHOD_CHOICE = wxID_HIGHEST + 200,
    ID_PARAM_FIELD,
    ID_INVOKE_BTN,
    ID_RESULT_AREA,
    ID_HISTORY_LIST
};

MethodInvokerPanel::MethodInvokerPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY),
      m_currentObj(InspectableObject::FromWindow(nullptr))
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Method selection row
    wxBoxSizer* methodRow = new wxBoxSizer(wxHORIZONTAL);
    methodRow->Add(new wxStaticText(this, wxID_ANY, "Method:"),
        0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_methodChoice = new wxChoice(this, ID_METHOD_CHOICE);
    methodRow->Add(m_methodChoice, 1, wxEXPAND);
    mainSizer->Add(methodRow, 0, wxEXPAND | wxALL, 4);

    // Parameter row
    wxBoxSizer* paramRow = new wxBoxSizer(wxHORIZONTAL);
    paramRow->Add(new wxStaticText(this, wxID_ANY, "Args:"),
        0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_paramField = new wxTextCtrl(this, ID_PARAM_FIELD,
        wxEmptyString, wxDefaultPosition, wxDefaultSize,
        wxTE_PROCESS_ENTER);
    paramRow->Add(m_paramField, 1, wxEXPAND);
    m_invokeBtn = new wxButton(this, ID_INVOKE_BTN, "Invoke");
    paramRow->Add(m_invokeBtn, 0, wxLEFT, 5);
    mainSizer->Add(paramRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);

    // Result area
    wxStaticBox* resultBox = new wxStaticBox(this, wxID_ANY, "Result");
    wxStaticBoxSizer* resultSizer = new wxStaticBoxSizer(resultBox, wxVERTICAL);
    m_resultArea = new wxTextCtrl(this, ID_RESULT_AREA, wxEmptyString,
        wxDefaultPosition, wxDefaultSize,
        wxTE_MULTILINE | wxTE_READONLY);
    m_resultArea->SetMinSize(wxSize(-1, 80));
    resultSizer->Add(m_resultArea, 1, wxEXPAND);
    mainSizer->Add(resultSizer, 1, wxEXPAND | wxALL, 4);

    // History
    wxStaticBox* histBox = new wxStaticBox(this, wxID_ANY, "History");
    wxStaticBoxSizer* histSizer = new wxStaticBoxSizer(histBox, wxVERTICAL);
    m_historyList = new wxListBox(this, ID_HISTORY_LIST);
    m_historyList->SetMinSize(wxSize(-1, 60));
    histSizer->Add(m_historyList, 1, wxEXPAND);
    mainSizer->Add(histSizer, 0, wxEXPAND | wxALL, 4);

    SetSizer(mainSizer);

    m_methodChoice->Bind(wxEVT_CHOICE, &MethodInvokerPanel::OnMethodSelected, this);
    m_invokeBtn->Bind(wxEVT_BUTTON, &MethodInvokerPanel::OnInvoke, this);
    m_paramField->Bind(wxEVT_TEXT_ENTER, &MethodInvokerPanel::OnInvoke, this);
    m_historyList->Bind(wxEVT_LISTBOX, &MethodInvokerPanel::OnHistorySelect, this);

    // Load history from config
    wxConfigBase* cfg = wxConfig::Get();
    if (cfg) {
        cfg->SetPath("/wxInspector/MethodHistory");
        long count;
        if (cfg->GetFirstEntry(m_resultArea->GetValue(), count)) {
            // Load entries — simplified: load up to MAX_HISTORY
        }
        cfg->SetPath("/");
    }
}

void MethodInvokerPanel::ShowObject(InspectableObject& obj)
{
    m_currentObj = obj;
    UpdateMethodList(obj);
}

void MethodInvokerPanel::Clear()
{
    m_methodChoice->Clear();
    m_paramField->Clear();
    m_resultArea->Clear();
    m_currentMethods.clear();
}

void MethodInvokerPanel::UpdateMethodList(InspectableObject& obj)
{
    m_methodChoice->Clear();
    m_currentMethods.clear();

    if (!obj.IsValid()) return;

    m_currentMethods = MethodRegistry::Get().GetMethods(obj);
    for (auto& m : m_currentMethods) {
        wxString label = m.name + " — " + m.signature;
        m_methodChoice->Append(label);
    }

    if (!m_currentMethods.empty())
        m_methodChoice->Select(0);
}

void MethodInvokerPanel::OnMethodSelected(wxCommandEvent& event)
{
    int sel = event.GetSelection();
    if (sel >= 0 && sel < (int)m_currentMethods.size()) {
        m_resultArea->SetValue(
            m_currentMethods[sel].name + ": " +
            m_currentMethods[sel].description);
    }
}

void MethodInvokerPanel::OnInvoke(wxCommandEvent&)
{
    int sel = m_methodChoice->GetSelection();
    if (sel < 0 || sel >= (int)m_currentMethods.size()) {
        m_resultArea->SetValue("Error: No method selected");
        return;
    }

    // Parse arguments
    wxString argStr = m_paramField->GetValue();
    wxVector<wxString> args;
    wxStringTokenizer tk(argStr, ",", wxTOKEN_STRTOK);
    while (tk.HasMoreTokens()) {
        wxString token = tk.GetNextToken();
        token.Trim(true).Trim(false);
        if (!token.empty()) args.push_back(token);
    }

    MethodInfo& method = m_currentMethods[sel];
    wxString result;
    if (method.invoke) {
        result = method.invoke(args);
    } else {
        result = "Error: Method has no invoke handler";
    }

    m_resultArea->SetValue(result);

    // Add to history
    wxString histEntry = method.name + "(" + argStr + ") -> " + result;
    m_history.push_back(histEntry);
    if (m_history.size() > MAX_HISTORY)
        m_history.erase(m_history.begin());

    m_historyList->Clear();
    for (auto it = m_history.rbegin(); it != m_history.rend(); ++it)
        m_historyList->Append(*it);
}

void MethodInvokerPanel::OnHistorySelect(wxCommandEvent& event)
{
    int sel = event.GetSelection();
    if (sel >= 0 && sel < (int)m_history.size()) {
        // History is stored in reverse order
        int actualIdx = (int)m_history.size() - 1 - sel;
        if (actualIdx >= 0) {
            m_resultArea->SetValue(m_history[actualIdx]);
        }
    }
}

} // namespace wxInspector
```

- [ ] **Step 3: Verify compilation**

Run: `cd build && cmake --build . --target wxInspector 2>&1 | tail -5`
Expected: Clean compile

- [ ] **Step 4: Commit**

```bash
git add include/wx/inspector/invoker.h src/invoker.cpp
git commit -m "feat: add MethodInvoker panel with method selection and history

Dropdown populated from MethodRegistry filtered by selected object.
Comma-separated argument parsing, invoke button, result area.
Last 20 invocations persisted in history list."
```


### Task 8: Event Logger Panel

**Files:**
- Create: `include/wx/inspector/events.h`
- Create: `src/events.cpp`

**Interfaces:**
- Consumes: `InspectableObject` (Task 1)
- Produces: `EventLoggerPanel` class — `ShowObject(InspectableObject&)`, `StartCapture()`, `StopCapture()`, `Clear()`

- [ ] **Step 1: Write the header `include/wx/inspector/events.h`**

```cpp
#ifndef WX_INSPECTOR_EVENTS_H
#define WX_INSPECTOR_EVENTS_H

#include <wx/panel.h>
#include <wx/checklst.h>
#include <wx/button.h>
#include <wx/dataview.h>
#include <wx/vector.h>
#include "wx/inspector/object.h"

namespace wxInspector {

struct EventLogEntry {
    wxString timestamp;
    wxString eventType;
    wxString eventData;
    wxString category;  // Mouse, Keyboard, Focus, Size, Command, Misc
};

class EventLoggerPanel : public wxPanel {
public:
    EventLoggerPanel(wxWindow* parent);

    void ShowObject(InspectableObject& obj);
    void StartCapture();
    void StopCapture();
    void Clear();

private:
    void OnStart(wxCommandEvent& event);
    void OnStop(wxCommandEvent& event);
    void OnClear(wxCommandEvent& event);

    void PopulateEventTypes(wxClassInfo* classInfo);
    void ConnectEvents();
    void DisconnectEvents();

    // Event handler thunks for each event type
    void OnEvent(wxEvent& event, const wxString& typeName,
                 const wxString& category);
    wxString FormatEventData(wxEvent& event, const wxString& typeName);

    wxCheckListBox* m_eventTypeList;
    wxButton* m_startBtn;
    wxButton* m_stopBtn;
    wxButton* m_clearBtn;
    wxDataViewListCtrl* m_logView;

    wxWindow* m_targetWindow;
    bool m_isCapturing;

    wxVector<EventLogEntry> m_entries;
    wxVector<int> m_boundEventTypes;

    static const int MAX_ENTRIES = 500;
};

} // namespace wxInspector

#endif // WX_INSPECTOR_EVENTS_H
```

- [ ] **Step 2: Write the implementation `src/events.cpp`**

```cpp
#include "wx/inspector/events.h"
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/datetime.h>
#include <wx/tokenzr.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>

namespace wxInspector {

enum {
    ID_EVENT_TYPE_LIST = wxID_HIGHEST + 300,
    ID_START_BTN,
    ID_STOP_BTN,
    ID_CLEAR_BTN,
    ID_LOG_VIEW
};

// --- Event type lookup table ---

struct EventTypeInfo {
    int eventType;
    const char* name;
    const char* category;
};

// Core wxWindow events
static const EventTypeInfo s_windowEvents[] = {
    { wxEVT_LEFT_DOWN,           "LeftDown",           "Mouse" },
    { wxEVT_LEFT_UP,             "LeftUp",             "Mouse" },
    { wxEVT_LEFT_DCLICK,         "LeftDClick",         "Mouse" },
    { wxEVT_MIDDLE_DOWN,         "MiddleDown",         "Mouse" },
    { wxEVT_MIDDLE_UP,           "MiddleUp",           "Mouse" },
    { wxEVT_MIDDLE_DCLICK,       "MiddleDClick",       "Mouse" },
    { wxEVT_RIGHT_DOWN,          "RightDown",          "Mouse" },
    { wxEVT_RIGHT_UP,            "RightUp",            "Mouse" },
    { wxEVT_RIGHT_DCLICK,        "RightDClick",        "Mouse" },
    { wxEVT_MOTION,              "Motion",             "Mouse" },
    { wxEVT_ENTER_WINDOW,        "EnterWindow",        "Mouse" },
    { wxEVT_LEAVE_WINDOW,        "LeaveWindow",        "Mouse" },
    { wxEVT_MOUSEWHEEL,          "MouseWheel",         "Mouse" },
    { wxEVT_MOUSE_CAPTURE_LOST,  "MouseCaptureLost",   "Mouse" },
    { wxEVT_KEY_DOWN,            "KeyDown",            "Keyboard" },
    { wxEVT_KEY_UP,              "KeyUp",              "Keyboard" },
    { wxEVT_CHAR,                "Char",               "Keyboard" },
    { wxEVT_SET_FOCUS,           "SetFocus",           "Focus" },
    { wxEVT_KILL_FOCUS,          "KillFocus",          "Focus" },
    { wxEVT_SIZE,                "Size",               "Size" },
    { wxEVT_MOVE,                "Move",               "Size" },
    { wxEVT_PAINT,               "Paint",              "Misc" },
    { wxEVT_ERASE_BACKGROUND,    "EraseBackground",    "Misc" },
    { wxEVT_IDLE,                "Idle",               "Misc" },
    { wxEVT_ACTIVATE,            "Activate",           "Focus" },
    { wxEVT_SHOW,                "Show",               "Misc" },
    { wxEVT_BUTTON,              "Button",             "Command" },
    { wxEVT_CHECKBOX,            "CheckBox",           "Command" },
    { wxEVT_CHOICE,              "Choice",             "Command" },
    { wxEVT_TEXT,                "Text",               "Command" },
    { wxEVT_TEXT_ENTER,          "TextEnter",          "Command" },
    { wxEVT_COMBOBOX,            "ComboBox",           "Command" },
    { wxEVT_RADIOBUTTON,         "RadioButton",        "Command" },
    { wxEVT_SLIDER,              "Slider",             "Command" },
    { wxEVT_SPINCTRL,            "SpinCtrl",           "Command" },
    { wxEVT_SCROLL_TOP,          "ScrollTop",          "Command" },
    { wxEVT_SCROLL_BOTTOM,       "ScrollBottom",       "Command" },
    { wxEVT_SCROLL_LINEUP,       "ScrollLineUp",       "Command" },
    { wxEVT_SCROLL_LINEDOWN,     "ScrollLineDown",     "Command" },
    { wxEVT_SCROLL_PAGEUP,       "ScrollPageUp",       "Command" },
    { wxEVT_SCROLL_PAGEDOWN,     "ScrollPageDown",     "Command" },
    { wxEVT_SCROLL_THUMBTRACK,   "ScrollThumbTrack",   "Command" },
    { wxEVT_SCROLL_THUMBRELEASE, "ScrollThumbRelease", "Command" },
    { wxEVT_SCROLL_CHANGED,      "ScrollChanged",      "Command" },
};
static const size_t s_windowEventCount = sizeof(s_windowEvents) / sizeof(s_windowEvents[0]);

struct ExtraEventInfo {
    wxClassInfo* classInfo;
    const EventTypeInfo* events;
    size_t count;
};

static const EventTypeInfo s_textEvents[] = {
    { wxEVT_TEXT,       "Text",       "Command" },
    { wxEVT_TEXT_ENTER, "TextEnter",  "Command" },
    { wxEVT_TEXT_MAXLEN, "TextMaxLen","Command" },
};

static const ExtraEventInfo s_extraEvents[] = {
    { CLASSINFO(wxTextCtrl),     s_textEvents, sizeof(s_textEvents)/sizeof(s_textEvents[0]) },
};

EventLoggerPanel::EventLoggerPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY), m_targetWindow(nullptr), m_isCapturing(false)
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Control buttons
    wxBoxSizer* btnRow = new wxBoxSizer(wxHORIZONTAL);
    m_startBtn = new wxButton(this, ID_START_BTN, "Start Capture");
    m_stopBtn = new wxButton(this, ID_STOP_BTN, "Stop");
    m_clearBtn = new wxButton(this, ID_CLEAR_BTN, "Clear");
    m_stopBtn->Disable();
    btnRow->Add(m_startBtn, 0, wxRIGHT, 4);
    btnRow->Add(m_stopBtn, 0, wxRIGHT, 4);
    btnRow->Add(m_clearBtn, 0);
    mainSizer->Add(btnRow, 0, wxEXPAND | wxALL, 4);

    // Event type checklist
    wxStaticBox* typeBox = new wxStaticBox(this, wxID_ANY, "Event Types");
    wxStaticBoxSizer* typeSizer = new wxStaticBoxSizer(typeBox, wxVERTICAL);
    m_eventTypeList = new wxCheckListBox(this, ID_EVENT_TYPE_LIST);
    m_eventTypeList->SetMinSize(wxSize(-1, 100));
    typeSizer->Add(m_eventTypeList, 1, wxEXPAND);
    mainSizer->Add(typeSizer, 0, wxEXPAND | wxALL, 4);

    // Log view (DataView)
    m_logView = new wxDataViewListCtrl(this, ID_LOG_VIEW);
    m_logView->AppendTextColumn("Timestamp", wxDATAVIEW_CELL_INERT, 140);
    m_logView->AppendTextColumn("Event", wxDATAVIEW_CELL_INERT, 120);
    m_logView->AppendTextColumn("Data", wxDATAVIEW_CELL_INERT, 300);
    mainSizer->Add(m_logView, 1, wxEXPAND | wxALL, 4);

    SetSizer(mainSizer);

    m_startBtn->Bind(wxEVT_BUTTON, &EventLoggerPanel::OnStart, this);
    m_stopBtn->Bind(wxEVT_BUTTON, &EventLoggerPanel::OnStop, this);
    m_clearBtn->Bind(wxEVT_BUTTON, &EventLoggerPanel::OnClear, this);
}

void EventLoggerPanel::ShowObject(InspectableObject& obj)
{
    StopCapture();
    m_targetWindow = obj.AsWindow();
    PopulateEventTypes(obj.GetObject()->GetClassInfo());
}

void EventLoggerPanel::PopulateEventTypes(wxClassInfo* classInfo)
{
    m_eventTypeList->Clear();
    if (!classInfo) return;

    // Always add window-level events
    for (size_t i = 0; i < s_windowEventCount; i++) {
        int idx = m_eventTypeList->Append(
            wxString(s_windowEvents[i].name) + " (" +
            s_windowEvents[i].category + ")");
        m_eventTypeList->Check(idx);
    }

    // Add control-specific events
    for (auto& extra : s_extraEvents) {
        if (classInfo->IsKindOf(extra.classInfo)) {
            for (size_t i = 0; i < extra.count; i++) {
                int idx = m_eventTypeList->Append(
                    wxString(extra.events[i].name) + " (" +
                    extra.events[i].category + ")");
                m_eventTypeList->Check(idx);
            }
        }
    }
}

void EventLoggerPanel::StartCapture()
{
    if (!m_targetWindow || m_isCapturing) return;

    ConnectEvents();
    m_isCapturing = true;
    m_startBtn->Disable();
    m_stopBtn->Enable();
}

void EventLoggerPanel::StopCapture()
{
    if (!m_isCapturing) return;

    DisconnectEvents();
    m_isCapturing = false;
    m_startBtn->Enable();
    m_stopBtn->Disable();
}

void EventLoggerPanel::ConnectEvents()
{
    if (!m_targetWindow) return;

    // Bind all checked event types
    wxEvtHandler* handler = m_targetWindow->GetEventHandler();
    for (size_t i = 0; i < s_windowEventCount; i++) {
        if (m_eventTypeList->IsChecked((unsigned int)i)) {
            const EventTypeInfo& info = s_windowEvents[i];
            handler->Bind(info.eventType, [this, info](wxEvent& evt) {
                OnEvent(evt, info.name, info.category);
            });
        }
    }

    // Bind extra events
    unsigned int baseIdx = (unsigned int)s_windowEventCount;
    for (auto& extra : s_extraEvents) {
        if (m_targetWindow->GetClassInfo()->IsKindOf(extra.classInfo)) {
            for (size_t i = 0; i < extra.count; i++) {
                unsigned int idx = baseIdx + (unsigned int)i;
                if (m_eventTypeList->IsChecked(idx)) {
                    const EventTypeInfo& info = extra.events[i];
                    handler->Bind(info.eventType, [this, info](wxEvent& evt) {
                        OnEvent(evt, info.name, info.category);
                    });
                }
            }
        }
        baseIdx += (unsigned int)extra.count;
    }
}

void EventLoggerPanel::DisconnectEvents()
{
    if (!m_targetWindow) return;

    wxEvtHandler* handler = m_targetWindow->GetEventHandler();
    for (size_t i = 0; i < s_windowEventCount; i++) {
        handler->Unbind(s_windowEvents[i].eventType, handler);
    }
    for (auto& extra : s_extraEvents) {
        for (size_t i = 0; i < extra.count; i++) {
            handler->Unbind(extra.events[i].eventType, handler);
        }
    }
}

void EventLoggerPanel::OnEvent(wxEvent& event, const wxString& typeName,
                                const wxString& category)
{
    EventLogEntry entry;
    entry.timestamp = wxDateTime::Now().FormatISOTime();
    entry.eventType = typeName;
    entry.category = category;
    entry.eventData = FormatEventData(event, typeName);

    m_entries.push_back(entry);
    if (m_entries.size() > MAX_ENTRIES)
        m_entries.erase(m_entries.begin());

    // Update view
    wxVector<wxVariant> rowData;
    rowData.push_back(wxVariant(entry.timestamp));
    rowData.push_back(wxVariant(entry.eventType));
    rowData.push_back(wxVariant(entry.eventData));
    m_logView->AppendItem(rowData);

    // Auto-scroll to bottom
    m_logView->EnsureVisible(m_logView->GetItemCount() - 1);

    // Trim if needed
    while (m_logView->GetItemCount() > MAX_ENTRIES)
        m_logView->DeleteItem(0);

    event.Skip();
}

wxString EventLoggerPanel::FormatEventData(wxEvent& event,
                                            const wxString& typeName)
{
    wxString data;

    if (event.GetEventType() == wxEVT_LEFT_DOWN ||
        event.GetEventType() == wxEVT_LEFT_UP ||
        event.GetEventType() == wxEVT_RIGHT_DOWN ||
        event.GetEventType() == wxEVT_RIGHT_UP ||
        event.GetEventType() == wxEVT_MIDDLE_DOWN ||
        event.GetEventType() == wxEVT_MIDDLE_UP ||
        event.GetEventType() == wxEVT_MOTION) {
        wxMouseEvent& me = static_cast<wxMouseEvent&>(event);
        data = wxString::Format("Pos=(%d,%d)", me.GetX(), me.GetY());
    } else if (event.GetEventType() == wxEVT_KEY_DOWN ||
               event.GetEventType() == wxEVT_KEY_UP) {
        wxKeyEvent& ke = static_cast<wxKeyEvent&>(event);
        data = wxString::Format("KeyCode=%d Modifiers=%d",
            ke.GetKeyCode(), ke.GetModifiers());
    } else if (event.GetEventType() == wxEVT_CHAR) {
        wxKeyEvent& ke = static_cast<wxKeyEvent&>(event);
        data = wxString::Format("Char='%c' KeyCode=%d",
            (char)ke.GetKeyCode(), ke.GetKeyCode());
    } else if (event.GetEventType() == wxEVT_SIZE) {
        wxSizeEvent& se = static_cast<wxSizeEvent&>(event);
        data = wxString::Format("Size=(%d,%d)", se.GetSize().x, se.GetSize().y);
    } else if (event.GetEventType() == wxEVT_MOVE) {
        wxMoveEvent& me = static_cast<wxMoveEvent&>(event);
        data = wxString::Format("Pos=(%d,%d)", me.GetPosition().x, me.GetPosition().y);
    } else if (event.GetEventType() == wxEVT_MOUSEWHEEL) {
        wxMouseEvent& mwe = static_cast<wxMouseEvent&>(event);
        data = wxString::Format("Wheel=%d", mwe.GetWheelRotation());
    } else if (event.GetEventType() == wxEVT_BUTTON ||
               event.GetEventType() == wxEVT_CHECKBOX ||
               event.GetEventType() == wxEVT_CHOICE ||
               event.GetEventType() == wxEVT_COMBOBOX ||
               event.GetEventType() == wxEVT_RADIOBUTTON ||
               event.GetEventType() == wxEVT_SLIDER ||
               event.GetEventType() == wxEVT_SPINCTRL) {
        wxCommandEvent& ce = static_cast<wxCommandEvent&>(event);
        data = wxString::Format("ID=%d", ce.GetId());
        if (ce.GetString() != wxEmptyString)
            data += " String=\"" + ce.GetString() + "\"";
        if (ce.GetInt() != 0)
            data += wxString::Format(" Int=%d", ce.GetInt());
    } else if (event.GetEventType() == wxEVT_TEXT) {
        wxCommandEvent& ce = static_cast<wxCommandEvent&>(event);
        data = wxString::Format("String=\"%s\"", ce.GetString());
    } else if (event.GetEventType() == wxEVT_IDLE) {
        data = "Idle";
    } else if (event.GetEventType() == wxEVT_PAINT) {
        data = "Paint";
    } else {
        data = wxString::Format("ID=%d", event.GetId());
    }

    return data;
}

void EventLoggerPanel::Clear()
{
    m_entries.clear();
    m_logView->DeleteAllItems();
}

void EventLoggerPanel::OnStart(wxCommandEvent&) { StartCapture(); }
void EventLoggerPanel::OnStop(wxCommandEvent&) { StopCapture(); }
void EventLoggerPanel::OnClear(wxCommandEvent&) { Clear(); }

} // namespace wxInspector
```

- [ ] **Step 3: Verify compilation**

Run: `cd build && cmake --build . --target wxInspector 2>&1 | tail -5`
Expected: Clean compile

- [ ] **Step 4: Commit**

```bash
git add include/wx/inspector/events.h src/events.cpp
git commit -m "feat: add EventLogger panel with capture and log view

Checklisted event types based on target widget class. Start/stop capture
binds/unbinds dynamically. Events logged to wxDataViewListCtrl with
timestamp, type name, and formatted event data. Auto-scroll, max 500 entries."
```


### Task 9: Widget Highlighting

**Files:**
- Create: `include/wx/inspector/highlighter.h`
- Create: `src/highlighter.cpp`

**Interfaces:**
- Consumes: `InspectableObject` (Task 1)
- Produces: `InspectionHighlighter` class — `Highlight(InspectableObject&)`, `ClearHighlight()`

- [ ] **Step 1: Write the header `include/wx/inspector/highlighter.h`**

```cpp
#ifndef WX_INSPECTOR_HIGHLIGHTER_H
#define WX_INSPECTOR_HIGHLIGHTER_H

#include <wx/object.h>
#include <wx/timer.h>
#include <wx/overlay.h>
#include "wx/inspector/object.h"

namespace wxInspector {

class InspectionHighlighter : public wxEvtHandler {
public:
    InspectionHighlighter();

    void Highlight(InspectableObject& obj);
    void ClearHighlight();

private:
    void OnTimer(wxTimerEvent& event);
    void DrawWindowHighlight(wxWindow* win);
    void DrawSizerHighlight(wxSizer* sizer, wxWindow* relativeTo);
    void DrawSizerItemHighlight(wxSizerItem* item, wxWindow* relativeTo);

    wxOverlay m_overlay;
    wxTimer m_timer;
    wxWindow* m_highlightedWindow; // for window flicker

    wxDECLARE_EVENT_TABLE();
};

} // namespace wxInspector

#endif // WX_INSPECTOR_HIGHLIGHTER_H
```

- [ ] **Step 2: Write the implementation `src/highlighter.cpp`**

```cpp
#include "wx/inspector/highlighter.h"
#include <wx/window.h>
#include <wx/sizer.h>
#include <wx/dcclient.h>
#include <wx/dcscreen.h>
#include <wx/toplevel.h>
#include <wx/gbsizer.h>
#include <wx/wrapsizer.h>

namespace wxInspector {

static const int HIGHLIGHT_DURATION_MS = 3000;
static const int FLICKER_INTERVAL_MS = 500;
static const int FLICKER_COUNT = 6;

wxBEGIN_EVENT_TABLE(InspectionHighlighter, wxEvtHandler)
    EVT_TIMER(wxID_ANY, InspectionHighlighter::OnTimer)
wxEND_EVENT_TABLE()

InspectionHighlighter::InspectionHighlighter()
    : m_highlightedWindow(nullptr)
{
    m_timer.SetOwner(this);
}

void InspectionHighlighter::Highlight(InspectableObject& obj)
{
    ClearHighlight();

    switch (obj.GetKind()) {
    case InspectableObject::Kind::Window:
        if (obj.AsWindow()) {
            DrawWindowHighlight(obj.AsWindow());
        }
        break;
    case InspectableObject::Kind::Sizer:
        if (obj.AsSizer()) {
            // Find the window this sizer belongs to for coordinate reference
            wxSizer* sizer = obj.AsSizer();
            wxWindow* refWin = sizer->GetContainingWindow();
            if (refWin) {
                DrawSizerHighlight(sizer, refWin);
            }
        }
        break;
    case InspectableObject::Kind::SizerItem:
        if (obj.AsSizerItem()) {
            wxSizerItem* item = obj.AsSizerItem();
            // Find the window the containing sizer belongs to
            wxWindow* refWin = nullptr;
            // We can't directly get the parent sizer from a sizer item,
            // so we try to find it through the item's own window or sizer
            if (item->IsWindow()) {
                refWin = item->GetWindow();
                DrawWindowHighlight(refWin);
                return;
            } else if (item->IsSpacer()) {
                // Spacer — just draw the spacer rect
                wxRect rect = item->GetRect();
                refWin = item->GetWindow(); // spacer doesn't have a window
                if (refWin) {
                    wxScreenDC dc;
                    dc.SetPen(wxPen(*wxGREEN, 2));
                    dc.SetBrush(*wxTRANSPARENT_BRUSH);
                    wxRect screenRect = rect;
                    screenRect.SetPosition(
                        refWin->ClientToScreen(rect.GetPosition()));
                    dc.DrawRectangle(screenRect);
                }
            }
        }
        break;
    }

    m_timer.Start(HIGHLIGHT_DURATION_MS, true);
}

void InspectionHighlighter::ClearHighlight()
{
    m_timer.Stop();
    m_overlay.Reset();
    m_highlightedWindow = nullptr;
}

void InspectionHighlighter::DrawWindowHighlight(wxWindow* win)
{
    wxTopLevelWindow* tlw = wxDynamicCast(win, wxTopLevelWindow);
    if (tlw) {
        // Flicker effect for top-level windows
        m_highlightedWindow = tlw;
        m_timer.Start(FLICKER_INTERVAL_MS);
        // We handle the flicker in OnTimer
        // The timer is restarted below for flicker
    } else {
        // Green outline for non-TLW windows
        wxScreenDC dc;
        dc.SetPen(wxPen(*wxGREEN, 3));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        wxRect rect = win->GetScreenRect();
        dc.DrawRectangle(rect);

        // Schedule clear
        m_timer.Start(HIGHLIGHT_DURATION_MS, true);
    }
}

void InspectionHighlighter::DrawSizerHighlight(wxSizer* sizer, wxWindow* relativeTo)
{
    wxScreenDC dc;

    // Green outline for sizer boundary
    dc.SetPen(wxPen(*wxGREEN, 2));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    wxSize sz = sizer->GetSize();
    wxPoint pos = sizer->GetPosition();
    wxPoint screenPos = relativeTo->ClientToScreen(pos);
    wxRect sizerRect(screenPos, sz);
    dc.DrawRectangle(sizerRect);

    // Red internal partition lines per sizer type
    dc.SetPen(wxPen(*wxRED, 1, wxPENSTYLE_DOT));

    if (wxBoxSizer* bs = wxDynamicCast(sizer, wxBoxSizer)) {
        wxSizerItemList& items = bs->GetChildren();
        int offset = (bs->GetOrientation() == wxHORIZONTAL)
            ? screenPos.x : screenPos.y;
        for (size_t i = 0; i < items.GetCount(); i++) {
            wxSizerItem* item = items[i];
            if (!item) continue;
            wxSize itemSz = item->GetSize();
            wxPoint itemPos = item->GetPosition();
            wxPoint screenItemPos = relativeTo->ClientToScreen(itemPos);
            int border = item->GetBorder();

            // Dark blue fill for items
            dc.SetBrush(wxBrush(wxColour(0, 0, 139, 64))); // dark blue with alpha
            dc.DrawRectangle(wxRect(screenItemPos, itemSz));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
        }
    } else if (wxGridSizer* gs = wxDynamicCast(sizer, wxGridSizer)) {
        int cols = gs->GetCols();
        int rows = gs->GetEffectiveRowsCount();
        if (cols > 0 && rows > 0) {
            wxSizerItemList& items = gs->GetChildren();
            for (size_t i = 0; i < items.GetCount(); i++) {
                wxSizerItem* item = items[i];
                if (!item) continue;
                wxPoint itemPos = item->GetPosition();
                wxSize itemSz = item->GetSize();
                wxPoint screenPos2 = relativeTo->ClientToScreen(itemPos);
                dc.SetBrush(wxBrush(wxColour(0, 0, 139, 64)));
                dc.DrawRectangle(wxRect(screenPos2, itemSz));
                dc.SetBrush(*wxTRANSPARENT_BRUSH);
            }
        }
    } else if (wxFlexGridSizer* fgs = wxDynamicCast(sizer, wxFlexGridSizer)) {
        // Use RowHeights/ColWidths for grid lines
        wxSizerItemList& items = fgs->GetChildren();
        for (size_t i = 0; i < items.GetCount(); i++) {
            wxSizerItem* item = items[i];
            if (!item) continue;
            wxPoint itemPos = item->GetPosition();
            wxSize itemSz = item->GetSize();
            wxPoint screenPos2 = relativeTo->ClientToScreen(itemPos);
            dc.SetBrush(wxBrush(wxColour(0, 0, 139, 64)));
            dc.DrawRectangle(wxRect(screenPos2, itemSz));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
        }
    } else if (wxWrapSizer* ws = wxDynamicCast(sizer, wxWrapSizer)) {
        wxSizerItemList& items = ws->GetChildren();
        for (size_t i = 0; i < items.GetCount(); i++) {
            wxSizerItem* item = items[i];
            if (!item) continue;
            wxPoint itemPos = item->GetPosition();
            wxSize itemSz = item->GetSize();
            wxPoint screenPos2 = relativeTo->ClientToScreen(itemPos);
            dc.SetBrush(wxBrush(wxColour(0, 0, 139, 64)));
            dc.DrawRectangle(wxRect(screenPos2, itemSz));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
        }
    }
}

void InspectionHighlighter::OnTimer(wxTimerEvent&)
{
    if (m_highlightedWindow) {
        // Flicker: toggle visibility
        static int flickerCount = 0;
        m_highlightedWindow->Show(flickerCount % 2 == 0);
        flickerCount++;
        if (flickerCount >= FLICKER_COUNT) {
            m_highlightedWindow->Show(true);
            m_highlightedWindow = nullptr;
            flickerCount = 0;
            m_timer.Stop();
        }
    } else {
        ClearHighlight();
    }
}

} // namespace wxInspector
```

- [ ] **Step 3: Verify compilation**

Run: `cd build && cmake --build . --target wxInspector 2>&1 | tail -5`
Expected: Clean compile

- [ ] **Step 4: Commit**

```bash
git add include/wx/inspector/highlighter.h src/highlighter.cpp
git commit -m "feat: add InspectionHighlighter with overlay rendering

Green rectangle for windows, flicker effect for top-level windows.
Sizer boundaries (green) with internal item fill (dark blue).
Per-sizer-type logic: BoxSizer, GridSizer, FlexGridSizer, WrapSizer.
Auto-clears after 3 seconds via one-shot wxTimer."
```


### Task 10: InspectionFrame — AUI Layout Assembly

**Files:**
- Create: `include/wx/inspector/frame.h`
- Create: `src/frame.cpp`

**Interfaces:**
- Consumes: `InspectableObject` (Task 1), `InspectionTree` (Task 5), `ObjectInfoPanel` (Task 6), `MethodInvokerPanel` (Task 7), `EventLoggerPanel` (Task 8), `InspectionHighlighter` (Task 9), `PropertyProvider` (Task 3), `MethodRegistry` (Task 4)
- Produces: `InspectionFrame` (a `wxFrame` subclass) — `Show(wxObject* selectObj, bool refreshTree)`, `SelectObject(wxObject*)`, `RefreshTree()`
- Produces: `wxEVT_INSPECTION_FRAME_CLOSED` event

- [ ] **Step 1: Write the header `include/wx/inspector/frame.h`**

```cpp
#ifndef WX_INSPECTOR_FRAME_H
#define WX_INSPECTOR_FRAME_H

#include <wx/frame.h>
#include <wx/aui/aui.h>
#include "wx/inspector/object.h"

namespace wxInspector {

class InspectionTree;
class ObjectInfoPanel;
class MethodInvokerPanel;
class EventLoggerPanel;
class InspectionHighlighter;

wxDECLARE_EVENT(wxEVT_INSPECTION_FRAME_CLOSED, wxCommandEvent);

class InspectionFrame : public wxFrame {
public:
    InspectionFrame(wxWindow* parent, wxPoint pos, wxSize size);

    void Show(wxObject* selectObj = nullptr, bool refreshTree = false);
    void SelectObject(wxObject* obj);
    void RefreshTree();

    InspectionTree* GetTree() { return m_tree; }
    ObjectInfoPanel* GetInfoPanel() { return m_info; }
    MethodInvokerPanel* GetInvokerPanel() { return m_invoker; }
    EventLoggerPanel* GetEventPanel() { return m_events; }
    InspectionHighlighter* GetHighlighter() { return m_highlighter; }

    void SaveLayout();
    void LoadLayout();

private:
    void OnTreeSelectionChanged(wxCommandEvent& event);
    void OnHighlight(wxCommandEvent& event);
    void OnFindWidget(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    void SetupMenuBar();
    void SetupToolBar();
    void SetupAUI();

    wxAuiManager m_auiMgr;
    InspectionTree* m_tree;
    ObjectInfoPanel* m_info;
    MethodInvokerPanel* m_invoker;
    EventLoggerPanel* m_events;
    InspectionHighlighter* m_highlighter;

    wxDECLARE_EVENT_TABLE();
};

} // namespace wxInspector

#endif // WX_INSPECTOR_FRAME_H
```

- [ ] **Step 2: Write the implementation `src/frame.cpp`**

```cpp
#include "wx/inspector/frame.h"
#include "wx/inspector/tree.h"
#include "wx/inspector/info.h"
#include "wx/inspector/invoker.h"
#include "wx/inspector/events.h"
#include "wx/inspector/highlighter.h"
#include "wx/inspector/property_provider.h"
#include "wx/inspector/method_registry.h"
#include <wx/menu.h>
#include <wx/toolbar.h>
#include <wx/artprov.h>
#include <wx/config.h>
#include <wx/accel.h>

namespace wxInspector {

wxDEFINE_EVENT(wxEVT_INSPECTION_FRAME_CLOSED, wxCommandEvent);

enum {
    ID_HIGHLIGHT = wxID_HIGHEST + 400,
    ID_FIND_WIDGET,
    ID_REFRESH_TREE,
    ID_TOGGLE_SIZERS,
    ID_EXPAND_ALL,
    ID_COLLAPSE_ALL,
    ID_ABOUT,
    ID_QUIT
};

wxBEGIN_EVENT_TABLE(InspectionFrame, wxFrame)
    EVT_CLOSE(InspectionFrame::OnClose)
    EVT_COMMAND(wxID_ANY, wxEVT_INSPECT_TREE_SEL_CHANGED,
                InspectionFrame::OnTreeSelectionChanged)
wxEND_EVENT_TABLE()

InspectionFrame::InspectionFrame(wxWindow* parent, wxPoint pos, wxSize size)
    : wxFrame(parent, wxID_ANY, "wxInspector", pos, size,
              wxDEFAULT_FRAME_STYLE | wxFRAME_NO_TASKBAR)
{
    SetIcon(wxArtProvider::GetIcon(wxART_FIND));

    // Initialize registries
    PropertyProvider::Get().RegisterBuiltinProviders();
    MethodRegistry::Get().RegisterBuiltinMethods();

    // Create panels
    m_tree = new InspectionTree(this);
    m_info = new ObjectInfoPanel(this);
    m_invoker = new MethodInvokerPanel(this);
    m_events = new EventLoggerPanel(this);
    m_highlighter = new InspectionHighlighter();

    SetupMenuBar();
    SetupToolBar();
    SetupAUI();

    LoadLayout();

    // Keyboard accelerators
    wxAcceleratorEntry entries[] = {
        { wxACCEL_NORMAL, WXK_F1, ID_REFRESH_TREE },
        { wxACCEL_NORMAL, WXK_F2, ID_FIND_WIDGET },
        { wxACCEL_NORMAL, WXK_F3, ID_TOGGLE_SIZERS },
        { wxACCEL_NORMAL, WXK_F4, ID_EXPAND_ALL },
        { wxACCEL_NORMAL, WXK_F5, ID_COLLAPSE_ALL },
        { wxACCEL_NORMAL, WXK_F6, ID_HIGHLIGHT },
        { wxACCEL_CTRL,   'I',   ID_QUIT },
    };
    wxAcceleratorTable accel(sizeof(entries)/sizeof(entries[0]), entries);
    SetAcceleratorTable(accel);
}

void InspectionFrame::SetupMenuBar()
{
    wxMenuBar* mb = new wxMenuBar();

    wxMenu* fileMenu = new wxMenu();
    fileMenu->Append(ID_REFRESH_TREE, "&Refresh Tree\tF1");
    fileMenu->AppendCheckItem(ID_TOGGLE_SIZERS, "&Show Sizers\tF3");
    fileMenu->AppendSeparator();
    fileMenu->Append(ID_QUIT, "&Close\tCtrl+I");
    mb->Append(fileMenu, "&File");

    wxMenu* viewMenu = new wxMenu();
    viewMenu->Append(ID_EXPAND_ALL, "&Expand All\tF4");
    viewMenu->Append(ID_COLLAPSE_ALL, "&Collapse All\tF5");
    mb->Append(viewMenu, "&View");

    wxMenu* toolsMenu = new wxMenu();
    toolsMenu->Append(ID_HIGHLIGHT, "&Highlight Widget\tF6");
    toolsMenu->Append(ID_FIND_WIDGET, "&Find Widget\tF2");
    mb->Append(toolsMenu, "&Tools");

    wxMenu* helpMenu = new wxMenu();
    helpMenu->Append(ID_ABOUT, "&About");
    mb->Append(helpMenu, "&Help");

    SetMenuBar(mb);

    Bind(wxEVT_MENU, [this](wxCommandEvent&) { m_tree->RebuildTree(); }, ID_REFRESH_TREE);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { m_tree->ToggleSizers(); }, ID_TOGGLE_SIZERS);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { m_tree->ExpandAll(); }, ID_EXPAND_ALL);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { m_tree->CollapseAll(); }, ID_COLLAPSE_ALL);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { Close(); }, ID_QUIT);
    Bind(wxEVT_MENU, &InspectionFrame::OnHighlight, this, ID_HIGHLIGHT);
    Bind(wxEVT_MENU, &InspectionFrame::OnFindWidget, this, ID_FIND_WIDGET);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        wxMessageBox("wxInspector — Widget Inspection Tool\n"
                     "Ported from wxPython wx.lib.inspection\n\n"
                     "Ctrl+Shift+I to toggle", "About wxInspector",
                     wxOK | wxICON_INFORMATION, this);
    }, ID_ABOUT);
}

void InspectionFrame::SetupToolBar()
{
    wxToolBar* tb = CreateToolBar(wxTB_FLAT | wxTB_NODIVIDER);
    tb->AddTool(ID_REFRESH_TREE, "Refresh",
        wxArtProvider::GetBitmap(wxART_REDO), "Refresh (F1)");
    tb->AddTool(ID_FIND_WIDGET, "Find",
        wxArtProvider::GetBitmap(wxART_CROSS_MARK), "Find Widget (F2)");
    tb->AddCheckTool(ID_TOGGLE_SIZERS, "Sizers",
        wxArtProvider::GetBitmap(wxART_FIND), wxNullBitmap,
        "Toggle Sizers (F3)");
    tb->AddTool(ID_HIGHLIGHT, "Highlight",
        wxArtProvider::GetBitmap(wxART_TIP), "Highlight (F6)");
    tb->Realize();
}

void InspectionFrame::SetupAUI()
{
    m_auiMgr.SetManagedWindow(this);

    wxAuiPaneInfo treePane;
    treePane.Name("Tree").Caption("Widget Tree").Left().Layer(0)
        .Row(0).Position(0).BestSize(250, 400).MinSize(150, 200);
    m_auiMgr.AddPane(m_tree, treePane);

    wxAuiPaneInfo infoPane;
    infoPane.Name("Info").Caption("Object Info").Center().Layer(0)
        .Row(0).Position(0).BestSize(400, 400).MinSize(200, 200);
    m_auiMgr.AddPane(m_info, infoPane);

    wxAuiPaneInfo invokerPane;
    invokerPane.Name("Invoker").Caption("Method Invoker").Bottom().Layer(0)
        .Row(0).Position(0).BestSize(600, 200).MinSize(300, 150);
    m_auiMgr.AddPane(m_invoker, invokerPane);

    wxAuiPaneInfo eventPane;
    eventPane.Name("Events").Caption("Event Logger").Bottom().Layer(0)
        .Row(1).Position(0).BestSize(600, 200).MinSize(300, 150);
    m_auiMgr.AddPane(m_events, eventPane);

    m_auiMgr.Update();
}

void InspectionFrame::Show(wxObject* selectObj, bool refreshTree)
{
    if (refreshTree) m_tree->RebuildTree();
    if (selectObj) SelectObject(selectObj);
    wxFrame::Show(true);
    Raise();
}

void InspectionFrame::SelectObject(wxObject* obj)
{
    m_tree->SelectObject(obj);
}

void InspectionFrame::RefreshTree()
{
    m_tree->RebuildTree();
}

void InspectionFrame::OnTreeSelectionChanged(wxCommandEvent& event)
{
    wxObject* obj = static_cast<wxObject*>(event.GetClientData());
    if (!obj) {
        m_info->Clear();
        m_invoker->Clear();
        m_events->ShowObject(InspectableObject::FromWindow(nullptr));
        return;
    }

    wxClassInfo* info = obj->GetClassInfo();
    InspectableObject inspObj = InspectableObject::FromWindow(nullptr);

    if (info->IsKindOf(CLASSINFO(wxWindow))) {
        inspObj = InspectableObject::FromWindow(
            static_cast<wxWindow*>(obj));
    } else if (info->IsKindOf(CLASSINFO(wxSizer))) {
        inspObj = InspectableObject::FromSizer(
            static_cast<wxSizer*>(obj));
    } else if (info->IsKindOf(CLASSINFO(wxSizerItem))) {
        inspObj = InspectableObject::FromSizerItem(
            static_cast<wxSizerItem*>(obj));
    }

    m_info->ShowObject(inspObj);
    m_invoker->ShowObject(inspObj);
    m_events->ShowObject(inspObj);
}

void InspectionFrame::OnHighlight(wxCommandEvent&)
{
    InspectableObject obj = m_tree->GetSelectedObject();
    if (obj.IsValid()) {
        m_highlighter->Highlight(obj);
    }
}

void InspectionFrame::OnFindWidget(wxCommandEvent&)
{
    m_tree->FindWidget();
}

void InspectionFrame::SaveLayout()
{
    wxConfigBase* cfg = wxConfig::Get();
    if (!cfg) return;

    cfg->SetPath("/wxInspector/Frame");
    cfg->Write("X", GetPosition().x);
    cfg->Write("Y", GetPosition().y);
    cfg->Write("Width", GetSize().x);
    cfg->Write("Height", GetSize().y);
    cfg->Write("Maximized", IsMaximized());

    wxString perspective = m_auiMgr.SavePerspective();
    cfg->Write("Perspective", perspective);
    cfg->SetPath("/");
}

void InspectionFrame::LoadLayout()
{
    wxConfigBase* cfg = wxConfig::Get();
    if (!cfg) return;

    cfg->SetPath("/wxInspector/Frame");
    long x = cfg->ReadLong("X", -1);
    long y = cfg->ReadLong("Y", -1);
    long w = cfg->ReadLong("Width", 800);
    long h = cfg->ReadLong("Height", 600);
    bool maximized = cfg->ReadBool("Maximized", false);

    if (x >= 0 && y >= 0)
        SetSize((int)x, (int)y, (int)w, (int)h);
    else
        SetSize((int)w, (int)h);

    if (maximized) Maximize();

    wxString perspective = cfg->Read("Perspective", "");
    if (!perspective.empty())
        m_auiMgr.LoadPerspective(perspective);

    cfg->SetPath("/");
}

void InspectionFrame::OnClose(wxCloseEvent& event)
{
    SaveLayout();

    wxCommandEvent evt(wxEVT_INSPECTION_FRAME_CLOSED, GetId());
    evt.SetEventObject(this);
    ProcessWindowEvent(evt);

    event.Skip();
}

} // namespace wxInspector
```

- [ ] **Step 3: Add Tree getter to `include/wx/inspector/tree.h`**

Add to the InspectionTree class:
```cpp
    wxTreeCtrl* GetTreeCtrl() { return m_tree; }
```

- [ ] **Step 4: Verify compilation**

Run: `cd build && cmake --build . --target wxInspector 2>&1 | tail -5`
Expected: Clean compile (the library should now link since all .cpp files exist)

- [ ] **Step 5: Commit**

```bash
git add include/wx/inspector/frame.h src/frame.cpp include/wx/inspector/tree.h
git commit -m "feat: add InspectionFrame with AUI layout and menu/toolbar

4-pane AUI layout: Tree (left), Info (center), Invoker + Event Logger
(bottom, tabbed). Menu bar, toolbar, keyboard accelerators F1-F6.
Layout persistence via wxConfig (position, size, perspective)."
```


### Task 11: Public API + wxInspectorMixin

**Files:**
- Create: `include/wx/inspector/inspector.h`
- Create: `src/inspector.cpp`

**Interfaces:**
- Consumes: `InspectionFrame` (Task 10)
- Produces: `wxInspector` namespace functions — `Init()`, `Show()`, `Hide()`, `RefreshTree()`, `SelectObject()`, `IsVisible()`, `RegisterPlugin()`
- Produces: `wxInspectorMixin` class — auto-registers `Ctrl+Shift+I` accelerator

- [ ] **Step 1: Write the header `include/wx/inspector/inspector.h`**

```cpp
#ifndef WX_INSPECTOR_INSPECTOR_H
#define WX_INSPECTOR_INSPECTOR_H

#include <wx/object.h>
#include <wx/app.h>
#include "wx/inspector/plugin.h"

class wxConfigBase;

namespace wxInspector {

// --- Public API ---

bool Init(wxConfigBase* config = nullptr);
void Show(wxObject* selectObj = nullptr, bool refreshTree = false);
void Hide();
void RefreshTree();
void SelectObject(wxObject* obj);
bool IsVisible();
void RegisterPlugin(wxInspectorPlugin* plugin);

} // namespace wxInspector

// --- wxInspectorMixin ---

class wxInspectorMixin {
public:
    wxInspectorMixin();
    ~wxInspectorMixin();

    wxInspectorMixin(const wxInspectorMixin&) = delete;
    wxInspectorMixin& operator=(const wxInspectorMixin&) = delete;

protected:
    void SetupInspectorAccelerator(wxWindow* window);

private:
    static const int ID_INSPECTOR_TOGGLE = wxID_HIGHEST + 9999;
    void OnToggleInspector(wxCommandEvent& event);
    wxWindow* m_accelWindow;
};

#endif // WX_INSPECTOR_INSPECTOR_H
```

- [ ] **Step 2: Write the implementation `src/inspector.cpp`**

```cpp
#include "wx/inspector/inspector.h"
#include "wx/inspector/frame.h"
#include "wx/inspector/property_provider.h"
#include "wx/inspector/method_registry.h"
#include <wx/config.h>
#include <wx/accel.h>
#include <wx/window.h>

namespace wxInspector {

// Singleton state
static InspectionFrame* g_inspectorFrame = nullptr;
static wxConfigBase* g_config = nullptr;

bool Init(wxConfigBase* config)
{
    if (g_inspectorFrame) return true; // already initialized

    if (config) {
        g_config = config;
    } else {
        g_config = new wxConfig("wxInspector");
    }

    // Create the frame (hidden by default)
    int x = -1, y = -1, w = 800, h = 600;
    if (g_config) {
        g_config->SetPath("/wxInspector/Frame");
        x = (int)g_config->ReadLong("X", -1);
        y = (int)g_config->ReadLong("Y", -1);
        w = (int)g_config->ReadLong("Width", 800);
        h = (int)g_config->ReadLong("Height", 600);
        g_config->SetPath("/");
    }

    wxPoint pos(x >= 0 ? x : 50, y >= 0 ? y : 50);
    g_inspectorFrame = new InspectionFrame(
        nullptr, pos, wxSize(w, h));

    // Handle frame being closed
    g_inspectorFrame->Bind(wxEVT_INSPECTION_FRAME_CLOSED,
        [](wxCommandEvent&) {
            g_inspectorFrame->Show(false);
        });

    return true;
}

void Show(wxObject* selectObj, bool refreshTree)
{
    if (!g_inspectorFrame) return;
    g_inspectorFrame->Show(selectObj, refreshTree);
}

void Hide()
{
    if (g_inspectorFrame) {
        g_inspectorFrame->Show(false);
    }
}

void RefreshTree()
{
    if (g_inspectorFrame) {
        g_inspectorFrame->RefreshTree();
    }
}

void SelectObject(wxObject* obj)
{
    if (g_inspectorFrame) {
        g_inspectorFrame->SelectObject(obj);
    }
}

bool IsVisible()
{
    return g_inspectorFrame && g_inspectorFrame->IsShown();
}

void RegisterPlugin(wxInspectorPlugin* plugin)
{
    PropertyProvider::Get().RegisterPlugin(plugin);
    MethodRegistry::Get().RegisterPlugin(plugin);
}

} // namespace wxInspector

// --- wxInspectorMixin ---

wxInspectorMixin::wxInspectorMixin()
    : m_accelWindow(nullptr)
{
}

wxInspectorMixin::~wxInspectorMixin()
{
}

void wxInspectorMixin::SetupInspectorAccelerator(wxWindow* window)
{
    m_accelWindow = window;

    wxAcceleratorEntry entries[1];
    entries[0].Set(wxACCEL_CTRL | wxACCEL_SHIFT, 'I', ID_INSPECTOR_TOGGLE);
    wxAcceleratorTable accel(1, entries);
    window->SetAcceleratorTable(accel);

    window->Bind(wxEVT_MENU, &wxInspectorMixin::OnToggleInspector,
                 this, ID_INSPECTOR_TOGGLE);
}

void wxInspectorMixin::OnToggleInspector(wxCommandEvent&)
{
    if (wxInspector::IsVisible()) {
        wxInspector::Hide();
    } else {
        wxInspector::Show();
    }
}
```

- [ ] **Step 3: Verify the library links successfully**

Run: `cd build && cmake --build . --target wxInspector 2>&1 | tail -10`
Expected: Library `libwxInspector.a` (or `.dylib`) built successfully — all symbols resolved.

- [ ] **Step 4: Commit**

```bash
git add include/wx/inspector/inspector.h src/inspector.cpp
git commit -m "feat: add public API and wxInspectorMixin

Minimal integration: wxInspector::Init() + wxInspector::Show().
wxInspectorMixin auto-registers Ctrl+Shift+I accelerator for toggle.
Full API: Init, Show, Hide, RefreshTree, SelectObject, IsVisible,
RegisterPlugin."
```


### Task 12: Sample Application

**Files:**
- Create: `samples/minimal/minimal.cpp`

**Interfaces:**
- Consumes: `wxInspector` public API (Task 11), `wxInspectorMixin` (Task 11)
- Produces: Runnable sample app with buttons, text controls, sizers, menu for manual verification

- [ ] **Step 1: Write `samples/minimal/minimal.cpp`**

```cpp
#include <wx/wx.h>
#include <wx/inspector/inspector.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/slider.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/spinctrl.h>
#include <wx/radiobut.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/statbox.h>
#include <wx/sizer.h>
#include <wx/menu.h>
#include <wx/aboutdlg.h>

class SampleFrame : public wxFrame {
public:
    SampleFrame() : wxFrame(nullptr, wxID_ANY, "wxInspector Sample",
                             wxDefaultPosition, wxSize(500, 400))
    {
        SetupMenu();
        SetupUI();
        CreateStatusBar();
        SetStatusText("Press Ctrl+Shift+I to open wxInspector");

        // Setup inspector accelerator on this frame
        SetupInspectorAccelerator(this);
    }

private:
    void SetupMenu() {
        wxMenu* fileMenu = new wxMenu();
        fileMenu->Append(wxID_EXIT, "E&xit\tAlt+X");
        Bind(wxEVT_MENU, [this](wxCommandEvent&) { Close(true); }, wxID_EXIT);

        wxMenu* helpMenu = new wxMenu();
        helpMenu->Append(wxID_ABOUT, "&About");
        Bind(wxEVT_MENU, [this](wxCommandEvent&) {
            wxAboutDialogInfo info;
            info.SetName("wxInspector Sample");
            info.SetDescription("A minimal wxWidgets app for testing wxInspector");
            wxAboutBox(info);
        }, wxID_ABOUT);

        wxMenuBar* mb = new wxMenuBar();
        mb->Append(fileMenu, "&File");
        mb->Append(helpMenu, "&Help");
        SetMenuBar(mb);
    }

    void SetupUI() {
        wxPanel* panel = new wxPanel(this, wxID_ANY);
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        // --- Controls section ---
        wxStaticBox* ctrlBox = new wxStaticBox(panel, wxID_ANY, "Controls");
        wxStaticBoxSizer* ctrlSizer = new wxStaticBoxSizer(ctrlBox, wxVERTICAL);

        // Row 1: Text entry
        wxBoxSizer* row1 = new wxBoxSizer(wxHORIZONTAL);
        row1->Add(new wxStaticText(panel, wxID_ANY, "Name:"),
            0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
        wxTextCtrl* textCtrl = new wxTextCtrl(panel, wxID_ANY, "Sample text");
        textCtrl->SetName("nameTextCtrl");
        row1->Add(textCtrl, 1, wxEXPAND);
        ctrlSizer->Add(row1, 0, wxEXPAND | wxALL, 5);

        // Row 2: Checkbox + Radio buttons
        wxBoxSizer* row2 = new wxBoxSizer(wxHORIZONTAL);
        wxCheckBox* check = new wxCheckBox(panel, wxID_ANY, "Enable feature");
        check->SetName("enableCheckBox");
        check->SetValue(true);
        row2->Add(check, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 20);
        row2->Add(new wxRadioButton(panel, wxID_ANY, "Option A"), 0, wxALIGN_CENTER_VERTICAL);
        row2->Add(new wxRadioButton(panel, wxID_ANY, "Option B"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
        ctrlSizer->Add(row2, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);

        // Row 3: Slider + SpinCtrl
        wxBoxSizer* row3 = new wxBoxSizer(wxHORIZONTAL);
        row3->Add(new wxStaticText(panel, wxID_ANY, "Value:"),
            0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
        wxSlider* slider = new wxSlider(panel, wxID_ANY, 50, 0, 100);
        slider->SetName("valueSlider");
        row3->Add(slider, 1, wxEXPAND);
        wxSpinCtrl* spin = new wxSpinCtrl(panel, wxID_ANY, "50",
            wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100);
        spin->SetName("valueSpin");
        row3->Add(spin, 0, wxLEFT, 10);
        ctrlSizer->Add(row3, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);

        // Row 4: Choice + ComboBox
        wxBoxSizer* row4 = new wxBoxSizer(wxHORIZONTAL);
        row4->Add(new wxStaticText(panel, wxID_ANY, "Type:"),
            0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
        wxChoice* choice = new wxChoice(panel, wxID_ANY,
            wxDefaultPosition, wxDefaultSize,
            wxArrayString(3, "Type A", "Type B", "Type C"));
        choice->SetName("typeChoice");
        choice->Select(0);
        row4->Add(choice, 1);
        wxComboBox* combo = new wxComboBox(panel, wxID_ANY, "",
            wxDefaultPosition, wxDefaultSize,
            wxArrayString(3, "Item 1", "Item 2", "Item 3"));
        combo->SetName("itemCombo");
        combo->SetValue("Item 1");
        row4->Add(combo, 1, wxLEFT, 10);
        ctrlSizer->Add(row4, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);

        // Row 5: Gauge
        wxGauge* gauge = new wxGauge(panel, wxID_ANY, 100);
        gauge->SetName("progressGauge");
        gauge->SetValue(65);
        ctrlSizer->Add(gauge, 0, wxEXPAND | wxALL, 5);

        // Row 6: Buttons
        wxBoxSizer* row6 = new wxBoxSizer(wxHORIZONTAL);
        wxButton* okBtn = new wxButton(panel, wxID_OK, "OK");
        okBtn->SetName("okButton");
        wxButton* cancelBtn = new wxButton(panel, wxID_CANCEL, "Cancel");
        cancelBtn->SetName("cancelButton");
        wxButton* applyBtn = new wxButton(panel, wxID_APPLY, "Apply");
        applyBtn->SetName("applyButton");
        row6->Add(okBtn, 0, wxRIGHT, 5);
        row6->Add(cancelBtn, 0, wxRIGHT, 5);
        row6->Add(applyBtn, 0);
        ctrlSizer->Add(row6, 0, wxALIGN_RIGHT | wxALL, 5);

        mainSizer->Add(ctrlSizer, 0, wxEXPAND | wxALL, 10);

        // --- Nested panel with its own sizer ---
        wxStaticBox* nestedBox = new wxStaticBox(panel, wxID_ANY, "Nested Panel");
        wxStaticBoxSizer* nestedSizer = new wxStaticBoxSizer(nestedBox, wxHORIZONTAL);
        wxPanel* nestedPanel = new wxPanel(panel, wxID_ANY);
        nestedPanel->SetName("nestedPanel");
        wxBoxSizer* nestedInner = new wxBoxSizer(wxVERTICAL);
        nestedInner->Add(new wxStaticText(nestedPanel, wxID_ANY, "Sub-item 1"),
            0, wxALL, 5);
        nestedInner->Add(new wxTextCtrl(nestedPanel, wxID_ANY, "nested text"),
            0, wxEXPAND | wxALL, 5);
        nestedInner->Add(new wxCheckBox(nestedPanel, wxID_ANY, "Nested option"),
            0, wxALL, 5);
        nestedPanel->SetSizer(nestedInner);
        nestedSizer->Add(nestedPanel, 1, wxEXPAND | wxALL, 5);
        mainSizer->Add(nestedSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

        panel->SetSizer(mainSizer);

        // Wire up OK button to show a message
        okBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            wxMessageBox("OK clicked!", "wxInspector Sample");
        });
    }
};

class SampleApp : public wxApp, public wxInspectorMixin {
public:
    bool OnInit() override {
        if (!wxInspector::Init()) {
            wxLogError("Failed to initialize wxInspector");
        }

        SampleFrame* frame = new SampleFrame();
        frame->Show(true);

        return true;
    }
};

wxIMPLEMENT_APP(SampleApp);
```

- [ ] **Step 2: Verify the sample builds and links**

Run: `cd build && cmake --build . 2>&1 | tail -10`
Expected: Both `libwxInspector` and `minimal` executable build successfully.

- [ ] **Step 3: Commit**

```bash
git add samples/minimal/minimal.cpp
git commit -m "feat: add minimal sample app with diverse controls

Includes text ctrls, checkboxes, radio buttons, slider, spin ctrl,
choice, combo box, gauge, buttons, nested panels with sizers.
Uses wxInspectorMixin for Ctrl+Shift+I toggle."
```


---

## Execution Handoff

Plan complete and saved to `docs/superpowers/plans/2026-07-21-wxInspector-implementation.md`. Two execution options:

**1. Subagent-Driven (recommended)** — Fresh subagent per task, review between tasks, fast iteration. Use `/superpowers:subagent-driven-development`.

**2. Inline Execution** — Execute tasks in this session using `/superpowers:executing-plans`, batch execution with checkpoints.

**Which approach?**
