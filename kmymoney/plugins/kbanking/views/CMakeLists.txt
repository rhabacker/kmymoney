
########### next target ###############

set(kmm_kbanking_views_la_SOURCES
    kbjobview.cpp
    )

set(kmm_kbanking_views_UI
    kbjobview.ui
    )

ki18n_wrap_ui(kmm_kbanking_views_la_SOURCES
  ${kmm_kbanking_views_UI})

add_library(kmm_kbanking_views STATIC ${kmm_kbanking_views_la_SOURCES})

target_link_libraries(kmm_kbanking_views
  Qt5::Widgets
)
