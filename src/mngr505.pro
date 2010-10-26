QT              += xml
HEADERS		= \
		  BookData.h         \
		  Config.h           \
		  DeviceSelect.h     \
		  EPUB.h             \
		  EPUBParser.h       \
		  FB2Parser.h        \
		  FPanel.h           \
		  FPanelItem.h       \
		  Info.h             \
		  LRFParser.h        \
		  LogWidget.h        \
		  Media.h            \
                  NameEdit.h         \
		  unzip.h            \
		  unzip_p.h          \
		  utils.h            \
		  zipentry_p.h       \
                  Confirm.h          \
                  FB2toEPUB.h        \
                  FB2toLRF.h         \
                  Viewer.h           \
                  zip.h              \
                  zip_p.h            \
                  mngr505.h
TARGET		= mngr505
FORMS           = mngr505.ui LogWidget.ui Config.ui Confirm.ui
RESOURCES       = mngr505.qrc
RC_FILE         = mngr505.rc
SOURCES         = \
		  BookData.cpp       \
		  Config.cpp         \
		  DeviceSelect.cpp   \
		  EPUB.cpp           \
		  EPUBParser.cpp     \
		  FB2Parser.cpp      \
		  FPanel.cpp         \
		  FPanelItem.cpp     \
		  Info.cpp           \
		  LRFParser.cpp      \
		  LogWidget.cpp      \
		  Media.cpp          \
                  NameEdit.cpp       \
		  unzip.cpp          \
		  utils.cpp          \
                  Confirm.cpp        \
                  FB2toEPUB.cpp      \
                  FB2toLRF.cpp       \
                  Viewer.cpp         \
                  mngr505.cpp        \
                  zip.cpp            \
                  main.cpp
CONFIG += qt release
win32 {
    message(Building for Windows)
    SOURCES     += umount_win.cpp
    INCLUDEPATH += c:\MinGW\usr\include\mingw
    DEFINES     += WINDOWS
}
unix {
    macx {
        message(Building for MacOSX)
        SOURCES     += umount_lin.cpp
        DEFINES     += MACOSX
    } else {
        message(Building for Linux)
        SOURCES     += umount_lin.cpp
        DEFINES     += LINUX
    }
}
