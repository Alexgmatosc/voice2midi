# Voice2MIDI

Voice2MIDI es un conversor en tiempo real de voz a MIDI de baja latencia. Captura el audio monofónico de tu micrófono, analiza la frecuencia y la amplitud de tu voz mediante el algoritmo YIN y genera eventos MIDI (Note On, Note Off, Velocity y Pitch Bend) para tocar sintetizadores o instrumentos virtuales en tiempo real.

---

## 1. Guía Rápida de Ajuste de Parámetros

La interfaz genérica muestra 5 deslizadores (*sliders*). Ajustarlos correctamente es la diferencia entre una detección limpia y un caos de notas falsas:

### 1. Input Gain (Ganancia de Entrada)
* **Rango:** `-20 dB` a `+20 dB` (Por defecto: `0 dB`)
* **Qué hace:** Sube o baja el volumen del micrófono *antes* de que pase por el detector.
* **Cómo usarlo:** Si tu micrófono de por sí graba muy bajo, súbelo. En el monitor de depuración visual de la parte superior, busca que tu voz al hablar de forma normal marque entre `-15 dB` y `-5 dB`. Evita subirlo tanto que marque `0 dB` (distorsión/clipping).

### 2. Gate Threshold (Umbral de Puerta de Ruido)
* **Rango:** `-60 dB` a `-5 dB` (Por defecto: `-30 dB`)
* **Qué hace:** Es el filtro que separa el ruido de la habitación de tu canto. Si el volumen detectado es menor que este umbral, el programa no enviará ninguna nota MIDI.
* **Cómo usarlo:** 
  1. Quédate en absoluto silencio y mira el monitor visual de nivel de entrada (ej: marcará `-45 dB`).
  2. Ajusta el deslizador de **Gate Threshold** ligeramente por encima de ese número (ej: `-35 dB`).
  3. Ahora, cuando estés en silencio, la barra superior estará en **Cian** (puerta cerrada). Cuando cantes, superará el umbral y cambiará a **Verde** (puerta abierta, enviando notas).

### 3. Pitch Bend Range (Rango de Curvatura de Tono)
* **Opciones:** `1`, `2`, `12` o `24` semitonos (Por defecto: `2`)
* **Qué hace:** Define la sensibilidad al deslizar la voz. Si cantas una nota y haces un *portamento* (deslizamiento) o un vibrato suave, la app no disparará notas nuevas. En su lugar, envía comandos de *Pitch Bend* (curvatura de tono de 14 bits) para que el sintetizador imite tu deslizamiento.
* **Cómo usarlo:** **¡MUY IMPORTANTE!** El valor que elijas aquí debe coincidir exactamente con el rango de Pitch Bend configurado en tu sintetizador en GarageBand/DAW. Si en Voice2MIDI pones `2` y tu sintetizador en GarageBand está configurado en `2`, tu deslizamiento de voz sonará perfectamente afinado. Si no coinciden, sonará desafinado.

### 4. Min Frequency (Frecuencia Mínima)
* **Rango:** `20 Hz` a `500 Hz` (Por defecto: `80 Hz`)
* **Qué hace:** El límite más bajo donde el algoritmo buscará notas.
* **Cómo usarlo:** Limita las frecuencias graves para evitar que ruidos como golpes en el soporte del micrófono, viento o ruidos de baja frecuencia se interpreten como notas MIDI súper graves. Si eres tenor o soprano, puedes subir este valor con seguridad (ej: `100 Hz` o `120 Hz`). Esto además reduce el consumo de CPU.

### 5. Max Frequency (Frecuencia Máxima)
* **Rango:** `400 Hz` a `4000 Hz` (Por defecto: `1000 Hz`)
* **Qué hace:** El límite más alto donde el algoritmo buscará notas.
* **Cómo usarlo:** Limita las frecuencias agudas para evitar que los siseos de la boca (sonidos como "S", "T"), chasquidos o respiraciones fuertes se interpreten como notas MIDI súper agudas. Para la mayoría de voces humanas, `1000 Hz` o `1500 Hz` es más que suficiente (la nota Do más alta de una soprano ronda los 1046 Hz). Acotar esto ahorra mucha CPU.

---

## 2. ¿Cómo probarlo con GarageBand? (Mac)

Debido a que GarageBand no permite enrutar MIDI entre plugins, utilizaremos la aplicación **Standalone** junto con el cable virtual nativo de Mac:

### Configurar el cable MIDI virtual en tu Mac:
1. Abre la aplicación **Configuración de Audio MIDI** (búscala con Spotlight `Cmd + Espacio`).
2. En el menú superior, haz clic en **Ventana** -> **Mostrar estudio MIDI**.
3. Haz doble clic en el dispositivo rojo llamado **Controlador IAC** (IAC Driver).
4. Marca la casilla **"El dispositivo está conectado"** y cierra la ventana.

### Conectar los programas:
1. Abre `voice2midi.app`.
2. Ve a **Options** -> **Audio/MIDI Settings**:
   - **Input:** Selecciona tu micrófono (ej. `USB Audio CODEC`).
   - **MIDI Output:** Selecciona **"Bus del controlador IAC"** (o **"GarageBand virtual de entrada"** si ya abriste GarageBand).
3. Abre **GarageBand**, crea una pista de **Instrumento de Software** (un piano, sintetizador, etc.) y selecciónala.
4. Canta. El sintetizador de GarageBand sonará respondiendo a tu voz. 

> [!TIP]
> **Consejo para evitar latencia:** En la configuración de audio de Voice2MIDI, mantén el **Audio buffer size** lo más bajo posible (ej: `128 samples` o `256 samples`) para que el tiempo entre tu canto y el sonido del sintetizador sea imperceptible. Si escuchas chasquidos en el audio, sube el buffer un escalón.
