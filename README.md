# MiniDayZ-PSP

Recreación nativa en C/C++ para PSP, inspirada en [Mini DayZ Plus](https://github.com/NextDev65/MiniDayZ) (juego original hecho con Construct 2).

## Por qué desde cero

El original está exportado con Construct 2: la lógica vive serializada en un
blob de datos (data.js, 8.3 MB) que solo el runtime genérico de Construct 2
sabe interpretar. No hay código fuente legible para traducir, así que este
proyecto recrea las mecánicas jugando el original como referencia,
implementadas desde cero en C/C++ con SDL2 (toolchain pspdev).

## Hardware objetivo

- PSP-3001 (Slim), firmware 6.20 PRO-B5 (CFW)

## Estado

- [ ] Toolchain instalado y probado
- [ ] "Hello World" corriendo en PSP física
- [ ] Loop de juego básico (movimiento, mapa, cámara)
- [ ] Zombis + combate
- [ ] Inventario / items
- [ ] Crafteo

## Estructura

- `src/` — código fuente C/C++
- `assets/` — sprites y audio preparados para PSP
- `design/` — notas propias sobre las mecánicas del original
- `build/` — salida de compilación (ignorado)

## Compilar