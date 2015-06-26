CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# install edj
ADD_CUSTOM_COMMAND(OUTPUT volume_app.edj
                COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/images/private/volume
                ${CMAKE_CURRENT_SOURCE_DIR}/volume_app.edc ${CMAKE_BINARY_DIR}/volume_app.edj
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/volume_app.edc
)

ADD_CUSTOM_TARGET(edj_build DEPENDS volume_app.edj)
ADD_DEPENDENCIES(${PROJECT_NAME} edj_build)
INSTALL(FILES ${CMAKE_BINARY_DIR}/volume_app.edj DESTINATION ${EDJDIR})

# install images
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/images/public/ DESTINATION ${IMAGEDIR} FILES_MATCHING PATTERN "*.png")
