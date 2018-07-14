#-------------------------------------------------
#
# Project created by QtCreator 2018-07-14T13:05:24
#
#-------------------------------------------------

QT       += core gui network widgets mqtt

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MQTT_Subscriptions
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    subscriptionwindow.cpp

HEADERS += \
        mainwindow.h \
    subscriptionwindow.h

FORMS += \
        mainwindow.ui \
    subscriptionwindow.ui
