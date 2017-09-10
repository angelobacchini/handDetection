TEMPLATE += app

QT += widgets \
      charts \
      multimedia

CONFIG += c++11

TARGET = handDetection

#INCLUDEPATH += C:\opencv\release\install\include
#LIBS += C:\opencv\release\bin\libopencv_core320.dll \
#        C:\opencv\release\bin\libopencv_videoio320.dll \
#        C:\opencv\release\bin\libopencv_imgproc320.dll \
#        C:\opencv\release\bin\libopencv_video320.dll

INCLUDEPATH += /usr/include/opencv
LIBS += -lopencv_core \
        -lopencv_videoio \
        -lopencv_imgproc \
        -lopencv_video \
        -lopencv_highgui \
        -lopencv_imgcodecs

SOURCES += $$PWD/src/*.cpp \

HEADERS += $$PWD/inc/*.h \

INCLUDEPATH += $$PWD/inc/

DEFINES *= QT_USE_QSTRINGBUILDER
