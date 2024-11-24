QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++20

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    gamecontroller.cpp \
    gamemodel.cpp \
    gameview.cpp \
    main.cpp \
    mainwindow.cpp \
    textgameview.cpp

HEADERS += \
    gamecontroller.h \
    gamemodel.h \
    gameview.h \
    mainwindow.h \
    textgameview.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../worldlib_source/release/ -lworld
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../worldlib_source/debug/ -lworld

INCLUDEPATH += $$PWD/../worldlib_source/debug
DEPENDPATH += $$PWD/../worldlib_source/debug

RESOURCES += \
    images.qrc

DISTFILES += \
    images/enemy.png \
    images/enemy_defeated.png \
    images/healthpack.png \
    images/level1.png \
    images/level2.png \
    images/level3.png \
    images/maze1.png \
    images/maze2.png \
    images/maze3.png \
    images/penemy.png \
    images/portal.png \
    images/protagonist.png \
    images/world.png \
    images/worldmap4.png
