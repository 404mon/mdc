cmake_minimum_required(VERSION 3.10)
project(MDC VERSION 0.1.0 LANGUAGES C)

# Impostiamo lo standard C (C11 è un ottimo compromesso tra modernità e compatibilità)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Definiamo dove sono gli header (file .h)
include_directories(include)

# Cerchiamo tutti i file .c nella cartella src
file(GLOB SOURCES "src/*.c")

# Creiamo l'eseguibile
add_executable(MDC ${SOURCES})

# Messaggio di conferma a fine configurazione
message(STATUS "Configurazione MDC completata per la piattaforma: ${CMAKE_SYSTEM_NAME}")