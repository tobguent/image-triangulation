SET(SOURCEFILES
ImmediateRenderer2D.fx
Image2DView.fx
DisplayTriangulation.fx
RasterizeVariance.fx
RasterizePlane.fx
Brushing.fx
)

SET(HLSLSOURCES 
CopyToStaging.hlsl
UpdatePosition.hlsl
SolvePlane.hlsl
SaturateTexture.hlsl
)

ADD_CUSTOM_TARGET(imagetri_fx ALL SOURCES ${SOURCEFILES} ${HLSLSOURCES})

foreach(FILE ${SOURCEFILES})
  GET_FILENAME_COMPONENT(FILE_WE ${FILE} NAME_WE)
  ADD_CUSTOM_COMMAND(OUTPUT ${FILE_WE}.fxo
                     COMMAND ${DirectX_FXC_EXECUTABLE} /Fo ${CMAKE_BINARY_DIR}/$<CONFIGURATION>/${FILE_WE}.fxo ${CMAKE_CURRENT_SOURCE_DIR}/${FILE} /nologo /O2 /Tfx_5_0
                     MAIN_DEPENDENCY ${FILE}
                     COMMENT "Effect-compile ${FILE} \n ${COMMAND}"
                     VERBATIM)
  ADD_DEPENDENCIES(imagetri_fx ${FILE_WE}.fxo)
endforeach(FILE)

foreach(HLSLSOURCE ${HLSLSOURCES})
  GET_FILENAME_COMPONENT(FILE_WE ${HLSLSOURCE} NAME_WE)
  ADD_CUSTOM_COMMAND(OUTPUT ${FILE_WE}.fxo
                     COMMAND ${DirectX_FXC_EXECUTABLE} /Fo ${CMAKE_BINARY_DIR}/$<CONFIGURATION>/${FILE_WE}.fxo ${CMAKE_CURRENT_SOURCE_DIR}/${HLSLSOURCE} /nologo /O2 /Tcs_5_0
                     MAIN_DEPENDENCY ${HLSLSOURCE}
                     COMMENT "Effect-compile ${HLSLSOURCE} \n ${COMMAND}"
                     VERBATIM)
  ADD_DEPENDENCIES(imagetri_fx ${FILE_WE}.fxo)
endforeach(HLSLSOURCE)
