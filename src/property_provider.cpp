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
        props.push_back({"Size", "Geometry", PropertyType::String,
            wxString::Format("(%d,%d)", win->GetSize().x, win->GetSize().y),
            false, {},
            [win]() { wxSize s = win->GetSize();
                return wxString::Format("(%d,%d)", s.x, s.y); },
            [win](const wxString& val) {
                long w = -1, h = -1;
                if (sscanf(val.c_str(), "(%ld,%ld)", &w, &h) == 2 ||
                    sscanf(val.c_str(), "%ld,%ld", &w, &h) == 2) {
                    win->SetSize(wxSize(w, h)); return true;
                }
                return false;
            }});
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
        props.push_back({"MaxSize", "Geometry", PropertyType::String,
            wxString::Format("(%d,%d)", win->GetMaxSize().x, win->GetMaxSize().y),
            false, {},
            [win]() { wxSize s = win->GetMaxSize();
                return wxString::Format("(%d,%d)", s.x, s.y); },
            [win](const wxString& val) {
                long w = -1, h = -1;
                if (sscanf(val.c_str(), "(%ld,%ld)", &w, &h) == 2 ||
                    sscanf(val.c_str(), "%ld,%ld", &w, &h) == 2) {
                    win->SetMaxSize(wxSize(w, h)); return true;
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
                win->SetForegroundColour(wxColour(v));
                win->Refresh();
                return true;
            }});
        props.push_back({"Bg Colour", "Appearance", PropertyType::Colour,
            win->GetBackgroundColour().GetAsString(), false, {},
            [win]() { return win->GetBackgroundColour().GetAsString(); },
            [win](const wxString& v) {
                win->SetBackgroundColour(wxColour(v));
                win->Refresh();
                return true;
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
            [sizer]() { return wxString::Format("%p", sizer); }, nullptr});

        wxSize sz = sizer->GetSize();
        props.push_back({"Size", "Sizer", PropertyType::ReadOnly,
            wxString::Format("(%d, %d)", sz.x, sz.y), true, {},
            [sizer]() { wxSize s = sizer->GetSize();
                return wxString::Format("(%d, %d)", s.x, s.y); }, nullptr});
        wxSize minSz = sizer->GetMinSize();
        props.push_back({"MinSize", "Sizer", PropertyType::ReadOnly,
            wxString::Format("(%d, %d)", minSz.x, minSz.y), true, {},
            [sizer]() { wxSize s = sizer->GetMinSize();
                return wxString::Format("(%d, %d)", s.x, s.y); }, nullptr});

        if (wxBoxSizer* bs = wxDynamicCast(sizer, wxBoxSizer)) {
            props.push_back({"Orientation", "Sizer", PropertyType::ReadOnly,
                bs->GetOrientation() == wxHORIZONTAL ? "Horizontal" : "Vertical",
                true, {}, [bs]() { return bs->GetOrientation() == wxHORIZONTAL
                    ? "Horizontal" : "Vertical"; }, nullptr});
        } else if (wxGridSizer* gs = wxDynamicCast(sizer, wxGridSizer)) {
            props.push_back({"Cols", "Sizer", PropertyType::ReadOnly,
                wxString::Format("%d", gs->GetCols()), true, {},
                [gs]() { return wxString::Format("%d", gs->GetCols()); }, nullptr});
            props.push_back({"Rows", "Sizer", PropertyType::ReadOnly,
                wxString::Format("%d", gs->GetEffectiveRowsCount()), true, {},
                [gs]() { return wxString::Format("%d", gs->GetEffectiveRowsCount()); }, nullptr});
            props.push_back({"VGap", "Sizer", PropertyType::ReadOnly,
                wxString::Format("%d", gs->GetVGap()), true, {},
                [gs]() { return wxString::Format("%d", gs->GetVGap()); }, nullptr});
            props.push_back({"HGap", "Sizer", PropertyType::ReadOnly,
                wxString::Format("%d", gs->GetHGap()), true, {},
                [gs]() { return wxString::Format("%d", gs->GetHGap()); }, nullptr});
        }
        if (wxFlexGridSizer* fgs = wxDynamicCast(sizer, wxFlexGridSizer)) {
            props.push_back({"FlexDir", "Sizer", PropertyType::ReadOnly,
                fgs->GetFlexibleDirection() == wxHORIZONTAL ? "Horizontal" : "Vertical",
                true, {}, [fgs]() { return fgs->GetFlexibleDirection() == wxHORIZONTAL
                    ? "Horizontal" : "Vertical"; }, nullptr});
            props.push_back({"NonFlexGrowMode", "Sizer", PropertyType::ReadOnly,
                fgs->GetNonFlexibleGrowMode() != wxFLEX_GROWMODE_NONE ? "Grow" : "None",
                true, {}, [fgs]() { return fgs->GetNonFlexibleGrowMode() != wxFLEX_GROWMODE_NONE
                    ? "Grow" : "None"; }, nullptr});
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
            "wxSizerItem", true, {}, []() { return "wxSizerItem"; }, nullptr});

        if (item->IsSpacer()) {
            props.push_back({"Type", "Identity", PropertyType::ReadOnly,
                "Spacer", true, {}, []() { return "Spacer"; }, nullptr});
        } else if (item->IsWindow()) {
            props.push_back({"Type", "Identity", PropertyType::ReadOnly,
                "Window", true, {}, []() { return "Window"; }, nullptr});
        } else if (item->IsSizer()) {
            props.push_back({"Type", "Identity", PropertyType::ReadOnly,
                "Sizer", true, {}, []() { return "Sizer"; }, nullptr});
        }

        props.push_back({"Proportion", "Layout", PropertyType::Integer,
            wxString::Format("%d", item->GetProportion()), false, {},
            [item]() { return wxString::Format("%d", item->GetProportion()); },
            [item](const wxString& v) { long l; return v.ToLong(&l)
                ? (item->SetProportion((int)l), true) : false; }});
        props.push_back({"Flag", "Layout", PropertyType::Integer,
            wxString::Format("%d", item->GetFlag()), false, {},
            [item]() { return wxString::Format("%d", item->GetFlag()); },
            [item](const wxString& v) { long l; return v.ToLong(&l)
                ? (item->SetFlag((int)l), true) : false; }});
        props.push_back({"Border", "Layout", PropertyType::Integer,
            wxString::Format("%d", item->GetBorder()), false, {},
            [item]() { return wxString::Format("%d", item->GetBorder()); },
            [item](const wxString& v) { long l; return v.ToLong(&l)
                ? (item->SetBorder((int)l), true) : false; }});
        wxSize sz = item->GetSize();
        props.push_back({"Size", "Layout", PropertyType::ReadOnly,
            wxString::Format("(%d, %d)", sz.x, sz.y), true, {},
            [item]() { wxSize s = item->GetSize();
                return wxString::Format("(%d, %d)", s.x, s.y); }, nullptr});
        wxSize minSz = item->GetMinSize();
        props.push_back({"MinSize", "Layout", PropertyType::ReadOnly,
            wxString::Format("(%d, %d)", minSz.x, minSz.y), true, {},
            [item]() { wxSize s = item->GetMinSize();
                return wxString::Format("(%d, %d)", s.x, s.y); }, nullptr});

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
