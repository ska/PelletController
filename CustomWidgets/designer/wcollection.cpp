#include "wcollection.h"
#include "wClockWidgetPlugin.h"
#include "wClickableLabelPlugin.h"
#include "wTimePickerPlugin.h"
#include "wClickableLCDNumberPlugin.h"

wCollection::wCollection(QObject *parent)
    : QObject(parent)
{
    m_widgets.append(new wClickableLabelPlugin(this));
    m_widgets.append(new wClockWidgetPlugin(this));
    m_widgets.append(new wTimePickerPlugin(this));
    m_widgets.append(new wClickableLCDNumberPlugin(this));
}

QList<QDesignerCustomWidgetInterface *> wCollection::customWidgets() const
{
    return m_widgets;
}
