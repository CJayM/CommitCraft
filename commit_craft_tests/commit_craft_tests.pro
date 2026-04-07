QT += testlib widgets gui

CONFIG += c++17
CONFIG += testcase


TARGET = commit_craft_tests
TEMPLATE = app

OBJECTS_DIR = _temp_build/objs
MOC_DIR = _temp_build/moc

SOURCES += \
    main.cpp \
    tst_commithistorymodel.cpp \
    tst_filemodel.cpp \
    tst_git.cpp \
    tst_gitparser.cpp \
    tst_intralinediff.cpp

HEADERS += \
    tst_commithistorymodel.h \
    tst_filemodel.h \
    tst_git.h \
    tst_gitparser.h \
    tst_intralinediff.h

# Include diff editor sources from the app directory
SOURCES += \
    $$PWD/../commit_craft_app/intralinediff.cpp \
    $$PWD/../commit_craft_app/diffhighlighter.cpp \
    $$PWD/../commit_craft_app/diffpanel.cpp \
    $$PWD/../commit_craft_app/diffeditor.cpp \
    $$PWD/../commit_craft_app/codeeditor.cpp \
    $$PWD/../commit_craft_app/syntaxhighlighter.cpp

HEADERS += \
    $$PWD/../commit_craft_app/intralinediff.h \
    $$PWD/../commit_craft_app/diffhighlighter.h \
    $$PWD/../commit_craft_app/diffpanel.h \
    $$PWD/../commit_craft_app/diffeditor.h \
    $$PWD/../commit_craft_app/codeeditor.h \
    $$PWD/../commit_craft_app/linenumberarea.h \
    $$PWD/../commit_craft_app/syntaxhighlighter.h

INCLUDEPATH += $$PWD/../commit_craft_app



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
