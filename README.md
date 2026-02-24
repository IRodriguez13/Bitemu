# Bitemu

Motor de emulación genérico con interfaz reutilizable por consola.

## Estructura

- **core/** — Motor genérico, independiente de consola. Define la interfaz en `core/console.h`.
- **BE1/** — Game Boy (este repo).
- **BE2**, **BE3** — Genesis, PS1 en repos separados (mover `consoles/genesis`, `consoles/ps1` a mano).

## Build

```bash
make
./bitemu
```
