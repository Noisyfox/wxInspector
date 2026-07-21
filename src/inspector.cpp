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

    // Handle frame being closed (the frame already hides itself via
    // OnClose; this handler is a secondary listener for API users)
    g_inspectorFrame->Bind(wxEVT_INSPECTION_FRAME_CLOSED,
        [](wxCommandEvent&) {
            g_inspectorFrame->Hide();
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
        g_inspectorFrame->Hide();
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
