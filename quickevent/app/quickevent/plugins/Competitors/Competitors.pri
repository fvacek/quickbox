message(including plugin $$PWD)

PLUGIN_NAME = Competitors

include (src/src.pri)

RESOURCES += \
      $$PWD/$${PLUGIN_NAME}.qrc

lupdate_only {
SOURCES += \
        $$PWD/qml/*.qml \
}
