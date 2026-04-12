cmake_minimum_required(VERSION 3.10)
project(MDC VERSION 0.1.0 LANGUAGES C)

# Genera il file per l'IntelliSense di VS Code / clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Standard C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 1. Trova le librerie necessarie tramite PkgConfig
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)
find_package(OpenGL REQUIRED)
pkg_check_modules(GLEW REQUIRED glew)

# 2. Configura le directory degli Header
# Includiamo 'include' (i tuoi .h), 'deps' (nuklear) e quelle di sistema
include_directories(
    include 
    deps 
    ${GLFW_INCLUDE_DIRS} 
    ${OPENGL_INCLUDE_DIR} 
    ${GLEW_INCLUDE_DIRS}
)

set(SOURCES 
    src/main.c 
    src/nuklear_impl.c 
    src/md4c.c   # <--- AGGIUNGILO ESPLICITAMENTE SE IL GLOB FALLISCE
)

# 4. Crea l'eseguibile
add_executable(MDC ${SOURCES})

# 5. Linka le librerie all'eseguibile
# Aggiungiamo 'm' per la libreria matematica (math.h), richiesta da Nuklear
target_link_libraries(MDC 
    ${GLFW_LIBRARIES} 
    ${OPENGL_LIBRARIES} 
    ${GLEW_LIBRARIES} 
    m 
    dl
)

# Messaggio di stato
message(STATUS "Configurazione MDC completata.")
message(STATUS "Piattaforma: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Librerie linkate: GLFW, OpenGL, GLEW")