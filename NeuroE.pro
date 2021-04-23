#-------------------------------------------------
#
# Project created by QtCreator 2020-05-22T12:58:19
#
#-------------------------------------------------

QT       += core gui multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = NeuroE
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    usbcam.cpp \
    nrtexe.cpp \
    predict.cpp

HEADERS += \
        mainwindow.h \
    usbcam.h \
    nrtexe.h \
    usbcam.h \
    shared_include.h \
    predict.h

RESOURCES += \
    qtresource.qrc \
    qtresource.qrc

FORMS += \
        mainwindow.ui

INCLUDEPATH += /usr/include/opencv4

LIBS += -L/usr/lib/aarch64-linux-gnu \
        -lnrt

LIBS += -L/usr/lib \
        -lopencv_highgui \
        -lopencv_videoio \
        -lopencv_imgcodecs \
        -lopencv_imgproc \
        -lopencv_core \
        -lsqlite3
#        -lopencv_dnn \
#        -lopencv_ml \
#        -lopencv_objdetect \
#        -lopencv_shape \
#        -lopencv_stitching \
#        -lopencv_superres \
#        -lopencv_videostab \
#        -lopencv_calib3d \
#        -lopencv_features2d \
#        -lopencv_video \
#        -lopencv_photo \
#        -lopencv_flann \

