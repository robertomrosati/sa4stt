project(stt)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)

find_package(Boost 1.70 COMPONENTS coroutine context REQUIRED)

file(GLOB sources *.cc)
file(GLOB headers *.hh)
include(easylocal-ide)

add_subdirectory(pugixml)

set(SOURCE_FILES ${sources} ${headers})
add_executable(stt ${SOURCE_FILES})
target_compile_options(stt PUBLIC -Wall -Wpedantic)
target_link_libraries(stt EasyLocal pugixml Boost::context)
