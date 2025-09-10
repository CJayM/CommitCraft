QT += widgets
CONFIG += c++17

TARGET = commit_craft
TEMPLATE = app

SOURCES += \
    codeeditor.cpp \
    commithistorymodel.cpp \
    commititemdelegate.cpp \
    filemodel.cpp \
    main.cpp \
    mainwindow.cpp \
    settingsdialog.cpp \
    # synchronizezoom.cpp

HEADERS += \
    codeeditor.h \
    commithistorymodel.h \
    commititemdelegate.h \
    filemodel.h \
    linenumberarea.h \
    mainwindow.h \
    settingsdialog.h

FORMS += \
    mainwindow.ui \
    settingsdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/${TARGET}/bin
else: unix:!android: target.path = /opt/${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
