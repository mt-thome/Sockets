# Versão mínima requerida do CMake
cmake_minimum_required(VERSION 3.10)

# Nome do projeto
project(
    sockets_client C
)
project(
    sockets_server C
)

# Pacotes utilizados (bibliotecas)
find_package()
# Adicionar todas as bibliotecas
#...

# Adicionando ao executável
add_executable(
    main_client.exe client.c
)
add_executable(
    main_server.exe server.c
)

# adicionando as bibliotecas a compilação
target_link_libraries(
    main_client.exe PRIVATE
    # ...
)
target_link_libraries(
    main_server.exe PRIVATE
    # ...
)