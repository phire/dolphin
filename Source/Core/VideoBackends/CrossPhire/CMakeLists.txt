set(SRCS main.cpp
         Render.cpp
		 VertexManager.cpp)

set(LIBS videocommon
         SOIL
         common
         ${X11_LIBRARIES})

add_dolphin_library(videosoftware "${SRCS}" "${LIBS}")
