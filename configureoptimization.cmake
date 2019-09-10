if(WIN32)
    add_compile_options($<$<CONFIG:Debug>:/Od>)
    add_compile_options($<$<CONFIG:Checked>:/O1>)
    add_compile_options($<$<CONFIG:Release>:/Ox>)
    add_compile_options($<$<CONFIG:RelWithDebInfo>:/O2>)
elseif(CLR_CMAKE_PLATFORM_UNIX)
    add_compile_options($<$<CONFIG:Debug>:-O0>)
    add_compile_options($<$<CONFIG:Checked>:-O2>)
    add_compile_options($<$<CONFIG:RelWithDebInfo>:-O2>)

    if (CLR_CMAKE_BUILD_COREBUNDLE)
        add_compile_options($<$<CONFIG:Release>:-Os>)
        add_compile_options(-fdata-sections -ffunction-sections)
        add_compile_options(-fno-unroll-loops)
        add_compile_options(-fno-asynchronous-unwind-tables)
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
    else()
        add_compile_options($<$<CONFIG:Release>:-O3>)
    endif()
endif()
