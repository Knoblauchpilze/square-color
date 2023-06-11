
target_sources (square-color_lib PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/olcEngine.cc
	${CMAKE_CURRENT_SOURCE_DIR}/TexturePack.cc
	${CMAKE_CURRENT_SOURCE_DIR}/PGEApp.cc
	)

target_include_directories (square-color_lib PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}"
	)
