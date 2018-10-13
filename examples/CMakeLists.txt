cmake_minimum_required(VERSION 2.8.4)
project(examples)

SET (LIBIIO_LIBRARY iio)

SET (common_link_libs ${LIBIIO_LIBRARY})
include_directories(../)

ADD_EXECUTABLE (testLibIIO testlibiio.cpp)
TARGET_LINK_LIBRARIES (testLibIIO ${common_link_libs})

ADD_EXECUTABLE (test_ad9361 test_ad9361.cpp)
TARGET_LINK_LIBRARIES (test_ad9361 ${common_link_libs})

