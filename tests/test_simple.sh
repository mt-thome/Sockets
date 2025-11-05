#!/bin/bash

# Script de teste simples para servidor e cliente

echo "=== Teste do Sistema Cliente-Servidor ==="
echo ""
echo "Este teste vai:"
echo "1. Iniciar o servidor"
echo "2. Iniciar 1 cliente"
echo "3. Processar 1 bloco de 500x500"
echo ""
echo "Pressione CTRL+C para parar o teste"
echo ""

# Iniciar servidor em background
cd /home/mt-thome/Projetos/C/Sockets

# Input para o servidor:
# - num_clients: 1
# - num_blocks: 1  
# - num_lines: 500
# - num_columns: 500
echo "Iniciando servidor..."
(echo "1"; echo "1"; echo "500"; echo "500") | ./bin/server &
SERVER_PID=$!

# Aguardar servidor inicializar
sleep 2

echo "Iniciando cliente..."
./bin/client &
CLIENT_PID=$!

# Aguardar processamento
echo "Aguardando processamento..."
wait $SERVER_PID
wait $CLIENT_PID

echo ""
echo "=== Teste Concluído ==="
