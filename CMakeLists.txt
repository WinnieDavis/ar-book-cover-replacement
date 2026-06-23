CMAKE_MINIMUM_REQUIRED(VERSION "4.0")

# This is set manually for the lab computers
set(CMAKE_PREFIX_PATH "C:/Program Files/opencv/build")
# If you are working on your own computer:
# - First try commenting it out with a # at the start of line
# - If that does not work, set the path to the directory containing OpenCVConfig.cmake
#   This will depend on how you installed OpenCV.

set(CMAKE_CXX_STANDARD 17)

project( ObjectReplacement )

find_package(OpenCV REQUIRED)

include_directories( ${OpenCV_INCLUDE_DIRS} )

add_executable( objectReplacement objectReplacementMain.cpp Timer.h Timer.cpp)

target_link_libraries(objectReplacement
  ${OpenCV_LIBS}
)

set_target_properties (
	objectReplacement PROPERTIES 
	VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)