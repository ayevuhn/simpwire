cmake_minimum_required(VERSION 3.10.2)
project(simpwire VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 11)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
#set(CMAKE_BUILD_TYPE Release)

option(BUILD_DEMO "Build demo program" OFF)
option(BUILD_TEST "Build test program" OFF)

if(WIN32)
  if(MSVC)
    # Unlike GCC or MINGW, MSVC does not automatically add the "lib" prefix to libraries.
    set(PROJECT_LINK_LIBS simpwire.dll Threads::Threads)
  else()
    set(PROJECT_LINK_LIBS libsimpwire.dll Threads::Threads)
  endif(MSVC)
else()
  set(PROJECT_LINK_LIBS libsimpwire.so Threads::Threads)
endif(WIN32)

link_directories(${CMAKE_BINARY_DIR})

if(WIN32)
  add_definitions(-DBUILDING_DLL)
endif(WIN32)

include_directories(include)
set(SOURCES 
    src/Peer.cpp 
    src/TcpNode.cpp
    src/Socket.cpp
    src/PeerPrivate.hpp
    src/PeerPrivate.cpp
    src/TcpNodePrivate.hpp
    src/TcpNodePrivate.cpp
    src/Socket.hpp
    src/ISocket.hpp
    include/Peer.hpp 
    include/TcpNode.hpp 
    include/common.hpp
    include/simpwire.hpp)


add_library(simpwire SHARED ${SOURCES})
#Set shared object version on Linux
set_target_properties(simpwire PROPERTIES VERSION ${PROJECT_VERSION}) 
target_include_directories(simpwire PRIVATE include)


if(WIN32)
  target_link_libraries(simpwire ws2_32)
  target_link_libraries(simpwire Threads::Threads)
else()
  target_link_libraries(simpwire Threads::Threads)
endif(WIN32)


if(BUILD_DEMO)
  add_executable(simpwire_demo demo/demo.cpp)
  add_dependencies(simpwire_demo simpwire)
  target_include_directories(simpwire_demo PRIVATE include)
  target_link_libraries(simpwire_demo ${PROJECT_LINK_LIBS})
endif(BUILD_DEMO)

if(BUILD_TEST)
  #enable_testing()
  set(GTEST_INC_DIR googletest/googletest/include)
  set(GMOCK_INC_DIR googletest/googlemock/include)
  add_subdirectory(googletest)
  add_executable(
    simpwire_test
    test/simpwire_test.cpp
    test/mock_socket.hpp
    test/tst_gtest.hpp
    test/tst_socket.hpp
    test/tst_tcpnode.hpp
    ${SOURCES})
  target_include_directories(simpwire_test PUBLIC include)
  target_include_directories(simpwire_test PUBLIC ${GTEST_INC_DIR})
  target_include_directories(simpwire_test PUBLIC ${GMOCK_INC_DIR})
  target_link_libraries(simpwire_test PRIVATE gtest gmock)
  if(WIN32)
    target_link_libraries(simpwire_test PRIVATE ws2_32 gtest gmock)
  else()
    target_link_libraries(simpwire_test PRIVATE gtest gmock)
  endif(WIN32)
  #add_test(NAME simpwire_test COMMAND simpwire_test)

endif(BUILD_TEST)


#Install (not on Windows)

if(NOT WIN32)
  install(TARGETS simpwire LIBRARY DESTINATION lib)
  install(FILES 
          ${CMAKE_SOURCE_DIR}/include/Peer.hpp
          ${CMAKE_SOURCE_DIR}/include/TcpNode.hpp
          ${CMAKE_SOURCE_DIR}/include/simpwire.hpp
          ${CMAKE_SOURCE_DIR}/include/common.hpp
          DESTINATION include)
endif(NOT WIN32)

