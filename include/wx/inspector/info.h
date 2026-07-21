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
