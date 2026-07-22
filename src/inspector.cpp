#include "wx/inspector/inspector.h"
#include "wx/inspector/frame.h"
#include "wx/inspector/object.h"
#include "wx/inspector/property_provider.h"
#include "wx/inspector/method_registry.h"
#include <wx/accel.h>
#include <wx/config.h>
#include <wx/window.h>

namespace wxInspector {

void RegisterPlugin(wxInspectorPlugin* plugin)
{
    PropertyProvider::Get().RegisterPlugin(plugin);
    MethodRegistry::Get().RegisterPlugin(plugin);
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
