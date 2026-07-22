#include "wx/inspector/inspector.h"

#ifndef WXINSPECTOR_DISABLE

#include "wx/inspector/frame.h"
#include "wx/inspector/object.h"
#include "wx/inspector/property_provider.h"
#include "wx/inspector/method_registry.h"
#include <wx/accel.h>
#include <wx/window.h>

namespace wxInspector {

void detail::RegisterPluginImpl(wxInspectorPlugin* plugin)
{
    PropertyProvider::Get().RegisterPlugin(plugin);
    MethodRegistry::Get().RegisterPlugin(plugin);
}

wxInspectableImpl::wxInspectableImpl()
    : m_inspectorFrame(nullptr)
    , m_accelWindow(nullptr)
{
}

void wxInspectableImpl::SetupInspectorAccelerator(wxWindow* window)
{
    m_accelWindow = window;
    if (!window)
        return;

    wxAcceleratorEntry entries[] = {
        { wxACCEL_CTRL | wxACCEL_SHIFT, 'I', ID_INSPECTOR_TOGGLE },
    };
    wxAcceleratorTable accel(sizeof(entries) / sizeof(entries[0]), entries);
    window->SetAcceleratorTable(accel);
    window->Bind(wxEVT_MENU, &wxInspectableImpl::OnToggleInspector, this, ID_INSPECTOR_TOGGLE);
}

void wxInspectableImpl::OnToggleInspector(wxCommandEvent&)
{
    if (IsInspectorVisible())
        HideInspector();
    else
        ShowInspector();
}

void wxInspectableImpl::ShowInspector(wxObject* selectObj)
{
    if (!m_inspectorFrame)
    {
        wxWindow* parent = m_accelWindow;
        m_inspectorFrame = new wxInspector::InspectionFrame(
            parent, wxDefaultPosition, wxSize(800, 600));
    }
    m_inspectorFrame->Show(selectObj);
}

void wxInspectableImpl::HideInspector()
{
    if (m_inspectorFrame)
        m_inspectorFrame->Hide();
}

bool wxInspectableImpl::IsInspectorVisible() const
{
    return m_inspectorFrame && m_inspectorFrame->IsVisible();
}

void wxInspectableImpl::RefreshInspectorTree()
{
    if (m_inspectorFrame)
        m_inspectorFrame->RefreshTree();
}

void wxInspectableImpl::SelectInspectorObject(wxObject* obj)
{
    if (m_inspectorFrame)
        m_inspectorFrame->SelectObject(obj);
}

} // namespace wxInspector

#endif // WXINSPECTOR_DISABLE
