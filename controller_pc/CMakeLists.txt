cmake_minimum_required( VERSION 2.8.11 )
set( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules" )
project( controller_pc )

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g3 -std=c11 -D_GNU_SOURCE -D_BSD_SOURCE" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g3 -std=c++11 -DGE_LINUX -DGE_EGL -fpermissive -Wno-pmf-conversions" )
set( CMAKE_AUTOMOC ON )
set( CMAKE_AUTORCC ON )

find_package( Nasm )
find_package( Qt5Widgets )
find_package( Qt5OpenGL )
find_package( Qt5PrintSupport )
find_package( QScintilla2 )
find_package( OpenGL )

add_subdirectory( ${CMAKE_SOURCE_DIR}/../libcontroller libcontroller )
include_directories( ${CMAKE_SOURCE_DIR}/../libcontroller )
include_directories( ${CMAKE_SOURCE_DIR}/../libcontroller/links )
include_directories( ${CMAKE_SOURCE_DIR}/../librawwifi )
include_directories( ${CMAKE_SOURCE_DIR}/../external/LuaJIT-2.0.4/src )
include_directories( ${CMAKE_SOURCE_DIR}/../external/openh264-master/codec/api/svc )

include_directories( ${Qt5PrintSupport_INCLUDE_DIRS} ${QSCINTILLA_INCLUDE_DIR} ${QSCINTILLA_INCLUDE_DIR}/Qsci )
include_directories( ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/ui ${CMAKE_BINARY_DIR} )

add_custom_target( libluajit_static COMMAND bash ${CMAKE_SOURCE_DIR}/../external/build_luajit.sh ${CMAKE_BINARY_DIR} ${CMAKE_C_COMPILER} ${CMAKE_ASM_COMPILER} ${CROSS}strip "" )
add_custom_target( libopenh264_static COMMAND bash ${CMAKE_SOURCE_DIR}/../external/build_openh264.sh ${CMAKE_BINARY_DIR} )

qt5_wrap_ui( controller_pc_UI
				ui/mainWindow.ui
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
				Config.cpp
				tinyxml2.cpp
				${controller_pc_MOCS}
				${controller_pc_UI}
				)

add_dependencies( controller_pc libopenh264_static )
add_dependencies( controller_pc libluajit_static )
add_dependencies( controller_pc controller )

target_link_libraries( controller_pc -L./ controller rawwifi -lluajit_static -ldecoder_static -lopenh264_static ${QSCINTILLA_LIBRARY} Qt5::Widgets Qt5::OpenGL Qt5::PrintSupport ${OPENGL_LIBRARIES} -lpcap -liw -lpthread -ldl )

install( TARGETS controller_pc RUNTIME DESTINATION bin )
