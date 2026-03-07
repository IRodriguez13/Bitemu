# Implementación de audio - Bitemu (Game Boy)

Propuesta para añadir salida de audio al emulador, de forma que el usuario pueda activarla o desactivarla desde Opciones y el core genere muestras que el frontend (o un driver en C) reproduzca.

## Implementado (arquitectura universal sin SDL)

- **Core:** Buffer circular en `be1/audio/audio.h` (sin SDL ni miniaudio). Muestreo cada `GB_CYCLES_PER_SAMPLE` (~95 ciclos) en `gb_step`; `apu_mix_sample(apu, mem, audio)` escribe en el buffer. Salidas por canal y mezcla son stubs hasta implementar las funciones reales.
- **API:** `bitemu_audio_init`, `bitemu_audio_set_enabled`, `bitemu_read_audio`, `bitemu_get_audio_spec`, `bitemu_audio_shutdown`. El frontend usa `read_audio()` después de cada frame.
- **Frontend:** Opciones → Activar audio abre `sounddevice.RawOutputStream` a 44100 Hz mono S16; tras cada frame se llama a `read_audio` y se escribe en el stream. Sin dependencias de audio en la librería C; igual en Windows, macOS y Linux. Ver `Documentation/audio-improvements.md` para mejoras pendientes.

---

## Estado actual

- **APU (be1/apu.c):** actualiza estado interno (duty, wave, LFSR) según los registros NR10–NR52 y los ciclos. **No** genera muestras PCM ni las escribe en ningún buffer.
- **API (include/bitemu.h):** no hay funciones de audio; solo framebuffer e input.
- **Frontend:** menú Opciones → “Activar audio” (guardado en QSettings); por ahora no hace nada.

## Objetivo

1. Que el **core** produzca un **buffer de muestras** por frame (o por bloque de N muestras) en formato estándar (mono o estéreo, 44.1 kHz o 48 kHz, S16 o float).
2. Que el **frontend** (o un módulo en C) envíe ese buffer al sistema de audio cuando “Activar audio” esté activo.
3. Mantener el core **desacoplado** del sistema de sonido (sin SDL/miniaudio obligatorios en el core); la salida puede ser vía callback o buffer que el frontend lee.

## Opciones de arquitectura

### A) Muestras en el core, reproducción en el frontend (recomendada)

- El core (APU + nueva capa “audio out”) escribe en un **buffer circular** o en un **callback** que el frontend registra.
- La API pública añade algo como:
  - `bitemu_set_audio_callback(emu, callback, userdata)` y/o
  - `bitemu_get_audio_buffer(emu, *buf, *samples)` para leer las últimas N muestras después de cada frame.
- El **frontend Python** (cuando “Activar audio” está activo):
  - Después de cada `bitemu_run_frame()` lee el buffer de audio (o recibe un callback).
  - Envía las muestras al sistema con **PyAudio**, **sounddevice** o **SDL2** (vía binding Python). Así no añadimos dependencias en C para audio; todo el playback está en Python.

**Ventajas:** Core portable; el frontend ya tiene la opción “Activar audio” y solo hay que conectar; fácil probar en Linux/macOS/Windows con la misma API.  
**Desventaja:** Pequeña latencia y posible uso de un hilo en Python para el stream de audio.

### B) Salida de audio en C (SDL2 o miniaudio)

- El core o un módulo `audio.c` enlazado con **SDL2** (o **miniaudio**) abre un stream de audio y, en un callback de SDL/miniaudio, pide al APU las muestras y las escribe.
- La API podría exponer `bitemu_set_audio_enabled(emu, true/false)` y el core internamente abre/cierra el dispositivo.
- El frontend solo activa/desactiva el flag; no reproduce audio por su cuenta.

**Ventajas:** Menor latencia y control fino en C.  
**Desventajas:** Dependencia de SDL2 o miniaudio en el build; hay que compilar/enlazar en Windows/macOS/Linux; el Makefile y el CI deben instalar esas libs.

### C) Híbrido: core genera buffer, driver opcional en C

- El core genera siempre un buffer de muestras (por frame o por tiempo real) y lo deja en una estructura accesible (por ejemplo `bitemu_get_audio_buffer()`).
- Por defecto **no** se enlaza ningún driver; el frontend Python usa ese buffer y lo reproduce con sounddevice/PyAudio.
- Opcionalmente se puede compilar un driver en C (SDL2) que lea el mismo buffer y lo envíe al sistema, para quien quiera un binario sin Python en el camino del audio.

Para una **primera release** con audio, **A** es la opción más simple y alineada con el frontend actual.

---

## Diseño recomendado (opción A)

### 1. Core (C)

- **Frecuencia de muestreo:** 44.100 Hz (estándar GB) o 48 kHz. El GB APU corre a 4194304 Hz; hay que muestrear a una tasa fija (44.1k) y convertir “por tiempo” (cada cuántos ciclos de CPU corresponde una muestra).
- **Formato:** mono o estéreo (el GB es mono; NR50/NR51 mezclan a izquierda/derecha; se puede exportar estéreo con panning o mono).
- **Buffer:** en `bitemu_t` (o en la estructura interna del engine) un buffer circular de, por ejemplo, 4096 muestras S16. Después de cada `bitemu_run_frame()` (o durante el frame, muestreando a 44.1k), el APU escribe en ese buffer.
- **Nueva función en bitemu.h:**
  - `void bitemu_get_audio_spec(int *sample_rate, int *channels, int *format);` — solo lectura; dice 44100, 1 (o 2), S16.
  - `int bitemu_read_audio(bitemu_t *emu, int16_t *buf, int max_samples);` — devuelve el número de muestras leídas (y las copia a `buf`); el core puede implementarlo vaciando su buffer circular. Si no hay audio o está desactivado, devuelve 0.

### 2. APU: de “solo estado” a “muestras”

- Hoy el APU solo avanza contadores (duty, wave, LFSR). Hay que **muestrear** la salida analógica de cada canal (Square1, Square2, Wave, Noise), mezclarlos con NR50/NR51 (volumen y panning) y escribir muestras S16 en el buffer.
- **Ciclos por muestra:** a 44.1 kHz, una muestra cada `4194304 / 44100 ≈ 95.1` ciclos. Cada N ciclos de CPU se llama a una función “apu_mix_sample()” que:
  - Obtiene el nivel actual de cada canal (según duty/wave/LFSR y volumen).
  - Aplica NR50 (volumen global) y NR51 (panning).
  - Escribe una o dos muestras (L/R) en el buffer circular.
- Documentar en este mismo doc o en `Documentation/` la fórmula de mezcla y el mapeo de NR50/NR51 para que quien implemente el mix tenga la referencia.

### 3. Frontend (Python)

- Si **Opciones → Activar audio** está desactivado, no se llama a `bitemu_read_audio` ni se abre ningún stream.
- Si está activado:
  - Al iniciar (o al activar la opción), abrir un stream de audio (sounddevice o PyAudio) a 44100 Hz, mono o estéreo, S16.
  - En el mismo bucle donde se llama `bitemu_run_frame()` y se pinta el framebuffer, después del frame llamar a `bitemu_read_audio(emu, buf, N)` y enviar las muestras al stream. Si el stream usa callbacks, se puede llenar una cola desde el main loop y el callback del stream lee de esa cola.
- Dependencia opcional: `sounddevice` o `PyAudio` en `requirements.txt` (o en `requirements-audio.txt`) para no romper instalaciones mínimas.

### 4. Documentación

- En **README** o en **Documentation/audio-implementation.md**: resumir que el audio es opcional, que el core genera muestras y el frontend las reproduce cuando “Activar audio” está activo, y qué dependencias extra se necesitan (p. ej. `pip install sounddevice`).
- En **DESIGN.md** del frontend: actualizar la sección de Opciones indicando que “Activar audio” enciende la lectura del buffer y el envío al sistema de audio.

## Pasos concretos para implementar (checklist)

1. **Core**
   - [ ] Definir en `bitemu.h` (o en un `audio.h` interno) la frecuencia (44100), canales (1) y formato (S16).
   - [ ] Añadir buffer circular de audio en la estructura interna del emulador (tamaño ~4096–8192 muestras).
   - [ ] En el step del engine (dentro de `console_step` o en un wrapper), cada X ciclos llamar a una función que muestree el APU y escriba una muestra en el buffer (apu_mix_one_sample).
   - [ ] Implementar en APU la generación de valor analógico por canal (duty, wave, noise) y la mezcla con NR50/NR51.
   - [ ] Exponer `bitemu_read_audio(emu, buf, max_samples)` que lee del buffer circular y devuelve el número de muestras.
   - [ ] (Opcional) `bitemu_set_audio_enabled(emu, bool)` para que el core no escriba en el buffer si está desactivado (o el frontend simplemente no llama a `read_audio`).

2. **Frontend**
   - [ ] Añadir dependencia `sounddevice` (o PyAudio) en `requirements.txt` o en un extra `[audio]`.
   - [ ] Al iniciar la ventana, si “Activar audio” está activo, abrir stream 44100 Hz, 1 canal, S16.
   - [ ] En el timer/tick del juego, después de `run_frame()`, llamar a `bitemu_read_audio`, convertir a formato numpy/bytes si hace falta y escribir en el stream.
   - [ ] Al cerrar o al desactivar audio, cerrar el stream.
   - [ ] Manejar errores (dispositivo no disponible, etc.) con un mensaje en la barra de estado o un diálogo.

3. **Docs**
   - [ ] Actualizar README: sección “Audio” con requisitos (sounddevice) y que es opcional.
   - [ ] Este documento (o un resumen en Documentation/) como referencia para mantenimiento y posibles mejoras (latencia, tamaño de buffer, etc.).

## Referencias

- Pan Docs: Audio (NR10–NR52, canales, frecuencias).
- Documentación existente del APU en `Documentation/05-subsystems/05-apu.md` (si existe).
- Game Boy CPU runs at 4194304 Hz; sample rate 44100 Hz implica ~95 ciclos por muestra.
