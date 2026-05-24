QT += widgets
QT += gui
CONFIG += c++17

VERSION = 0.1.0
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

TARGET = commit_craft
TEMPLATE = app

SOURCES += \
    brancheswidget.cpp \
    codeeditor.cpp \
    diffeditor.cpp \
    diffhighlighter.cpp \
    diffpanel.cpp \
    hunkactionpanel.cpp \
    intralinediff.cpp \
    main.cpp \
    mainwindow.cpp \
    settingsdialog.cpp \
    syntaxhighlighter.cpp

HEADERS += \
    brancheswidget.h \
    codeeditor.h \
    diffeditor.h \
    diffhighlighter.h \
    diffpanel.h \
    hunkactionpanel.h \
    intralinediff.h \
    linenumberarea.h \
    mainwindow.h \
    settingsdialog.h \
    syntaxhighlighter.h

FORMS += \
    mainwindow.ui \
    settingsdialog.ui \
    diffeditor.ui

RESOURCES += \
    icons.qrc

win32 {
    RC_ICONS = icons/app.ico
}



# Default rules for deployment.
qnx: target.path = /tmp/${TARGET}/bin
else: unix:!android: target.path = /opt/${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../commit_craft_lib/release/ -lcommit_craft_lib
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../commit_craft_lib/debug/ -lcommit_craft_lib
else:unix: LIBS += -L$$OUT_PWD/../commit_craft_lib/ -lcommit_craft_lib

INCLUDEPATH += $$PWD/../commit_craft_lib
DEPENDPATH += $$PWD/../commit_craft_lib

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../commit_craft_lib/release/libcommit_craft_lib.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../commit_craft_lib/debug/libcommit_craft_lib.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../commit_craft_lib/release/commit_craft_lib.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../commit_craft_lib/debug/commit_craft_lib.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../commit_craft_lib/libcommit_craft_lib.a
