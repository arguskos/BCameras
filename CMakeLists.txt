cmake_minimum_required(VERSION 2.8)

message(STATUS "## ${CMAKE_CURRENT_BINARY_DIR} ##")
get_filename_component(EXAMPLE_NAME "${CMAKE_CURRENT_BINARY_DIR}" NAME)
string(SUBSTRING ${EXAMPLE_NAME} 0 3 TARGET_NAME)
project(${TARGET_NAME})



# =================== Add openCV support! ==============================
if(ENABLE_OPENCV)
    message(STATUS "\n======================= Configure OpenCV library =======================")
    find_package(OpenCV QUIET COMPONENTS core highgui imgproc)  # some pckg in OpenCV obsolet ?!?
    if(OpenCV_FOUND)
        message(STATUS "OpenCV Version: '${OpenCV_VERSION}'")
        message(STATUS "OpenCV LibDir:  '${OpenCV_LIB_PATH}'")
        if(NOT "${OpenCV_LIB_PATH}" STREQUAL "")
            get_filename_component(OpenCV_BIN_PATH "${OpenCV_LIB_PATH}/../bin" ABSOLUTE)
            if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
                string(REPLACE "/" "\\" OpenCV_BIN_PATH "${OpenCV_BIN_PATH}")
            endif()
        else()
            set(OpenCV_BIN_PATH "${_OpenCV_LIB_PATH}")
        endif()
        message(STATUS "OpenCV BinDir:  '${OpenCV_BIN_PATH}'")

        if(OpenCV_VERSION_MAJOR GREATER "2")
            add_definitions(-DUSE_OCL_COMPONENT=3)
            find_package(OpenCV COMPONENTS videoio imgcodecs)
            if(NOT OpenCV_FOUND)
                message(STATUS "OpenCV has no Video support!")
            endif()
        elseif (OpenCV_VERSION_MAJOR LESS "2")
            set(OpenCV_FOUND FALSE)
            message(STATUS "OpenCV with Version 1.x not supported!")
        else()  # OpenCV Version 2.x
            find_package(OpenCV COMPONENTS ocl)  # some pckg in OpenCV obsolet ?!?
        endif()
    else()
        message(STATUS "OpenCV not supported!")
    endif()

    if(OpenCV_FOUND AND ENABLE_OPENCV)
        add_definitions(-DUSE_OPENCV=1)
        list(APPEND INCLUDE_DIR ${OpenCV_INCLUDE_DIRS})
        message(STATUS "OpenCV Libs:  '${OpenCV_LIBS}'")
    else()
        message(STATUS "This means program '${BGAPI2_EXAMPLE}' couldn't run properly!")
        message(STATUS "Please set the correct search path for the OpenCV package and/or use OpenCV > 3.0!")
        message(STATUS "=====================================================================\n")
    endif()
endif()


set(SOURCE_FILES ${EXAMPLE_NAME}.cpp)
add_executable(${TARGET_NAME} ${SOURCE_FILES})
target_link_libraries(${TARGET_NAME} bgapi::bgapi2_genicam)
if(OpenCV_FOUND)
    target_link_libraries(${TARGET_NAME} "${OpenCV_LIBS}")
endif()
set_property(TARGET ${TARGET_NAME} PROPERTY OUTPUT_NAME ${EXAMPLE_NAME})
set_property(TARGET ${TARGET_NAME} PROPERTY PROJECT_LABEL ${EXAMPLE_NAME})

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(USER_FILE ${TARGET_NAME}.vcxproj.user)
    configure_file(UserTemplate.user ${USER_FILE} @ONLY)

    foreach(BGPAI2_FILE ${BGAPI2_SDK})
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BGPAI2_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${CFG_OUT_FOLDER})
    endforeach()
endif()
