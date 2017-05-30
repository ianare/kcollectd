
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAG} ${KDE4_ENABLE_EXCEPTIONS}")

find_package(Boost COMPONENTS filesystem system)

kde4_add_executable(kcollectd 
  graph.cc
  gui.cc
  kcollectd.cc
  misc.cc
  rrd_interface.cc
  timeaxis.cc)
set(rrd_LIBRARIES rrd)
include_directories(${KDE4_INCLUDES} ${Boost_INCLUDE_DIRS})
target_link_libraries(kcollectd 
  ${KDE4_KDEUI_LIBS} 
  ${KDE4_KIO_LIBS} 
  ${Boost_LIBRARIES} 
  ${rrd_LIBRARIES})
install(TARGETS kcollectd  ${INSTALL_TARGETS_DEFAULT_ARGS})

# desktop-file
install(FILES kcollectd.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})

# mime-type
# for KDE 4.2 we require at least version 0.23
set(SHARED_MIME_INFO_MINIMUM_VERSION "0.23")
find_package(SharedMimeInfo REQUIRED)
install(FILES kcollectd.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})

subdirs(icons)
