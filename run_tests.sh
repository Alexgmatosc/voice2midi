#!/bin/bash
# Script de compilación y ejecución de pruebas unitarias para Voice2MIDI

# Detener el script si ocurre algún error
set -e

echo "============================================="
echo "   Configurando y Ejecutando Tests"
echo "============================================="

# 1. Configurar la carpeta de compilación activando el flag BUILD_TESTS
echo "-> Configurando proyecto CMake (Tests Habilitados)..."
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON

# 2. Compilar específicamente el target de tests usando todos los núcleos de CPU
echo "-> Compilando el ejecutable de pruebas..."
cmake --build build --config Release --target voice2midi_tests -j$(sysctl -n hw.logicalcpu)

# 3. Ejecutar las pruebas unitarias automáticas
echo ""
echo "-> Ejecutando suite de pruebas unitarias..."
echo "============================================="
./build/voice2midi_tests_artefacts/Release/voice2midi_tests
echo "============================================="
echo "   ¡Pruebas ejecutadas con éxito!"
echo "============================================="
