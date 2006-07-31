# File generated by kdevelop's qmake manager. 
# ------------------------------------------- 
# Subdir relative project main directory: ./src/core
# Target is a library:  traversocore

include(../libbase.pri)

PRECOMPILED_HEADER = precompile.h 

LIBS += -ltraversocommands \
        -ltraverso 
        
INCLUDEPATH += ../../src/traverso \
               ../../src/traverso/build \
               ../../src/core \
               ../../src/commands \
               ../../src/engine \
               ../../src/plugins/LV2 \
               ../../src/plugins \
               . 
QMAKE_LIBDIR = ../../lib 
TARGET = traversocore 
DESTDIR = ../../lib 

TEMPLATE = lib 

SOURCES	= AudioClip.cpp \
	AudioClipList.cpp \
	AudioClipManager.cpp \
	AudioSource.cpp \
	AudioSourceManager.cpp \
	Command.cpp \
	ContextItem.cpp \
	ContextPointer.cpp \
	Curve.cpp \
	CurveNode.cpp \
	Debugger.cpp \
	DiskIO.cpp \
	Export.cpp \
	FileHelpers.cpp \
	HistoryStack.cpp \
	IEAction.cpp \
	IEMessage.cpp \
	Information.cpp \
	InputEngine.cpp \
	Mixer.cpp \
	MtaRegion.cpp \
	MtaRegionList.cpp \
	Peak.cpp \
	Project.cpp \
	ProjectManager.cpp \
	ReadSource.cpp \
	RingBuffer.cpp \
	Song.cpp \
	Track.cpp \
	Tsar.cpp \
	Utils.cpp \
	WriteSource.cpp \
	gdither.cpp \
	SnapList.cpp

HEADERS	= precompile.h \
	AudioClip.h \
	AudioClipList.h \
	AudioClipManager.h \
	AudioSource.h \
	AudioSourceManager.h \
	Command.h \
	ContextItem.h \
	ContextPointer.h \
	CurveNode.h \
	Curve.h \
	Debugger.h \
	DiskIO.h \
	Export.h \
	FileHelpers.h \
	HistoryStack.h \
	IEAction.h \
	IEMessage.h \
	Information.h \
	InputEngine.h \
	Mixer.h \
	MtaRegion.h \
	MtaRegionList.h \
	Peak.h \
	Project.h \
	ProjectManager.h \
	ReadSource.h \
	RingBuffer.h \
	Song.h \
	Track.h \
	Tsar.h \
	Utils.h \
	WriteSource.h \
	libtraversocore.h \
	gdither.h \
	gdither_types.h \
	gdither_types_internal.h \
	noise.h \
	FastDelegate.h \
	SnapList.h
	
macx {
	QMAKE_LIBDIR += /usr/local/qt/lib
}

