QT += core gui network multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

SOURCES += \
    adminapi.cpp \
    interfaz.cpp \
    main.cpp \
    login.cpp \
    registrar.cpp \
    grabaraudio.cpp

HEADERS += \
    adminapi.h \
    errorhandler.h \
    interfaz.h \
    login.h \
    registrar.h \
    grabaraudio.h

FORMS += \
    interfaz.ui \
    login.ui \
    registrar.ui

DISTFILES += \
    style.qss \
