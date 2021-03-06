QT += core serialport
QT -= gui

TARGET = CameraControlledShooting-OpenCV-ArduinoSerial
CONFIG += console
CONFIG += c++11
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    servoControl.cpp \
    cameraControl.cpp \
    missionControlCenter.cpp \
    config.cpp

INCLUDEPATH += /usr/local/include/opencv
#LIBS += -L/usr/local/lib -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_videoio -lopencv_imgcodecs  -lopencv_videostab  -lopencv_video
LIBS += -L/usr/lib/arm-linux-gnueabihf -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_video
#-lopencv_videoio -lopencv_imgcodecs  -lopencv_videostab
HEADERS += \
    missionControlCenter.h \
    cameraControl.h \
    servoControl.h \
    main.h \
    config.h
