if(CONFIG_ZMK_DISPLAY AND CONFIG_NICE_VIEW_LILY_STATUS)
  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
  zephyr_library_sources(status_screen.c nvl_render.c nvl_assets.c)
endif()
