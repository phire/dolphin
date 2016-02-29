set(SRCS
	   main.cpp
	   MemoryAllocator.cpp
	   Renderer.cpp
	   VertexManager.cpp
	   )

set(LIBS	${LIBS}
			videocommon
			SOIL
			common
			${X11_LIBRARIES})

add_dolphin_library(videovk "${SRCS}" "${LIBS}")
