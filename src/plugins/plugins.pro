include(../libbase.pri)


TARGET = traversoplugins 
DESTDIR = ../../lib 
TEMPLATE = lib 

INCLUDEPATH += ../core \
		../engine \
		../commands \
		LV2 \
		native \
		
SOURCES += PluginChain.cpp \
Plugin.cpp \
PluginManager.cpp \
PluginSlider.cpp \
native/CorrelationMeter.cpp \
native/SpectralMeter.cpp \
LV2/LV2Plugin.cpp \
LV2/LV2PluginPropertiesDialog.cpp
HEADERS += PluginChain.h \
Plugin.h \
PluginManager.h \
PluginSlider.h \
native/CorrelationMeter.h \
native/SpectralMeter.h \
LV2/LV2Plugin.h \
LV2/LV2PluginPropertiesDialog.h

!contains(DEFINES, LV2_SUPPORT) {
	HEADERS -= 	LV2/LV2Plugin.h \
			LV2/LV2ControlPort.h \
			LV2/LV2PluginPropertiesDialog.h
	SOURCES -= 	LV2/LV2Plugin.cpp \
			LV2/LV2ControlPort.cpp \
			LV2/LV2PluginPropertiesDialog.cpp
	INCLUDEPATH -= LV2
}

win32{
    INCLUDEPATH += ../../3thparty/include .
}
