# ReDroidCPP GUI Module

set(GUI_SOURCES
    main.cpp
    MainWindow.cpp
)

set(GUI_HEADERS
    MainWindow.h
)

# Create GUI executable
add_executable(redroid-gui
    ${GUI_SOURCES}
    ${GUI_HEADERS}
)

# Link Qt6 libraries
target_link_libraries(redroid-gui
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

# Include directories
target_include_directories(redroid-gui
    PRIVATE ${CMAKE_SOURCE_DIR}/src/GUI
)

# Install target
install(TARGETS redroid-gui
    RUNTIME DESTINATION bin
)

# Qt6 macOS specific
if(APPLE)
    set_target_properties(redroid-gui PROPERTIES
        MACOSX_BUNDLE TRUE
    )
endif()
