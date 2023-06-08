#-------------------------------------------------
#
# Project created by QtCreator 2017-06-16T20:25:10
#
#-------------------------------------------------

QT       += core gui
RC_ICONS+=screenshot.ico
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = PathTest
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
#C:\Users\wangy\Desktop\OpenCV-MinGW-Build\include
#C:\Users\wangy\Desktop\OpenCV-MinGW-Build\x64\mingw\lib\libopencv_*.a
INCLUDEPATH +=  \
    E:\IanWong\JOB\opencv\build\include \
    E:\IanWong\JOB\Screenshot-with-picture-edit\onnxruntime\include \


LIBS += \
    E:\IanWong\JOB\opencv\build\x64\vc15\lib\opencv_world460d.lib \
    E:\IanWong\JOB\Screenshot-with-picture-edit\onnxruntime\lib\*.lib \

#win32 {
#    LIBS += -luser32
#}

SOURCES += main.cpp \
    beingsnap.cpp \
    controlwidget.cpp \
    myrect.cpp \
    ocr/text_angle_cls.cpp \
    ocr/text_det.cpp \
    ocr/text_rec.cpp \
    screen.cpp \
    qslabel.cpp \


HEADERS += \
    beingsnap.h \
    controlwidget.h \
    myrect.h \
    ocr/text_angle_cls.h \
    ocr/text_det.h \
    ocr/text_rec.h \
    screen.h \
    qslabel.h \


FORMS += \
    beingsnap.ui \
        widget.ui \
    controlwidget.ui

RESOURCES += \
    res.qrc

DISTFILES += \
    image/control_01.jpg \
    image/control_02.jpg \
    image/control_03.jpg \
    image/control_04.jpg \
    image/control_05.jpg \
    image/control_06.jpg \
    image/control_07.jpg \
    image/control_08.jpg \
    image/control_09.jpg \
    image/control_10.jpg \
    image/control_11.jpg \
    image/control_12.jpg \
    image/control_13.jpg \
    ../../Pictures/screen/screenshot.png



