SYSTEM DIRECTIVE: DUBLEAR-STYLE EXPRESSIVITY & CORE ENHANCEMENTS

To: Antigravity (AI Coding Agent)
Context: We are upgrading voice2midi to match the expressive capabilities of Vochlea Dubler 2. This requires integrating real-time spectral analysis, high-speed transient detection, and an auto-calibration system.

TASK 1: Spectral Analyzer (Vowels to MIDI CC)

Class: SpectralAnalyzer

Implementation: Integrate juce::dsp::FFT.

Math: Compute the Spectral Centroid of each audio block using:


$$Centroid = \frac{\sum_{n=0}^{N/2-1} f(n) \cdot |X(n)|}{\sum_{n=0}^{N/2-1} |X(n)|}$$


Where $f(n)$ is the center frequency of bin $n$ and $|X(n)|$ is the magnitude.

Mapping: Map this value linearly to the expression_cc parameter.

Safety: Use a pre-allocated circular buffer and Hanning windowing to minimize spectral leakage. Ensure the FFT runs in a non-blocking context (use a dedicated FFT-buffer size, e.g., 512).

TASK 2: Onset Detector (Beatbox Percussion)

Class: OnsetDetector

Implementation: Operate in parallel to the PitchDetector (low-latency path).

Math: Use the Energy Derivative method:


$$E_{current} = \sum_{n=0}^{N-1} x^2(n)$$

$$E_{diff} = E_{current} - E_{prev}$$


Trigger a MIDI Note (36/38/42) if $E_{diff} > TransientThreshold$.

Band-Limiting: Apply a simple IIR high-pass filter before the derivative to focus on "click" transients (Snare/Hi-Hat) and low-pass for "thump" (Kick).

Output: Route events exclusively to MIDI Channel 10.

TASK 3: Intelligent Pitch Bend ("Intellibend")

Refactor: MidiEventGenerator

Logic: Implement "Stickiness". When the detected frequency is within $x$ cents of a chromatic note (user defined), dampen the Pitch Bend output, forcing the note center.

True Bend Mode: Implement an alternative mode that follows input pitch rawly for expressive glissandos.

Clamp: Ensure all outputs are clamped via juce::jlimit(0, 16383, Value) to prevent MIDI protocol errors.

TASK 4: Auto-Calibration Manager

Class: CalibrationManager

Logic: Create a "Listen" state.

Noise Floor Analysis: Record 2 seconds of silence to establish the background noise floor.

Envelope Thresholding: Calculate the RMS of user input during calibration and set gate_threshold to (RMS_Input - NoiseFloor) * 0.8.

Range Mapping: Detect the user's vocal range (Min/Max Hz) and automatically set APVTS parameters for min_frequency and max_frequency.

EXECUTION

Create Source/DSP/SpectralAnalyzer.h/cpp.

Create Source/DSP/OnsetDetector.h/cpp.

Create Source/DSP/CalibrationManager.h/cpp.

Integrate these into VoiceToMidiProcessor::processBlock ensuring NO locks and NO dynamic allocations.

Acknowledge these instructions and start by generating the SpectralAnalyzer header and implementation.

Resumen de Mejoras: Transformación a "Vocal Instrument"

El objetivo es pasar de un simple conversor de Voz -> Nota MIDI a un Instrumento Vocal Expresivo que compita en calidad y sensación con los estándares de la industria (como Dubler 2).

Aquí tienes el resumen de las 4 patas principales que vamos a implementar:

1. Motor de Expresividad (Timbre/Vocales)

Qué es: Mapear la forma de tu boca (tus vocales) a parámetros de control MIDI (MIDI CC).

Por qué: Dubler permite que, si cambias de decir "Ooo" a "Aaa", el filtro de tu sintetizador se abra o cierre. Esto da una expresividad orgánica.

Técnica: Calcularemos el Centroide Espectral ($Centroid$). Es el "centro de gravedad" de las frecuencias. Una "i" tiene energía en frecuencias altas (centroide alto), una "o" en bajas (centroide bajo).


$$Centroid = \frac{\sum f(n) \cdot |X(n)|}{\sum |X(n)|}$$

2. Motor de Percusión (Beatbox)

Qué es: Disparar sonidos de batería (Bombo, Caja, Charles) con golpes de aire o chasquidos.

Por qué: Permite crear ritmos sin usar las manos, solo con la boca.

Técnica: Detector de Transitorios (Energy Derivative). En lugar de analizar el tono (que es lento), analizaremos el "golpe" de energía instantáneo.


$$E_{diff} = \sum x^2(n)_{actual} - \sum x^2(n)_{anterior}$$


Si la diferencia supera un umbral, disparas el bombo al instante.

3. Inteligencia Operativa (Auto-Calibración)

Qué es: Un sistema que "aprende" tu micrófono y tu habitación en 5 segundos.

Por qué: No todos los micrófonos son iguales ni todas las habitaciones tienen el mismo ruido de fondo.

Técnica:

Análisis de ruido: Mide el silencio para quitar el "hiss" de fondo.

Umbral dinámico: Ajusta el nivel de activación (gate) para que no se disparen notas por respirar.

Mapeo de rango: Detecta qué tan grave o agudo puedes cantar para limitar las notas automáticamente.

4. Estabilidad Musical (Intellibend)

Qué es: Un "ancla" inteligente para tu afinación.

Por qué: Las voces humanas fluctúan naturalmente. Si el plugin es demasiado rígido, suena robótico; si es muy flexible, suena desafinado.

Técnica: Implementaremos un sistema de histéresis donde, si estás cerca de una nota correcta, el plugin te "ayuda" a mantenerla, pero si haces un movimiento brusco, te permite hacer un glissando (el bend) suavemente.