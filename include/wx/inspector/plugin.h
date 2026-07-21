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
