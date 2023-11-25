QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    filereader.cpp \
    mainwindow.cpp \
    playground.cpp \
    syntaxhighlighter.cpp \
    texteditor.cpp \
    treesitter.cpp \
    worker.cpp

HEADERS += \
    kvec.h \
    filereader.h \
    mainwindow.h \
    playground.h \
    syntaxhighlighter.h \
    texteditor.h \
    treesitter.h \
    worker.h

FORMS += \
    mainwindow.ui

INCLUDEPATH += \
    $$PWD/tree-sitter/lib/include \
    $$PWD/gnu_regex

LIBS += \
    -L$$PWD/tree-sitter -ltree-sitter \
    -L$$PWD/tree-sitter-javascript -ltree-sitter-javascript \
    -L$$PWD/gnu_regex/dist -lregex
#    $$PWD/tree-sitter/tree-sitter.a \
#    $$PWD/tree-sitter-javascript/tree-sitter-javascript.a

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
