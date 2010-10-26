HEADERS		= \
		  Config.h           \
		  EPUBed.h           \
		  ../unzip.h         \
		  ../unzip_p.h       \
		  ../utils.h         \
		  ../zipentry_p.h    \
                  ../zip.h           \
                  ../zip_p.h
TARGET		= EPUBed
FORMS           = EPUBed.ui Config.ui
RESOURCES       =
RC_FILE         =
SOURCES         = \
		  Config.cpp         \
		  EPUBed.cpp         \
		  ../unzip.cpp       \
                  ../zip.cpp         \
                  main.cpp
INCLUDEPATH += ..
CONFIG += qt release

win32 {
    message(Building for Windows)
    INCLUDEPATH += c:\MinGW\usr\include\mingw
    DEFINES     += WINDOWS
}
unix {
    macx {
        message(Building for MacOSX)
        DEFINES     += MACOSX
    } else {
        message(Building for Linux)
        DEFINES     += LINUX
    }
}
