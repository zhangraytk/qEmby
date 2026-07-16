qt_add_executable(qemby_input_tests
    input_tests.cpp
    ../src/qEmbyApp/utils/inputnavigation.cpp
    ../src/qEmbyApp/utils/inputnavigation.h
    ../src/qEmbyApp/utils/smoothscrollcontroller.cpp
    ../src/qEmbyApp/utils/smoothscrollcontroller.h
    ../src/qEmbyApp/utils/wheelinput.cpp
    ../src/qEmbyApp/utils/wheelinput.h
)

target_include_directories(qemby_input_tests PRIVATE
    ../src/qEmbyApp/utils
)

target_link_libraries(qemby_input_tests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
)

add_test(NAME qemby_input_tests COMMAND qemby_input_tests)
set_tests_properties(qemby_input_tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_add_executable(qemby_shortcut_tests
    shortcutedit_tests.cpp
    ../src/qEmbyApp/components/shortcutedit.cpp
    ../src/qEmbyApp/components/shortcutedit.h
    ../src/qEmbyApp/utils/shortcututils.cpp
    ../src/qEmbyApp/utils/shortcututils.h
)

target_include_directories(qemby_shortcut_tests PRIVATE
    ../src/qEmbyApp/components
    ../src/qEmbyApp/utils
    ../src/qEmbyCore
)

target_link_libraries(qemby_shortcut_tests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
    qEmbyCore
)

add_test(NAME qemby_shortcut_tests COMMAND qemby_shortcut_tests)
set_tests_properties(qemby_shortcut_tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_add_executable(qemby_sliding_stack_tests
    slidingstackedwidget_tests.cpp
    ../src/qEmbyApp/components/slidingstackedwidget.cpp
    ../src/qEmbyApp/components/slidingstackedwidget.h
)

target_include_directories(qemby_sliding_stack_tests PRIVATE
    ../src/qEmbyApp/components
    ../src/qEmbyCore
)

target_link_libraries(qemby_sliding_stack_tests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
    qEmbyCore
)

add_test(NAME qemby_sliding_stack_tests COMMAND qemby_sliding_stack_tests)
set_tests_properties(qemby_sliding_stack_tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
