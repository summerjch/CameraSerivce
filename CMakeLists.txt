cmake_minimum_required(VERSION 3.15)
project(CameraService)

find_package(OpenCV REQUIRED)
find_package(Boost REQUIRED)

add_executable(frame_capture src/frame_capture.cpp)
target_link_libraries(frame_capture ${OpenCV_LIBS})

add_executable(frame_reader src/frame_reader.cpp)
target_link_libraries(frame_reader ${OpenCV_LIBS})

add_executable(optical_flow src/optical_flow.cpp)
target_link_libraries(optical_flow ${OpenCV_LIBS})

add_executable(circle_detection src/circle_detection.cpp)
target_link_libraries(circle_detection ${OpenCV_LIBS})

# add_executable(test1 src/test1.cpp)
# target_link_libraries(test1 ${OpenCV_LIBS})

# add_executable(test2 src/test2.cpp)
# target_link_libraries(test2 ${OpenCV_LIBS})
