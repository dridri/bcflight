cmake_minimum_required( VERSION 2.8.11 )
set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../CMakeModules" )
include( add_submodule )

enable_language( C CXX ASM )
project( controller_pc )

SET( board "pc" CACHE BOOL "board" )

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g3 -std=c11 -D_GNU_SOURCE -D_BSD_SOURCE -Wfatal-errors" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g3 -std=c++11 -DGE_LINUX -DGE_EGL -fpermissive -Wno-pmf-conversions -Wfatal-errors" )
set( CMAKE_AUTOMOC ON )
set( CMAKE_AUTORCC ON )
set( OpenGL_GL_PREFERENCE LEGACY )

find_package( Nasm )
find_package( Qt5Widgets )
find_package( Qt5OpenGL )
find_package( Qt5PrintSupport )
find_package( Qt5Multimedia )
find_package( QScintilla2 )
find_package( MP4V2 )
find_package( Shine )

add_definitions( -DSTD=std )

if ( WIN32 OR MINGW )
	if ( NOT DEFINED ${QSCINTILLA_LIBRARY} OR ${QSCINTILLA_LIBRARY} MATCHES OFF )
		set( QSCINTILLA_LIBRARY -lqscintilla2 )
	endif()
	set( OPENGL_LIBRARIES -lopengl32 )
	set( OTHER_LIBS -lwinmm -lws2_32 )
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWIN32" )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32" )
else()
	find_package( OpenGL )
	set( OTHER_LIBS -ldl )
endif()


include_directories( ${CMAKE_SOURCE_DIR}/../lib/controller )
include_directories( ${CMAKE_SOURCE_DIR}/../lib/links )
include_directories( ${CMAKE_SOURCE_DIR}/../lib/rawwifi )
include_directories( ${CMAKE_SOURCE_DIR}/../lib/debug )
include_directories( ${CMAKE_SOURCE_DIR}/../external/LuaJIT-2.0.4/src )
include_directories( ${CMAKE_SOURCE_DIR}/../external/openh264-master/codec/api/svc )
include_directories( ${Qt5PrintSupport_INCLUDE_DIRS} ${QSCINTILLA_INCLUDE_DIR} ${QSCINTILLA_INCLUDE_DIR}/Qsci )
include_directories( ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/ui ${CMAKE_BINARY_DIR} )

add_submodule( controller )
add_submodule( debug )
if ( ${RAWWIFI_ON} MATCHES 1 )
	set( OTHER_LIBS ${OTHER_LIBS} rawwifi )
	if ( WIN32 OR MINGW )
		set( OTHER_LIBS ${NLGENL_LIBRARY} ${NLROUTE_LIBRARY} ${NL_LIBRARY} -ldl )
	endif()
endif()
if ( RAWWIFI_ON MATCHES 0 )
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_RAWWIFI" )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNO_RAWWIFI" )
endif()

set( OTHER_LIBS ${OTHER_LIBS} -lfftw3f )

add_custom_target( libopenh264_static COMMAND bash ${CMAKE_SOURCE_DIR}/../external/build_openh264.sh ${CMAKE_BINARY_DIR} ${CMAKE_C_COMPILER} )

qt5_wrap_ui( controller_pc_UI
				ui/mainWindow.ui
				ui/blackbox.ui
				ui/videoEditor.ui
				ui/config.ui
)

add_executable( controller_pc
				main.cpp
				Stream.cpp
				ControllerPC.cpp
				qcustomplot.cpp
				ui/HStatusBar.cpp
				ui/resources.qrc
				MainWindow.cpp
				BlackBox.cpp
				VideoEditor.cpp
				ui/VideoViewer.cpp
				Config.cpp
				tinyxml2.cpp
				${controller_pc_MOCS}
				${controller_pc_UI}
				)

add_dependencies( controller_pc libopenh264_static )
add_dependencies( controller_pc controller )
add_dependencies( controller_pc debuglib )

target_link_libraries( controller_pc -L./ controller debuglib -ldecoder_static -lopenh264_static ${SHINE_LIBRARIES} ${MP4V2_LIBRARIES} ${QSCINTILLA_LIBRARY} Qt5::Widgets Qt5::OpenGL Qt5::PrintSupport Qt5::Multimedia ${OPENGL_LIBRARIES} -lpthread ${OTHER_LIBS} )

install( TARGETS controller_pc RUNTIME DESTINATION bin )
