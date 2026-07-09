cmake_minimum_required(VERSION 3.15)
project(MyProtocolCore CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Настраиваем пути поиска для Apple/Homebrew
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
endif()

# 2. УНИВЕРСАЛЬНЫЙ ПОИСК LIBSODIUM (vcpkg + системный Mac/Linux)
find_package(unofficial-sodium CONFIG QUIET)

if(unofficial-sodium_FOUND)
    message(STATUS "Found libsodium via vcpkg")
    set(SODIUM_LINK_TARGET unofficial-sodium::sodium)
else()
    # Если vcpkg нет, ищем классическим ручным способом (для Mac/Linux)
    find_path(SODIUM_INCLUDE_DIR sodium.h)
    find_library(SODIUM_LIBRARY sodium)
    
    if(SODIUM_INCLUDE_DIR AND SODIUM_LIBRARY)
        message(STATUS "Found system libsodium: ${SODIUM_LIBRARY}")
        
        # Чтобы не разводить кашу в target_link_libraries, 
        # заворачиваем ручные пути в один красивый псевдо-таргет
        add_library(sodium_local INTERFACE)
        target_include_directories(sodium_local INTERFACE ${SODIUM_INCLUDE_DIR})
        target_link_libraries(sodium_local INTERFACE ${SODIUM_LIBRARY})
        
        set(SODIUM_LINK_TARGET sodium_local)
    else()
        message(FATAL_ERROR "libsodium NOT found! Please install it.")
    endif()
endif()

# 3. Ищем liboqs в системе
find_package(liboqs REQUIRED)

# 4. Собираем файлы в текущей папке
file(GLOB SOURCES CONFIGURE_DEPENDS "*.cpp" "*.hpp" "*.h")

# 5. СОЗДАЕМ ТАРГЕТ
add_executable(${PROJECT_NAME} ${SOURCES})

# 6. ПОДКЛЮЧАЕМ ВНУТРЕННИЕ ИНКЛЮДЫ
target_include_directories(${PROJECT_NAME} PRIVATE include)

# 7. ЛИНКУЕМ ВСЕ БИБЛИОТЕКИ К СОЗДАННОМУ ТАРГЕТУ
target_link_libraries(${PROJECT_NAME} PRIVATE
    OQS::oqs
    ${SODIUM_LINK_TARGET}  # Сюда автоматически подставится либо vcpkg, либо локальный путь
)

# Если мы на Windows — докидываем системные сокеты
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 wsock32)
endif()