QT += gui \
    widgets

TEMPLATE = lib
CONFIG += staticlib

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

TARGET = commit_craft_lib

# Default rules for deployment.
unix {
    target.path = $[QT_INSTALL_PLUGINS]/generic
}
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    commithistorymodel.h \
    commititemdelegate.h \
    filemodel.h \
    git.h \
    gitparser.h

SOURCES += \
    commithistorymodel.cpp \
    commititemdelegate.cpp \
    filemodel.cpp \
    git.cpp \
    gitparser.cpp
