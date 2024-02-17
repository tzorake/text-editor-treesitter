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
    arena.cpp \
    filereader.cpp \
    main.cpp \
    mainwindow.cpp \
    syntaxhighlighter.cpp \
    texteditor.cpp \
    treesitter.cpp \
    worker.cpp

HEADERS += \
    arena.h \
    filereader.h \
    mainwindow.h \
    syntaxhighlighter.h \
    texteditor.h \
    treesitter.h \
    worker.h

FORMS += \
    mainwindow.ui

INCLUDEPATH += \
    $${PWD}/tree-sitter/lib/include

LIBS += \
    -L$${PWD}/tree-sitter -ltree-sitter \
    -L$${PWD}/tree-sitter-javascript -ltree-sitter-javascript

CONFIG(debug, debug|release) {
    DESTDIR = $${OUT_PWD}/debug
} else {
    DESTDIR = $${OUT_PWD}/release
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Copy resources
# https://stackoverflow.com/questions/19066593/copy-a-file-to-build-directory-after-compiling-project-with-qt
dummy.commands = @echo After build copying is finieshed!
QMAKE_EXTRA_TARGETS += dummy
PRE_TARGETDEPS += dummy

IN_DIR = $$shell_quote($$shell_path($$PWD/resources))
OUT_DIR = $$shell_quote($$shell_path($$DESTDIR)/resources)

tools.commands = $(COPY_DIR) $$IN_DIR $$OUT_DIR
dummy.depends += tools
QMAKE_EXTRA_TARGETS += tools
