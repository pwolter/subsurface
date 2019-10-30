# install a few things so that one can run Subsurface from the build
# directory
add_custom_target(themeLink ALL
	COMMAND
	rm -f ${CMAKE_BINARY_DIR}/theme &&
	ln -sf ${CMAKE_SOURCE_DIR}/theme ${CMAKE_BINARY_DIR}/theme
)
if(NOT NO_PRINTING)
	add_custom_target(printing_templatesLink ALL
		COMMAND
		rm -f ${CMAKE_BINARY_DIR}/printing_templates &&
		ln -sf ${CMAKE_SOURCE_DIR}/printing_templates ${CMAKE_BINARY_DIR}/printing_templates
	)
endif()
if(NOT NO_DOCS)
	add_custom_target(
		documentationLink ALL
		COMMAND
		mkdir -p ${CMAKE_BINARY_DIR}/Documentation/ &&
		rm -rf ${CMAKE_BINARY_DIR}/Documentation/images &&
		ln -sf ${CMAKE_SOURCE_DIR}/Documentation/images ${CMAKE_BINARY_DIR}/Documentation/images &&
		ln -sf ${CMAKE_SOURCE_DIR}/Documentation/mobile-images ${CMAKE_BINARY_DIR}/Documentation/mobile-images
	)
	add_custom_target(
		documentation ALL
		COMMAND
		${CMAKE_MAKE_PROGRAM} -C ${CMAKE_SOURCE_DIR}/Documentation OUT=${CMAKE_BINARY_DIR}/Documentation/ doc
		DEPENDS documentationLink
	)
endif()
