
target_sources(square-color-tests PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/ControlsTest.cc
	)

target_include_directories(square-color-tests PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}"
	)
