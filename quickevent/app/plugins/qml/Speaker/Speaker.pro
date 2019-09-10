message(including plugin $$PWD)

PLUGIN_NAME = Speaker

include ( ../quickeventqmlplugin.pri )

QT += widgets sql

CONFIG += c++11 hide_symbols

DEFINES += SPEAKERPLUGIN_BUILD_DLL

INCLUDEPATH += \
    $$PWD/../../../../lib/include \
    $$PWD/../Event/include \

LIBS += \
    -L$$DESTDIR \
    -lEventplugin \

include (src/src.pri)

#RESOURCES += \
#    $${PLUGIN_NAME}.qrc \

TRANSLATIONS += \
	$${PLUGIN_NAME}.cs_CZ.ts \
	$${PLUGIN_NAME}.pl_PL.ts \
	$${PLUGIN_NAME}.nb_NO.ts \
	$${PLUGIN_NAME}.ru_RU.ts \
	$${PLUGIN_NAME}.nl_BE.ts \

lupdate_only {
SOURCES += \
	$$PWD/qml/*.qml \
}
