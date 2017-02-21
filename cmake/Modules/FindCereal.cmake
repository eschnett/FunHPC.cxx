find_package(PkgConfig)
pkg_check_modules(PC_CEREAL QUIET cereal)

find_path(CEREAL_INCLUDE_DIR cereal/cereal.hpp
  HINTS
  ${CEREAL_ROOT} ENV CEREAL_ROOT
  ${PC_CEREAL_MINIMAL_INCLUDEDIR}
  ${PC_CEREAL_MINIMAL_INCLUDE_DIRS}
  ${PC_CEREAL_INCLUDEDIR}
  ${PC_CEREAL_INCLUDE_DIRS}
  PATH_SUFFIXES include)

# Cereal has no library files
set(CEREAL_LIBRARY)

set(CEREAL_INCLUDE_DIRS ${CEREAL_INCLUDE_DIR})
set(CEREAL_LIBRARIES ${CEREAL_LIBRARY})

# Cereal has no library files
find_package_handle_standard_args(
  Cereal DEFAULT_MSG CEREAL_INCLUDE_DIR)

get_property(_type CACHE CEREAL_ROOT PROPERTY TYPE)
if(_type)
  set_property(CACHE CEREAL_ROOT PROPERTY ADVANCED 1)
  if("x${_type}" STREQUAL "xUNINITIALIZED")
    set_property(CACHE CEREAL_ROOT PROPERTY TYPE PATH)
  endif()
endif()

mark_as_advanced(CEREAL_ROOT CEREAL_INCLUDE_DIR CEREAL_LIBRARY)
