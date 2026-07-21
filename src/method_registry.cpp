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
