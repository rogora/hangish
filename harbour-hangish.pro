# NOTICE:
#
# Application name defined in TARGET has a corresponding QML filename.
# If name defined in TARGET is changed, the following needs to be done
# to match new name:
#   - corresponding QML filename must be changed
#   - desktop icon filename must be changed
#   - desktop filename must be changed
#   - icon definition filename in desktop file must be changed
#   - translation filenames have to be changed

# The name of your application
QT += dbus contacts
include(notifications.pri)
include(translations/translations.pri)
#include(keepalive.pri)

TARGET = harbour-hangish

CONFIG += sailfishapp c++11

QMAKE_CFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CFLAGS_RELEASE += -O3
QMAKE_CXXFLAGS_RELEASE += -O3

QMAKE_CFLAGS -= -O2
QMAKE_CFLAGS -= -O1
QMAKE_CXXFLAGS -= -O2
QMAKE_CXXFLAGS -= -O1
QMAKE_CFLAGS += -O3
QMAKE_CXXFLAGS += -O3

system(qdbusxml2cpp harbour.hangish.xml -i notifier.h -a adaptor)

SOURCES += src/Hangish.cpp \
    #authenticator.cpp \
    client.cpp \
    channel.cpp \
    conversationmodel.cpp \
    rostermodel.cpp \
    utils.cpp \
    contactsmodel.cpp \
    notifier.cpp \
    filemodel.cpp \
    messagefield.cpp \
    adaptor.cpp \
    oauth2auth.cpp \
    imagehandler.cpp

OTHER_FILES += \
    qml/cover/CoverPage.qml \
    qml/pages/FirstPage.qml \
    translations/*.ts \
    qml/pages/Conversation.qml \
    qml/pages/Roster.qml \
    qml/pages/FullscreenImage.qml \
    qml/pages/ImagePicker.qml \
    qml/pages/InfoBanner.qml \
    qml/delegates/Message.qml \
    qml/delegates/RosterDelegate.qml \
    harbour.hangish.xml \
    harbour-hangish.desktop \
    rpm/harbour-hangish.spec \
    rpm/harbour-hangish.yaml \
    qml/harbour-hangish.qml \
    qml/pages/About.qml \
    rpm/harbour-hangish.changes \
    qml/pages/ConvRenameInput.qml \
    qml/pages/Contacts.qml \
    qml/delegates/ContactEntry.qml

# to disable building translations every time, comment out the
# following CONFIG line
CONFIG += sailfishapp_i18n
#TRANSLATIONS += translations/harbour-hangish-it.ts

HEADERS += \
    #authenticator.h \
    client.h \
    channel.h \
    qtimports.h \
    conversationmodel.h \
    rostermodel.h \
    utils.h \
    structs.h \
    contactsmodel.h \
    notifier.h \
    filemodel.h \
    messagefield.h \
    adaptor.h \
    oauth2auth.h \
    imagehandler.h

RESOURCES += \
    harbour-hangish.qrc
