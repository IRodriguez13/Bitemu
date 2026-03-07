# Mejoras para el audio – Bitemu

Resumen del estado actual del audio y pasos concretos para conectarlo al botón del frontend y mejorar la implementación.

---

## Estado actual

| Capa | Qué hay | Qué falta |
|------|---------|-----------|
| **include/bitemu.h** | API completa: `bitemu_audio_init`, `set_enabled`, `set_callback`, `shutdown`; backends SDL2, miniaudio, null, callback | — |
| **be1/audio/audio.h** | Estructura `bitemu_audio_t` con buffer, SDL_mutex, SDL_AudioDeviceID, ma_device | Desacoplar del core: no debería depender de SDL/miniaudio en la librería |
| **be1/apu.c** | `gb_apu_step` (avanza duty/wave/LFSR); `apu_mix_sample(apu, audio)` que escribe en el buffer | 1) `apu_square1_output`, `apu_square2_output`, `apu_wave_output`, `apu_noise_output` y `apply_volume_and_pan` **no están definidas**. 2) `apu_mix_sample` usa `apu->GB_IO_NR50` y `apu->GB_IO_NR51` pero **gb_apu_t no tiene esos campos** (están en `mem->io`). 3) **Nadie llama** a `apu_mix_sample` (no se invoca desde `gb_step` ni desde el engine). |
| **src/bitemu.c** | Wrapper del engine, sin lógica de audio | **No implementa** `bitemu_audio_init`, `bitemu_audio_set_enabled`, `bitemu_audio_set_callback`, `bitemu_audio_shutdown`. |
| **Frontend (core.py)** | Solo API de emu, framebuffer, input | No expone `bitemu_read_audio` ni `bitemu_audio_*`. |
| **Frontend (main_window)** | Opción "Activar audio" guardada en QSettings | `_toggle_audio` no llama al core ni abre ningún stream. |

Conclusión: la API y el APU están empezados pero **incompletos y desconectados**. Para que el botón del frontend tenga efecto hay que cerrar estos huecos.

---

## Problemas concretos a corregir

### 1. APU – funciones sin implementar

En `apu_mix_sample` se usan:

- `apu_square1_output(apu)`, `apu_square2_output(apu)`, `apu_wave_output(apu)`, `apu_noise_output(apu)`  
  Deben devolver el valor analógico del canal (p. ej. 0–15 o -128–127) según duty, wave o LFSR y los registros NR1x/NR2x/NR3x/NR4x. Hoy **no existen** en el código.

- `apply_volume_and_pan(ch1, ch2, ch3, ch4, nr50, nr51, left_or_right)`  
  Debe mezclar los cuatro canales aplicando NR50 (volumen global L/R) y NR51 (panning por canal). **Tampoco está definida**.

- `apu_mix_sample` recibe solo `(apu, audio)` pero usa NR50/NR51. Esos registros están en **memoria**, no en `gb_apu_t`. Hay que pasar `struct gb_mem *mem` (o los valores ya leídos) a `apu_mix_sample`.

### 2. Muestreo en el step

El diseño (Documentation/audio-implementation.md) dice: a 44.1 kHz, una muestra cada ~95 ciclos (4194304 / 44100). Hoy:

- `gb_step` ejecuta hasta 70224 ciclos por frame pero **no** llama a ninguna función que muestree el APU ni escriba en un buffer.
- Hay que, cada ~95 ciclos (o por bloques), llamar a algo tipo `apu_mix_sample(apu, mem, audio)` y escribir una muestra en el buffer del core.

### 3. Core acoplado a SDL/miniaudio

`be1/audio/audio.h` usa `SDL_mutex`, `SDL_AudioDeviceID`, `ma_device`. Eso obliga a enlazar SDL2/miniaudio en la lib y complica el build y el frontend Python.

Recomendación (opción A del doc de audio):

- El **core** solo mantiene un **buffer circular** de muestras S16 y expone:
  - `bitemu_read_audio(emu, buf, max_samples)` → número de muestras leídas.
  - Opcional: `bitemu_get_audio_spec(&sample_rate, &channels)` (44100, 1 o 2).
- Sin mutex de SDL en el core: se puede usar un buffer lock-free (un solo productor en el step, un consumidor en el frontend) o un mutex genérico si se añade una capa mínima de threading en C.
- El **frontend Python** (con “Activar audio”) abre un stream (p. ej. sounddevice) a 44100 Hz y, después de cada `run_frame()`, llama a `bitemu_read_audio` y escribe las muestras en el stream.

Así la librería no depende de SDL ni miniaudio; el frontend sí puede usar sounddevice/PyAudio.

### 4. API pública sin implementar en src/bitemu.c

Las funciones declaradas en `bitemu.h` no tienen implementación en `src/bitemu.c`. Opciones:

- **Opción A (recomendada):** Implementar solo el modelo “buffer + pull”:
  - En `bitemu_t` (o en una estructura interna) un buffer circular de audio (p. ej. 4096 muestras S16), `write_pos`, `read_pos`.
  - `bitemu_audio_init(emu, backend, config)` → si backend es CALLBACK o NULL, solo inicializa el buffer y la espec (44100, 1 canal).
  - `bitemu_audio_set_enabled(emu, true/false)` → activa/desactiva que el step escriba en el buffer (o el frontend simplemente no llama a `read_audio` si está desactivado).
  - **Nueva:** `bitemu_read_audio(emu, int16_t *buf, int max_samples)` → lee del buffer circular y devuelve cuántas muestras se copiaron.
  - `bitemu_audio_set_callback` puede quedar para un driver en C que quiera recibir callbacks; para el frontend Python basta `read_audio` después de cada frame.
  - `bitemu_audio_shutdown` libera el buffer.

- **Opción B:** Mantener la API actual con backends SDL2/miniaudio en C. Entonces hay que implementar `bitemu_audio_init` etc. en `bitemu.c` y enlazar SDL2 o miniaudio en el Makefile; el frontend solo llamaría `bitemu_audio_set_enabled`. Es más trabajo y añade dependencias en C.

### 5. Frontend desconectado

- **core.py:** Añadir wrappers ctypes para:
  - `bitemu_get_audio_spec(emu, c_int_p, c_int_p)` o constantes 44100, 1.
  - `bitemu_read_audio(emu, buffer_ptr, max_samples)` → devuelve int.
  - Si se usa la API actual: `bitemu_audio_init`, `bitemu_audio_set_enabled`, `bitemu_audio_set_callback` (si se elige callback en lugar de pull).
- **game_widget.py (o main_window):** Después de `run_frame()`, si “Activar audio” está activo, llamar `bitemu_read_audio`, convertir a bytes/numpy y escribir en un stream de sounddevice/PyAudio. Gestionar inicio/cierre del stream al activar/desactivar la opción.
- **main_window._toggle_audio:** Además de guardar en QSettings, llamar al core (`bitemu_audio_set_enabled`) y arrancar/parar el stream de audio en Python.

---

## Orden sugerido de implementación

1. **Corregir APU (be1/apu.c)**  
   - Implementar `apu_square1_output`, `apu_square2_output`, `apu_wave_output`, `apu_noise_output` (leyendo duty/wave/LFSR y NR1x–NR4x desde `mem` si hace falta pasarlo).  
   - Implementar `apply_volume_and_pan` usando NR50/NR51.  
   - Cambiar la firma de `apu_mix_sample` a `apu_mix_sample(gb_apu_t *apu, gb_mem_t *mem, bitemu_audio_t *audio)` y usar `mem->io[GB_IO_NR50]` y `mem->io[GB_IO_NR51]`.

2. **Desacoplar audio del core (opción A)**  
   - Sustituir o simplificar `be1/audio/audio.h`: solo buffer circular (int16_t *buffer, size_t size, read_pos, write_pos), sin SDL ni miniaudio. Si se necesita exclusión, usar un mutex genérico (pthread) o un buffer lock-free de un productor / un consumidor.  
   - En `apu_mix_sample` escribir en ese buffer (o en una estructura que el engine rellene); no llamar a SDL_LockMutex.

3. **Integrar muestreo en el step**  
   - En `gb_step` (o en el engine), cada `GB_CPU_HZ / 44100` ciclos (~95) llamar a `apu_mix_sample(apu, mem, &audio_state)`.  
   - El `audio_state` (buffer circular) debe vivir en `bitemu_t` o en el engine y ser inicializado en `bitemu_audio_init` (o en `bitemu_create` si siempre se crea el buffer).

4. **Implementar API en src/bitemu.c**  
   - Añadir el buffer de audio en `struct bitemu` (o en una subestructura).  
   - Implementar `bitemu_audio_init`, `bitemu_audio_set_enabled`, `bitemu_read_audio`, `bitemu_audio_shutdown`.  
   - Declarar `bitemu_read_audio` y, si aplica, `bitemu_get_audio_spec` en `include/bitemu.h`.

5. **Frontend Python**  
   - Añadir `sounddevice` (o PyAudio) a `requirements.txt` (o extra opcional `[audio]`).  
   - En `core.py`: `get_audio_spec()`, `read_audio(buf, max_samples)`.  
   - En el tick del juego: si audio está activo, después de `run_frame()` llamar `read_audio` y enviar las muestras al stream.  
   - En `_toggle_audio`: guardar preferencia, llamar `bitemu_audio_set_enabled` y abrir/cerrar el stream en Python.

6. **Documentación**  
   - Actualizar `Documentation/audio-implementation.md` con el estado real y referencias a este documento.  
   - README: sección “Audio” con dependencia opcional (sounddevice) e instrucciones.

---

## Checklist rápido para “conectar el botón”

- [ ] APU: implementar salidas por canal y `apply_volume_and_pan`; corregir `apu_mix_sample` para usar `mem` en NR50/NR51.  
- [ ] Core: buffer circular de audio sin SDL; muestreo cada ~95 ciclos en `gb_step`.  
- [ ] `bitemu.c`: implementar init/set_enabled/read_audio/shutdown (y get_audio_spec si se usa).  
- [ ] `bitemu.h`: declarar `bitemu_read_audio` (y opcional `bitemu_get_audio_spec`).  
- [ ] core.py: bindings para read_audio y set_enabled (y get_audio_spec si aplica).  
- [ ] frontend: stream sounddevice; después de cada frame leer audio y escribir en el stream; _toggle_audio enciende/apaga stream y set_enabled.  
- [ ] Probar con una ROM que tenga sonido (p. ej. pantalla de título de un juego conocido) y comprobar que el botón “Activar audio” realmente activa/desactiva la salida.

Con esto se puede tener una primera versión funcional del audio conectada al botón del frontend y, a partir de ahí, afinar latencia, tamaño del buffer y calidad del mix.
