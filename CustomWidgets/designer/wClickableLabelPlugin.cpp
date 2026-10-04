#include "wClickableLabel.h"
#include "wClickableLabelPlugin.h"

#include <QtPlugin>

wClickableLabelPlugin::wClickableLabelPlugin(QObject *parent)
    : QObject(parent)
{
}

void wClickableLabelPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool wClickableLabelPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *wClickableLabelPlugin::createWidget(QWidget *parent)
{
    return new wClickableLabel(parent);
}

QString wClickableLabelPlugin::name() const
{
    return QLatin1String("wClickableLabel");
}

QString wClickableLabelPlugin::group() const
{
    return QLatin1String("wGroupTest");
}

QIcon wClickableLabelPlugin::icon() const
{
    return QIcon(QLatin1String(":/heat-red.png"));
}

QString wClickableLabelPlugin::toolTip() const
{
    return QLatin1String("wToolTip");
}

QString wClickableLabelPlugin::whatsThis() const
{
    return QLatin1String("XXXX");
}

bool wClickableLabelPlugin::isContainer() const
{
    return false;
}

QString wClickableLabelPlugin::domXml() const
{
    return QLatin1String(R"(<widget class="wClickableLabel" name="clickableLabel">
    <property name="geometry">
     <rect>
      <x>170</x>
      <y>150</y>
      <width>54</width>
      <height>17</height>
     </rect>
    </property>
</widget>)");
}

QString wClickableLabelPlugin::includeFile() const
{
    return QLatin1String("wClickableLabel.h");
}
