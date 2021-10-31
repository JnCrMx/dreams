file(GLOB_RECURSE copy_textures *.png)

foreach(texture ${copy_textures})
	file(RELATIVE_PATH rel ${CMAKE_CURRENT_SOURCE_DIR} ${texture})
	get_filename_component(dst ${rel} DIRECTORY)

	file(COPY ${texture} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${dst})
	install(FILES ${texture} DESTINATION ${CMAKE_INSTALL_BINDIR}/assets/textures/${dst})
endforeach()
