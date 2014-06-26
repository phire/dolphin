set(SRCS
	   main.cpp
	   Render.cpp
	   VertexManager.cpp)

set(LIBS	videocommon
			SOIL
			common
			dl
			${X11_LIBRARIES})
if(USE_EGL)
	set(LIBS ${LIBS}
		EGL)
endif()

if(wxWidgets_FOUND)
	set(LIBS	${LIBS} ${wxWidgets_LIBRARIES})
endif(wxWidgets_FOUND)

if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD" OR
	${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
	set(LIBS	${LIBS} usbhid)
endif()

add_dolphin_library(videonull "${SRCS}" "${LIBS}")
