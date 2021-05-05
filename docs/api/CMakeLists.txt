set(DOXYGEN_INPUT_DIRS "${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include")
set(DOXYGEN_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")

set(DOXYGEN_HTML_OUTPUT doxygen)
set(DOXYGEN_HEADER_HTML "${DOXYGEN_SOURCE_DIR}/header.html")
set(DOXYGEN_FOOTER_HTML "${DOXYGEN_SOURCE_DIR}/footer.html")
set(DOXYGEN_STYLE_CSS "${DOXYGEN_SOURCE_DIR}/style.css")
configure_file(Doxyfile.in Doxyfile @ONLY)

add_custom_target(
  docs
  COMMAND doxygen Doxyfile
  COMMENT "Generating API documentation with Doxygen"
  VERBATIM
)
