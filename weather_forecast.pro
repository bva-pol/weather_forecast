QT       += core gui network charts
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = build/weather_forecast
MOC_DIR = build/.moc
OBJECTS_DIR = build/.obj
UI_DIR = src/ui/headers

CONFIG += c++17

SOURCES += \
    src/CityEditor.cpp \
    src/CityManager.cpp \
    src/main.cpp \
    src/MainWindow.cpp \
    src/WeatherApi.cpp

HEADERS += \
    src/CityEditor.hpp \
    src/CityManager.hpp \
    src/MainWindow.hpp \
    src/WeatherApi.hpp

FORMS += \
    src/ui/CityEditor.ui \
    src/ui/MainWindow.ui \
