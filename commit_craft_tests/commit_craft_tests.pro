QT += testlib widgets

CONFIG += c++17

TARGET = commit_craft_tests
TEMPLATE = app

SOURCES += \
    main.cpp \    
    tst_commithistorymodel.cpp \
    tst_filemodel.cpp \
    tst_mainwindow.cpp

# Add the path to your main application's source files
INCLUDEPATH += ../commit_craft

# Link to the main application
LIBS += -L../commit_craft -lcommit_craft

# Default rules for deployment.
qnx: target.path = /tmp/${TARGET}/bin
else: unix:!android: target.path = /opt/${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    tst_commithistorymodel.h \
    tst_filemodel.h \
    tst_mainwindow.h
