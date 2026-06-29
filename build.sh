#!/bin/bash
# Script de compilación automática para Voice2MIDI

# Detener el script si ocurre algún error
set -e

echo "============================================="
echo "   Iniciando compilación de Voice2MIDI"
echo "============================================="

# 1. Crear y configurar la carpeta de compilación
echo "-> Configurando archivos del proyecto con CMake..."
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 2. Compilar el proyecto en modo Release usando todos los núcleos de CPU (compila App y Plugins)
echo "-> Compilando ejecutables..."
cmake --build build --config Release -j$(sysctl -n hw.logicalcpu)

echo ""
echo "============================================="
echo "   ¡Compilación completada con éxito!"
echo "============================================="
echo "Los artefactos generados se encuentran en:"
echo "📂 App Standalone:  build/voice2midi_artefacts/Release/Standalone/voice2midi.app"
echo "📂 Plugin VST3:     build/voice2midi_artefacts/Release/VST3/voice2midi.vst3"
echo "📂 Plugin AU (Mac): build/voice2midi_artefacts/Release/AU/voice2midi.component"
echo "============================================="
