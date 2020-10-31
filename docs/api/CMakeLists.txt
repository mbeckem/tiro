set(DOXYGEN_INPUT_DIRS "${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include")
set(DOXYGEN_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
configure_file(Doxyfile.in Doxyfile @ONLY)
configure_file(header.html.in header.html @ONLY)

add_custom_target(
  doc
  COMMAND doxygen Doxyfile
  COMMENT "Generating API documentation with Doxygen"
  VERBATIM
)
