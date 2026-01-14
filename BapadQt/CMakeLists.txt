cmake_minimum_required(VERSION 3.19)
project(BapadQt LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)
find_package(Microsoft.GSL CONFIG REQUIRED)
find_package(ICU REQUIRED COMPONENTS i18n uc data)

qt_standard_project_setup()

set(TEXT_DOC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Bapad/TextDocument")

set(APP_SOURCES
    main.cpp
    BapadQt.cpp
    TextView.cpp
    "${TEXT_DOC_DIR}/FormatConversionV2.cpp"
    "${TEXT_DOC_DIR}/PieceTree.cpp"
    "${TEXT_DOC_DIR}/TextDocument.cpp"

)

set(APP_HEADERS
    BapadQt.h
    BapadQt.ui
    TextView.h
    "${TEXT_DOC_DIR}/FormatConversionV2.h"
    "${TEXT_DOC_DIR}/PieceTree.h"
    "${TEXT_DOC_DIR}/TextDocument.h"
)

qt_add_executable(BapadQt
    WIN32 MACOSX_BUNDLE
    ${APP_SOURCES}
    ${APP_HEADERS}
    ../Bapad/TextDocument/TextDocument.h ../Bapad/TextDocument/TextDocument.cpp ../Bapad/TextDocument/PieceTree.h ../Bapad/TextDocument/PieceTree.cpp ../Bapad/TextDocument/FormatConversionV2.h ../Bapad/TextDocument/FormatConversionV2.cpp
)
target_include_directories(BapadQt PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${TEXT_DOC_DIR}"       # 搜索 BapadQt 内部
)

target_link_libraries(BapadQt
    PRIVATE
    Qt::Core
    Qt::Widgets
    ICU::i18n
    ICU::uc
    ICU::data
    Microsoft.GSL::GSL
)

include(GNUInstallDirs)

install(TARGETS BapadQt
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET BapadQt
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
message(STATUS "ICU_LIBRARIES: ${ICU_LIBRARIES}")
