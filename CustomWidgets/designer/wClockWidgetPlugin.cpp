#include "wClockWidget.h"
#include "wClockWidgetPlugin.h"

#include <QtPlugin>

wClockWidgetPlugin::wClockWidgetPlugin(QObject *parent)
    : QObject(parent)
{
}

void wClockWidgetPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool wClockWidgetPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *wClockWidgetPlugin::createWidget(QWidget *parent)
{
    return new wClockWidget(parent);
}

QString wClockWidgetPlugin::name() const
{
    return QLatin1String("wClockWidget");
}

QString wClockWidgetPlugin::group() const
{
    return QLatin1String("wGroupTest");
}

QIcon wClockWidgetPlugin::icon() const
{
    return QIcon(QLatin1String(":/heat-black.png"));
}

QString wClockWidgetPlugin::toolTip() const
{
    return QLatin1String("wToolTip");
}

QString wClockWidgetPlugin::whatsThis() const
{
    return QLatin1String("XXXX");
}

bool wClockWidgetPlugin::isContainer() const
{
    return false;
}

QString wClockWidgetPlugin::domXml() const
{
    return QLatin1String(R"(<widget class="wClockWidget" name="clockwidget">
    <property name="geometry">
     <rect>
      <x>0</x>
      <y>0</y>
      <width>120</width>
      <height>120</height>
     </rect>
    </property>
</widget>)");
}

QString wClockWidgetPlugin::includeFile() const
{
    return QLatin1String("wClockWidget.h");
}
