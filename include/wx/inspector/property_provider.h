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
