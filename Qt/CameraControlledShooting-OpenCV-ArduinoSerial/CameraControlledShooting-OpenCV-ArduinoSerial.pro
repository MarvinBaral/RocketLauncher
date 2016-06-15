QT += core serialport
QT -= gui

TARGET = CameraControlledShooting-OpenCV-ArduinoSerial
CONFIG += console
CONFIG += c++11
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    openCV.cpp \
    servoControl.cpp

INCLUDEPATH += /usr/local/include/opencv
LIBS += -L/usr/local/lib -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_video -lopencv_videoio -lopencv_videostab

HEADERS += \
    servoControl.hpp \
    openCV.hpp
