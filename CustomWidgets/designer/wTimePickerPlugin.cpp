#include "wTimePicker.h"
#include "wTimePickerPlugin.h"

#include <QtPlugin>

wTimePickerPlugin::wTimePickerPlugin(QObject *parent)
    : QObject(parent)
{
}

void wTimePickerPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool wTimePickerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *wTimePickerPlugin::createWidget(QWidget *parent)
{
    return new wTimePicker(parent);
}

QString wTimePickerPlugin::name() const
{
    return QLatin1String("wTimePicker");
}

QString wTimePickerPlugin::group() const
{
    return QLatin1String("wGroupTest");
}

QIcon wTimePickerPlugin::icon() const
{
    return QIcon(QLatin1String(":/heat-black.png"));
}

QString wTimePickerPlugin::toolTip() const
{
    return QLatin1String("wToolTip");
}

QString wTimePickerPlugin::whatsThis() const
{
    return QLatin1String("XXXX");
}

bool wTimePickerPlugin::isContainer() const
{
    return false;
}

QString wTimePickerPlugin::domXml() const
{
    return QLatin1String(R"(<widget class="wTimePicker" name="timepicker">
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

QString wTimePickerPlugin::includeFile() const
{
    return QLatin1String("wTimePicker.h");
}
