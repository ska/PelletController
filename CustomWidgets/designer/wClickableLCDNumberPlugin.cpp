#include "wClickableLCDNumber.h"
#include "wClickableLCDNumberPlugin.h"

#include <QtPlugin>

wClickableLCDNumberPlugin::wClickableLCDNumberPlugin(QObject *parent)
    : QObject(parent)
{
}

void wClickableLCDNumberPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool wClickableLCDNumberPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *wClickableLCDNumberPlugin::createWidget(QWidget *parent)
{
    return new wClickableLCDNumber(parent);
}

QString wClickableLCDNumberPlugin::name() const
{
    return QLatin1String("wClickableLCDNumber");
}

QString wClickableLCDNumberPlugin::group() const
{
    return QLatin1String("wGroupTest");
}

QIcon wClickableLCDNumberPlugin::icon() const
{
    return QIcon(QLatin1String(":/heat-red.png"));
}

QString wClickableLCDNumberPlugin::toolTip() const
{
    return QLatin1String("wToolTip");
}

QString wClickableLCDNumberPlugin::whatsThis() const
{
    return QLatin1String("XXXX");
}

bool wClickableLCDNumberPlugin::isContainer() const
{
    return false;
}

QString wClickableLCDNumberPlugin::domXml() const
{
    return QLatin1String(R"(<widget class="wClickableLCDNumber" name="clickablelcdnumber">
    </widget>)");
}

QString wClickableLCDNumberPlugin::includeFile() const
{
    return QLatin1String("wClickableLCDNumber.h");
}
