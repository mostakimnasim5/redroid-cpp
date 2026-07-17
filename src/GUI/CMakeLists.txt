# ReDroidCPP GUI Module - Professional Emulator UI

set(GUI_SOURCES
    main.cpp
    MainWindow.cpp
    PhoneWindow.cpp
    DashboardWindow.cpp
    AntiDetectionPanel.cpp
)

set(GUI_HEADERS
    MainWindow.h
    PhoneWindow.h
    DashboardWindow.h
    AntiDetectionPanel.h
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
    PRIVATE 
        ${CMAKE_SOURCE_DIR}/src/GUI
        ${CMAKE_SOURCE_DIR}/include
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
