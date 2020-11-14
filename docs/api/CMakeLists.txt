set(DOXYGEN_INPUT_DIRS "${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include")
set(DOXYGEN_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")

set(DOXYGEN_HTML_OUTPUT doxygen)
set(DOXYGEN_HEADER_HTML "${DOXYGEN_SOURCE_DIR}/header.html")
set(DOXYGEN_FOOTER_HTML "${DOXYGEN_SOURCE_DIR}/footer.html")
set(DOXYGEN_STYLE_CSS "${DOXYGEN_SOURCE_DIR}/style.css")
configure_file(Doxyfile.in Doxyfile @ONLY)

set(DOXYGEN_HTML_OUTPUT doxygen-embed)
set(DOXYGEN_HEADER_HTML "${DOXYGEN_SOURCE_DIR}/header.embed.html")
set(DOXYGEN_FOOTER_HTML "${DOXYGEN_SOURCE_DIR}/footer.embed.html")
set(DOXYGEN_STYLE_CSS "${DOXYGEN_SOURCE_DIR}/style.embed.css")
configure_file(Doxyfile.in Doxyfile.embed @ONLY)

add_custom_target(
  docs
  COMMAND doxygen Doxyfile
  COMMENT "Generating API documentation with Doxygen"
  VERBATIM
)

add_custom_target(
  docs-embed
  # Force clean build because image map ids are broken after rebuild...
  COMMAND ${CMAKE_COMMAND} -E remove_directory doxygen-embed
  COMMAND doxygen Doxyfile.embed
  COMMENT "Generating API documentation fragments for website with Doxygen"
  VERBATIM
)
