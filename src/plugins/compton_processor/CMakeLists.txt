#
# CMake configuration for mollerpol_processor plugin
#
find_package(JANA REQUIRED)

add_jana_plugin(mollerpol_processor
    SOURCES
        InitPlugin.cc
        JEventProcessor_mollerpol.cc
)

target_link_libraries(mollerpol_processor
    PRIVATE
        ${JANA_LIBRARY}
        ROOT::Core
        ROOT::RIO
        ROOT::Tree
        ROOT::Hist
        jana2_common_extensions::evio_common_modules_data_types  # data-object headers (FADC, TIScaler, MPD, VFTDC, ...)
)
