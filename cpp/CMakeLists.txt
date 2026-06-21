cmake_minimum_required(VERSION 3.15)
project(MyProtocolCore CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Настраиваем пути поиска для Apple/Homebrew
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
endif()

# 2. Ищем libsodium в системе
find_path(SODIUM_INCLUDE_DIR sodium.h)
find_library(SODIUM_LIBRARY sodium)

if(SODIUM_INCLUDE_DIR AND SODIUM_LIBRARY)
    message(STATUS "Found libsodium: ${SODIUM_LIBRARY}")
else()
    message(FATAL_ERROR "libsodium NOT found! Please install it.")
endif()

# 3. Ищем liboqs в системе
find_package(liboqs REQUIRED)

# 4. Собираем файлы в текущей папке
file(GLOB SOURCES CONFIGURE_DEPENDS "*.cpp" "*.hpp" "*.h")

# 5. СОЗДАЕМ ТАРГЕТ (ОБЯЗАТЕЛЬНО ПЕРВЫМ!)
add_executable(${PROJECT_NAME} ${SOURCES})

# 6. ПОДКЛЮЧАЕМ ИНКЛЮДЫ К ТАРГЕТУ
# Вместо глобального include_directories используем target_include_directories.
# Это изолирует пути внутри твоего проекта.
target_include_directories(${PROJECT_NAME} PRIVATE
    include
    ${SODIUM_INCLUDE_DIR}
)

# 7. ЛИНКУЕМ ВСЕ БИБЛИОТЕКИ К СОЗДАННОМУ ТАРГЕТУ
# Здесь объединяем линковку liboqs и libsodium в один красивый блок
target_link_libraries(${PROJECT_NAME} PRIVATE
    OQS::oqs
    ${SODIUM_LIBRARY}
)

# Если мы на Windows — докидываем системные сокеты
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 wsock32)
endif()