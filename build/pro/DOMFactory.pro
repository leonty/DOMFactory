#-------------------------------------------------
#
# Project created by QtCreator 2012-04-01T16:08:38
#
#-------------------------------------------------

QT       += xml xmlpatterns

QT       -= gui

TARGET = DOMFactory
TEMPLATE = lib
CONFIG += staticlib

SOURCES += ../../src/DomFactory.cpp

HEADERS += ../../src/DomFactory.h

unix:!symbian {
	maemo5 {
		target.path = /opt/usr/lib
	} else {
		target.path = /usr/lib
	}
	INSTALLS += target
}
