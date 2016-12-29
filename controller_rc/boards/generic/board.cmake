find_package( Qt5Widgets REQUIRED )

include_directories( ${Qt5Widgets_INCLUDE_DIRS} )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_BSD_SOURCE -D_GNU_SOURCE -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE")

set( BOARD_LIBS ${BOARD_LIBS} Qt5::Widgets Qt5::Core )
set( UIC qtchooser -qt=qt5 -run-tool=uic )
set( RCC qtchooser -qt=qt5 -run-tool=rcc )
set( MOC qtchooser -qt=qt5 -run-tool=moc )
