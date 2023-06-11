
target_sources (square-color_lib PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/Game.cc
	${CMAKE_CURRENT_SOURCE_DIR}/SavedGames.cc
	${CMAKE_CURRENT_SOURCE_DIR}/GameState.cc

	${CMAKE_CURRENT_SOURCE_DIR}/Board.cc
	)

target_include_directories (square-color_lib PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}"
	)
