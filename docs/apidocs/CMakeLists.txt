set(DOXYFILE_IN "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in")
set(DOXYFILE "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")

set(DOXYGEN_INPUT_DIRS "${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include")
set(DOXYGEN_OUTPUT_DIR "${PROJECT_BINARY_DIR}/apidoc")
configure_file("${DOXYFILE_IN}" "${DOXYFILE}" @ONLY)

add_custom_target(
  doc
  COMMAND doxygen "${DOXYFILE}"
  COMMENT "Generating API documentation with Doxygen"
  VERBATIM
)
