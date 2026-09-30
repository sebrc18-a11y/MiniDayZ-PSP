# MiniDayZ-PSP — Documento Maestro
## Estructura de Integración: SRS + Arquitectura de Software + Kanban

| Campo | Valor |
|---|---|
| **Proyecto** | MiniDayZ-PSP |
| **Repositorio** | `MiniDayZ-PSP` (GitHub) |
| **Versión del documento** | 1.1 — se agregó §2.4 (estructura de carpetas real) y KAN-22 (reorganización del repositorio) |
| **Fecha** | 29 de septiembre de 2026 |
| **Estado** | Documento vivo — se actualiza en cada fase, no es estático |
| **Referencia de diseño** | [Mini DayZ Plus](https://github.com/NextDev65/MiniDayZ) (juego original, Construct 2) — solo como fuente de assets y referencia jugable, **no** como código fuente (ver §1.1) |

> **Cómo usar este documento:** es la fuente única de verdad del proyecto. Antes de iniciar una tarea nueva, se verifica contra este documento; al cerrar una tarea, este documento se actualiza (estado del requerimiento, tablero Kanban, cronograma). Ningún cambio de alcance se considera "real" hasta que está reflejado aquí.

---

## 1. Introducción y Requerimientos (Enfoque SRS)

### 1.1 Resumen ejecutivo

MiniDayZ-PSP es una **recreación nativa en C/C++** del juego *Mini DayZ Plus* para PlayStation Portable física (hardware real, no solo emulador), construida sobre **SDL2** y el toolchain homebrew **pspdev**.

Un punto de diseño fundamental, verificado técnicamente antes de escribir la primera línea de motor: el juego original está exportado con **Construct 2**. Su lógica de eventos vive serializada dentro de `data.js` (8.3 MB), interpretada en tiempo de ejecución por un runtime genérico minificado (`c2runtime.js`). No existe código fuente legible que traducir — la propia comunidad de Construct confirma que ese formato no es viable de aplicar ingeniería inversa. **Consecuencia directa para el alcance de este proyecto:** MiniDayZ-PSP no es un *port* de código; es una reconstrucción de las mecánicas observadas jugando el original, implementada desde cero, usando únicamente los **assets visuales y de audio** del repositorio original (sprites `.png`, efectos `.ogg`) como recursos legítimos de contenido.

El desarrollo sigue un enfoque de **slice vertical incremental**: cada mecánica se construye y valida como una prueba aislada y compilada de verdad (no solo revisada) antes de integrarse al cuerpo principal del juego. Este documento formaliza esas pruebas ya completadas y define el camino restante.

**Hardware y entorno objetivo:**
- Consola: PSP-3001 (Slim), firmware 6.20 PRO-B5 (CFW)
- Emulador de iteración rápida: PPSSPP
- Entorno de desarrollo: Windows + VS Code, toolchain dentro de WSL2/Ubuntu (`psp-gcc` 15.2.0, MIPS/Allegrex)

### 1.2 Requerimientos Funcionales

Cada requerimiento tiene un identificador único (`RF-xx`) para trazabilidad con la arquitectura (§2) y el tablero Kanban (§4). El estado refleja la realidad actual del proyecto, no una aspiración.

| ID | Requerimiento | Estado | Validado en |
|---|---|---|---|
| RF-01 | El jugador se mueve en 4 direcciones y diagonales, por cruceta (D-pad) y stick analógico | ✅ Validado | `graphics_test`, `sprite_test` |
| RF-02 | El personaje anima correctamente según dirección y si está en movimiento o quieto (4 poses estáticas + 3 ciclos de caminar, con volteo horizontal para el lado sin sprite propio) | ✅ Validado | `sprite_test` |
| RF-03 | Una cámara sigue al jugador sobre un mundo mayor que la pantalla, sin salir de los límites del mapa | ✅ Validado | `camera_test` |
| RF-04 | El jugador colisiona con obstáculos sólidos del mapa; puede deslizarse contra una pared en vez de trabarse | ✅ Validado | `collision_test` |
| RF-05 | Existen objetos recolectables en el mundo; al tocarlos, se recogen y se refleja en un contador en pantalla | ✅ Validado | `item_test` |
| RF-06 | Los zombis detectan al jugador dentro de un radio y lo persiguen; fuera de ese radio, permanecen quietos | ✅ Validado | `zombie_test` |
| RF-07 | El jugador tiene una barra de vida; el contacto con un zombi aplica daño con un enfriamiento (no cada frame); al llegar a 0, el juego entra en estado de "muerto" | ✅ Validado | `health_test` |
| RF-08 | Existe un modo de combate **cuerpo a cuerpo automático**: sin pulsar ningún botón, golpea al zombi vivo más cercano dentro de rango | ✅ Validado | `combat_test`, `weapons_test` |
| RF-09 | Existe un modo de combate **a distancia manual** (pistola): dispara una bala en la dirección exacta de puntería (no solo 4 direcciones), con munición finita | ✅ Validado | `weapons_test` |
| RF-10 | El jugador puede alternar entre las armas disponibles con un botón dedicado | ✅ Validado | `weapons_test` |
| RF-11 | Eventos de juego (paso, disparo, impacto de zombi, recolección) producen retroalimentación sonora usando audio real del juego original | ✅ Validado | `audio_test` |
| RF-12 | Tras la muerte del jugador, es posible reiniciar la partida completa (posición, vida, items, zombis, munición) | ✅ Validado | `combat_test` |
| RF-13 | Cada arma tiene un **rango de disparo fijo** propio (distinto de `MELEE_RANGE` genérico usado en las pruebas) | 🔲 Pendiente | — |
| RF-14 | Sistema de mira/fijado automático: el arma detecta al zombi más cercano y comienza a "apuntarle"; un indicador visual pasa de rojo a verde según la estabilidad del fijado, y a mayor estabilidad, mayor probabilidad de acierto y daño | 🔲 Pendiente — **diseño no confirmado del todo por el usuario, requiere aclaración antes de implementar** (ver nota en §3) | — |
| RF-15 | Estadísticas de supervivencia adicionales: hambre y sed, con degradación en el tiempo | 🔲 Pendiente | — |
| RF-16 | Sistema de inventario real (más allá del contador de `item_test`): slots, tipos de item, uso/equipar desde el inventario | 🔲 Pendiente | — |
| RF-17 | Sistema de crafteo de items/armas | 🔲 Pendiente | — |
| RF-18 | Ciclo día/noche | 🔲 Pendiente | — |
| RF-19 | Tipos adicionales de zombi con comportamiento propio (se identificaron assets de `zed_tank`, `zed_screamer`, `zed_jumper`, `zed_shooter` en el repositorio original) | 🔲 Pendiente | — |
| RF-20 | Mapa de juego real y definitivo (los tiles de `camera_test`/`collision_test` son placeholders de color sólido generados por código, no arte final) | 🔲 Pendiente — decisión de diseño abierta sobre origen del arte del mapa | — |
| RF-21 | Guardado y carga de partida en memoria stick | 🔲 Pendiente | — |
| RF-22 | Menús de inicio, pausa y pantalla de muerte | 🔲 Pendiente | — |
| RF-23 | Construcción de base/refugio (se identificaron assets `tilemap_bunker` en el repositorio original) | 🔲 Pendiente | — |

### 1.3 Requerimientos No Funcionales

| ID | Requerimiento | Detalle |
|---|---|---|
| RNF-01 | **Rendimiento** | El hardware objetivo es un MIPS Allegrex a 333 MHz con 64 MB de RAM. El renderizado debe mantenerse en dibujo de formas/sprites (`SDL_RenderCopy`/`FillRect`), evitando manipulación de píxel por píxel: se confirmó en la investigación previa que el backend SDL2 de PSP solo acelera nativamente los modos `ABGR8888` y `BGR565`, y que el acceso a superficie pixel a pixel es notoriamente lento en esta plataforma. |
| RNF-02 | **Latencia de audio** | Los efectos se cargan como `Mix_Chunk` (no streaming), vía `Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 2048)`. Es el modelo adecuado para SFX cortos disparados por evento; no se ha evaluado aún latencia para música en streaming (fuera de alcance de `audio_test`). |
| RNF-03 | **Compatibilidad de hardware** | Validado en PSP-3001 con CFW 6.20 PRO-B5, homebrew sin firma ejecutado desde `ms0:/PSP/GAME/`. PPSSPP como entorno de prueba intermedio antes de cada validación en hardware real. |
| RNF-04 | **Reproducibilidad del entorno de desarrollo** | Toolchain instalado vía WSL2 + Ubuntu (no nativo de Windows, por mayor soporte oficial del proyecto `pspdev`), documentado paso a paso para poder reconstruirse desde cero. |
| RNF-05 | **Mantenibilidad del código** | Convenciones ya establecidas y en uso: patrón de *exit callback* de PSP en todo ejecutable, reutilización de funciones compartidas entre entidades (`GetSpriteFrame`, `Overlaps`, `CanMoveTo`) en vez de duplicar lógica, un test aislado por milestone antes de integrar. |
| RNF-06 | **Gestión de memoria** | Liberación explícita de texturas (`SDL_DestroyTexture`) y audio (`Mix_FreeChunk`, `Mix_CloseAudio`) al cierre; sin fugas conocidas en las pruebas actuales. |
| RNF-07 | **Seguridad** | Fuera de alcance en su forma tradicional: sin conectividad en línea, sin datos de usuario sensibles. Aplica únicamente integridad de archivos de guardado (relevante a partir de RF-21, todavía no implementado). |
| RNF-08 | **Trazabilidad de dependencias de enlace** | Cada librería externa (SDL2_image, SDL2_mixer y sus dependencias transitivas: `vorbisfile`, `vorbis`, `ogg`, `modplug`, y `stdc++` por el uso interno de C++ de `libmodplug`) debe quedar documentada en el `Makefile` del módulo correspondiente — se confirmó con `audio_test` que esto no es opcional: el enlazado falla sin ellas. |

---

## 2. Arquitectura del Software (SAD)

### 2.1 Capas del sistema

| Capa | Responsabilidad | Tecnología / Componente |
|---|---|---|
| **COMP-HW** — Hardware/Firmware | Ejecución física de código no firmado | PSP-3001, CFW 6.20 PRO-B5 |
| **COMP-SDK** — SDK de plataforma | Hilos, callbacks de salida (botón Home), acceso a hardware de bajo nivel | `pspsdk` / `pspkernel.h` (`sceKernelCreateThread`, `sceKernelCreateCallback`, `sceKernelExitGame`) |
| **COMP-MM** — Framework multimedia | Ventana/renderer, carga de texturas, mezcla de audio, mapeo de control | SDL2 2.32.x, SDL2_image 2.8.x, SDL2_mixer 2.8.x |
| **COMP-ENGINE** — Motor de juego | Bucle principal, entrada normalizada, cámara, mundo/tilemap, colisión genérica | Código propio en C (`main.c` de cada módulo) |
| **COMP-ENTITIES** — Entidades y lógica de juego | Jugador, zombis, items, balas, armas, IA, combate | Código propio en C (structs `Zombie`, `Item`, `Bullet`, funciones `UpdateZombie`, `DamageZombie`, etc.) |
| **COMP-UI** — Presentación/HUD | Barra de vida, contador de items, indicador de arma/munición, tinte de pantalla al morir | Primitivas `SDL_RenderFillRect`/`DrawRect` sobre coordenadas de pantalla fija (no de mundo) |
| **COMP-CONTENT** — Contenido/Assets | Sprites y audio reales, extraídos del repositorio original | `.png` (`player.png`, `zombie.png`, `item.png`) y `.ogg` (`footstep`, `pickup`, `shot`, `zombie_attack`) |

El flujo de dependencia es estrictamente descendente: COMP-ENTITIES y COMP-UI dependen de COMP-ENGINE; COMP-ENGINE depende de COMP-MM; COMP-MM depende de COMP-SDK; nada depende "hacia arriba". Esto es lo que permitió, por ejemplo, que `GetSpriteFrame` (COMP-ENGINE) se reutilizara sin cambios tanto para el jugador como para el zombi (COMP-ENTITIES) al introducir la IA.

### 2.2 Integración de componentes — caso de estudio: audio

Se documenta este caso porque fue explícitamente solicitado y porque es representativo del patrón usado para *todo* evento de juego, no solo sonido.

**Ciclo de vida (propiedad centralizada en `main()`):**
1. Al arrancar: `SDL_Init` incluye `SDL_INIT_AUDIO`; `Mix_OpenAudio(...)` abre el dispositivo una sola vez.
2. Carga: los 4 efectos se cargan una sola vez como `Mix_Chunk*` (`Mix_LoadWAV`) antes del bucle principal — nunca dentro del bucle.
3. Reproducción (disparada por evento, no por polling continuo): cada punto de COMP-ENGINE/COMP-ENTITIES donde ya ocurre un evento de gameplay llama `Mix_PlayChannel(-1, chunk, 0)` en el mismo lugar donde ese evento ya se procesaba:
   - Paso → dentro del bloque de movimiento, cuando el temporizador de zancada llega a `0.3s` y `isMoving` es verdadero.
   - Recolección → dentro del chequeo de superposición jugador-item, en el mismo `if` que desactiva el item.
   - Disparo → dentro de la rama de arma `WEAPON_PISTOL`, en el mismo bloque que descuenta munición.
   - Ataque de zombi → dentro del chequeo de daño de contacto, en el mismo bloque que resta vida.
4. El canal (`-1`) se delega al pool interno de SDL2_mixer (8 canales por defecto), por lo que sonidos simultáneos (p. ej. paso + disparo) no requieren gestión manual de voces.
5. Cierre: los 4 `Mix_FreeChunk` y `Mix_CloseAudio` se centralizan al final de `main()`, simétrico a la carga.

**Dependencia de enlace no evidente (hallazgo real, no anticipado):** `SDL2_mixer` requiere en tiempo de enlace `vorbisfile` + `vorbis` + `ogg` (decodificación OGG) y `modplug` (soporte de módulos, sin uso de contenido en este proyecto pero exigido igual por el enlazador estático). `libmodplug` está escrita en C++ internamente (usa `operator new`/`delete`), por lo que además fue necesario enlazar `-lstdc++` **a pesar de que todo el código propio es C puro**. Esto quedó documentado en el `Makefile` de `audio_test` precisamente para que no se repita como debugging desde cero en el siguiente módulo que use audio.

### 2.3 Stack tecnológico

| Categoría | Tecnología | Detalle / Versión |
|---|---|---|
| Toolchain de compilación | `psp-gcc` / `pspsdk` (proyecto `pspdev`) | 15.2.0, objetivo MIPS Allegrex |
| Entorno de desarrollo host | WSL2 + Ubuntu | Elegido sobre un toolchain nativo de Windows por ser la ruta oficialmente documentada y con más soporte |
| Editor | VS Code + extensión "WSL" (Microsoft) | Terminal integrada conectada directamente a WSL |
| Gráficos | SDL2 2.32.x | `SDL_Renderer` acelerado, `SDL_GameController` para entrada |
| Carga de imágenes | SDL2_image 2.8.x | Requiere `-ljpeg -lpng -lz` además de `-lSDL2_image` |
| Audio | SDL2_mixer 2.8.x | Requiere `-lvorbisfile -lvorbis -logg -lmodplug -lstdc++` (ver §2.2) |
| Emulador de iteración | PPSSPP | Prueba rápida en PC antes de cada validación en hardware real |
| Hardware de validación final | PSP-3001 (Slim) | CFW 6.20 PRO-B5, ejecución desde `ms0:/PSP/GAME/` |
| Control de versiones | Git + GitHub | Repositorio `MiniDayZ-PSP` |
| Fuente de contenido (solo assets) | Repositorio `NextDev65/MiniDayZ` | Extracción manual de `.png`/`.ogg` — **nunca** de `c2runtime.js`/`data.js` |

### 2.4 Estructura de Carpetas y Módulos de Código

Las 11 pruebas construidas hasta ahora (`tests/` tras la reorganización de esta sección) cumplieron su función: cada una validó **una sola mecánica**, de forma aislada y compilable. Pero cada una tiene su propio `main()`, su propio `BuildTestMap`, su propio bucle — si el "juego real" se siguiera escribiendo así, `src/` sería 11 programas sueltos que se duplican entre sí, no un juego integrado.

**Decisión de arquitectura:** las pruebas se separan de la arquitectura real. `src/` pasa a organizarse **por dominio** (qué hace cada cosa), no por milestone de prueba (en qué orden se construyó). Esto reutiliza el mismo código ya validado, sin duplicarlo.

```
MiniDayZ-PSP/
├── assets/
├── design/
│   └── SRS-SAD-KANBAN.md
├── tests/                       # todo lo construido hasta ahora, movido tal cual
│   ├── hello_world/
│   ├── graphics_test/
│   ├── sprite_test/
│   ├── camera_test/
│   ├── collision_test/
│   ├── item_test/
│   ├── zombie_test/
│   ├── health_test/
│   ├── combat_test/
│   ├── weapons_test/
│   └── audio_test/
├── src/                          # EL JUEGO REAL — por dominio, no por prueba
│   ├── core/
│   │   ├── platform.c / .h        # arranque de PSP: exit callback, hilo de callbacks
│   │   ├── game_loop.c / .h       # bucle principal: deltaTime, orden de update/render
│   │   └── math_utils.c / .h      # Overlaps() y utilidades sin estado propio
│   ├── world/
│   │   ├── map.c / .h             # tiles, mapa, IsTileBlocking
│   │   ├── camera.c / .h          # seguimiento y límites de cámara
│   │   └── collision.c / .h       # CanMoveTo — colisión genérica contra el mapa
│   ├── entities/
│   │   ├── player.c / .h          # posición, vida, arma equipada, munición
│   │   ├── zombie.c / .h          # struct Zombie, UpdateZombie, DamageZombie
│   │   ├── item.c / .h            # struct Item, colocación y recolección
│   │   └── bullet.c / .h          # struct Bullet, SpawnBullet, UpdateBullets
│   ├── systems/
│   │   ├── animation.c / .h       # GetSpriteFrame — compartido entre jugador y zombis
│   │   ├── combat.c / .h          # GetAttackRect, FindNearestZombieInRange, resolución de ataque
│   │   └── audio.c / .h           # carga/reproducción de Mix_Chunk, centralizada
│   ├── ui/
│   │   └── hud.c / .h             # barra de vida, contador de items, arma/munición, tinte de muerte
│   └── main.c                     # punto de entrada: inicializa subsistemas y arranca el game loop
├── Makefile                       # uno solo para todo src/ — ya no uno por carpeta de prueba
├── .gitignore
└── README.md
```

| Carpeta | Qué va aquí | Ejemplo ya validado en las pruebas |
|---|---|---|
| `core/` | Arranque de la plataforma y el bucle principal — no conoce "zombis" ni "items", solo orquesta | `SetupCallbacks`, `exit_callback`, el `while (running)` |
| `world/` | El mapa y todo lo que depende de coordenadas de mundo | `worldMap`, `IsTileBlocking`, `CanMoveTo`, cálculo de `cameraX/cameraY` |
| `entities/` | Cada "cosa" que existe en el mundo, con su propio estado | `struct Zombie`, `struct Item`, `struct Bullet`, el estado del jugador |
| `systems/` | Lógica que opera **sobre varias entidades a la vez** — no le pertenece a una sola | `GetSpriteFrame` (lo usan jugador y zombi por igual), `FindNearestZombieInRange` |
| `ui/` | Todo lo que se dibuja fijo en pantalla, sin cámara | Barra de vida, contador de latas, indicador de arma/munición |

**Nota de compilación:** el mismo sistema `build.mak` de pspsdk acepta objetos de subcarpetas listados en `OBJS` (p. ej. `OBJS = main.o core/platform.o world/map.o entities/zombie.o ...`), así que pasar de un `Makefile` por prueba a uno solo para todo `src/` no exige cambiar de sistema de build — se define cuando empecemos a escribir estos archivos.



> **Nota sobre las estimaciones:** estos números son una estimación de ritmo de desarrollo sostenido (una persona, con apoyo de IA para la escritura y verificación de código). No representan el tiempo real que tomaron las pruebas descritas en este documento, que fue considerablemente menor por tratarse de una sesión de trabajo conversacional continua. Se usan aquí para planificación realista de las fases restantes.

| Fase | Contenido | Estado | Estimación |
|---|---|---|---|
| **Fase 0 — Entorno** | Instalación de toolchain (WSL2 + pspdev), validación de compilación y ejecución ("Hello World") en PPSSPP y hardware real | ✅ Completada | 1 semana |
| **Fase 1 — Núcleo de render/input** | Ventana/renderer SDL2, entrada por D-pad y stick, carga de sprite real, animación direccional con delta time | ✅ Completada | 2 semanas |
| **Fase 2 — Mundo** | Cámara con seguimiento y límites, tilemap con culling de tiles visibles, colisión AABB con deslizamiento | ✅ Completada | 2 semanas |
| **Fase 3 — Entidades base** | Objetos recolectables, IA de persecución por radio para zombis | ✅ Completada | 2 semanas |
| **Fase 4 — Combate y audio** | Vida/daño con enfriamiento, combate cuerpo a cuerpo automático, arma a distancia manual con balas reales, cambio de arma, reinicio tras morir, integración de audio real por evento | ✅ Completada | 3 semanas |
| **Fase 5 — Supervivencia** | RF-15 (hambre/sed) y ampliación del HUD | 🔲 Pendiente | 2 semanas |
| **Fase 6 — Inventario y crafteo** | RF-16, RF-17 | 🔲 Pendiente | 3 semanas |
| **Fase 7 — Contenido y combate avanzado** | RF-13, RF-14 (mira/fijado — requiere aclarar diseño primero), RF-19 (tipos de zombi), RF-20 (mapa real) | 🔲 Pendiente | 4–6 semanas (mayor incertidumbre: depende de decisiones de arte/alcance aún abiertas) |
| **Fase 8 — Persistencia y menús** | RF-21 (guardado/carga), RF-22 (menús) | 🔲 Pendiente | 2 semanas |
| **Fase 9 — Pulido y despliegue** | Optimización de rendimiento en hardware real (RNF-01), empaquetado final, pruebas de regresión sobre todo lo anterior | 🔲 Pendiente | 2–3 semanas |

**Total ya validado (Fases 0–4):** 10 semanas de esfuerzo equivalente.
**Total estimado restante (Fases 5–9):** 13–16 semanas.

---

## 4. Mapeo y Ejecución Ágil (Tablero Kanban)

### 4.1 Estados del tablero

| Estado | Significado | Quién opera en este proyecto |
|---|---|---|
| **Backlog** | Requerimiento identificado, sin iniciar | — |
| **In Progress** | Diseño y escritura de código en curso | Claude (asistente) |
| **Code Review** | El código compila limpio contra el toolchain real; cuando aplica, se ejecutan pruebas de lógica extraídas de forma aislada (como se hizo con `weapons_test`, 16 aserciones sobre balas/daño/objetivo más cercano) | Claude (asistente), antes de entregar |
| **Testing** | Prueba interactiva en PPSSPP y/o en el PSP-3001 físico | Usuario |
| **Done** | Confirmado funcional por el usuario en pruebas interactivas | Usuario |

### 4.2 Matriz de trazabilidad: Fase → Requerimiento → Componente → Tarjeta

| Fase | Requerimiento(s) | Componente | Tarjeta Kanban | Estado |
|---|---|---|---|---|
| 0 | RNF-04 | COMP-SDK | KAN-01 — Instalar y validar toolchain PSP (`hello_world`) | Done |
| 1 | RF-01 | COMP-ENGINE, COMP-MM | KAN-02 — Render de figura básica + entrada por D-pad/stick (`graphics_test`) | Done |
| 1 | RF-01, RF-02 | COMP-ENTITIES, COMP-CONTENT | KAN-03 — Carga de sprite real y animación 4-direccional (`sprite_test`) | Done |
| 2 | RF-03 | COMP-ENGINE | KAN-04 — Cámara con seguimiento y tilemap con scroll (`camera_test`) | Done |
| 2 | RF-04 | COMP-ENGINE | KAN-05 — Colisión AABB contra paredes, con deslizamiento (`collision_test`) | Done |
| 3 | RF-05 | COMP-ENTITIES, COMP-UI | KAN-06 — Recolección de items + contador en HUD (`item_test`) | Done |
| 3 | RF-06 | COMP-ENTITIES | KAN-07 — IA de persecución del zombi por radio (`zombie_test`) | Done |
| 4 | RF-07 | COMP-ENTITIES, COMP-UI | KAN-08 — Vida, daño de contacto con enfriamiento, estado de muerte (`health_test`) | Done |
| 4 | RF-08 | COMP-ENTITIES | KAN-09 — Combate cuerpo a cuerpo automático (`combat_test`) | Done |
| 4 | RF-09, RF-10 | COMP-ENTITIES, COMP-UI | KAN-10 — Pistola manual, balas reales, cambio de arma (`weapons_test`) | Done |
| 4 | RF-11 | COMP-MM, COMP-CONTENT | KAN-11 — Integración de SDL2_mixer y 4 efectos reales (`audio_test`) | Done |
| 4 | RF-12 | COMP-ENGINE | KAN-12 — Reinicio completo de partida tras morir (`combat_test`) | Done |
| Transición | RNF-05 | Todos (reestructuración) | KAN-22 — Separar `tests/` de `src/`; reorganizar `src/` por dominio (`core`, `world`, `entities`, `systems`, `ui`) | In Progress |
| 5 | RF-15 | COMP-ENTITIES, COMP-UI | KAN-13 — Hambre y sed | Backlog |
| 6 | RF-16 | COMP-ENTITIES, COMP-UI | KAN-14 — Sistema de inventario real | Backlog |
| 6 | RF-17 | COMP-ENTITIES | KAN-15 — Sistema de crafteo | Backlog |
| 7 | RF-13, RF-14 | COMP-ENTITIES | KAN-16 — Rango fijo por arma + mira con fijado progresivo (**requiere sesión de aclaración de diseño antes de estimarse en detalle**) | Backlog |
| 7 | RF-19 | COMP-ENTITIES, COMP-CONTENT | KAN-17 — Tipos adicionales de zombi (tank/screamer/jumper/shooter) | Backlog |
| 7 | RF-20 | COMP-CONTENT, COMP-ENGINE | KAN-18 — Diseño e integración del mapa real de juego | Backlog |
| 8 | RF-21 | COMP-SDK | KAN-19 — Guardado/carga en memory stick | Backlog |
| 8 | RF-22 | COMP-UI | KAN-20 — Menús de inicio, pausa y muerte | Backlog |
| 9 | RNF-01 | COMP-ENGINE | KAN-21 — Optimización de rendimiento en hardware real | Backlog |

### 4.3 Ejemplo de tarjeta Kanban detallada — KAN-11 (módulo de sonido)

---

**KAN-11 — Integración de SDL2_mixer y reproducción de efectos de sonido reales**

**Historia de usuario:**
> Como jugador, quiero escuchar sonidos de pasos, disparo, impacto de zombi y recolección de items, para que el juego se sienta reactivo y no silencioso.

**Componente(s):** COMP-MM, COMP-CONTENT
**Requerimiento(s) relacionado(s):** RF-11
**Prioridad:** Alta (bloqueaba la sensación de "juego terminado" de todo lo construido en Fase 4)

**Criterios de aceptación:**
- **Dado** que el jugador se mueve de forma continua, **cuando** transcurren ~0.3s de movimiento, **entonces** se reproduce el sonido de paso.
- **Dado** que el jugador se detiene, **cuando** deja de haber entrada de movimiento, **entonces** el temporizador de paso se reinicia sin sonido adicional.
- **Dado** que el jugador toca un item activo, **cuando** ocurre la superposición, **entonces** se reproduce el sonido de recolección exactamente una vez por item.
- **Dado** que el jugador dispara con la pistola equipada y hay munición disponible, **cuando** se genera la bala, **entonces** se reproduce el sonido de disparo.
- **Dado** que un zombi vivo toca al jugador y el enfriamiento de daño lo permite, **cuando** se aplica el daño, **entonces** se reproduce el sonido de ataque de zombi.
- **Dado** que dos o más de estos eventos ocurren en el mismo frame, **cuando** esto pasa, **entonces** todos los sonidos se escuchan sin cortarse entre sí.
- El proyecto compila y enlaza contra el toolchain oficial (`psp-gcc` 15.2.0) sin advertencias ni errores.

**Definición de Hecho (DoD):**
- [x] Compilación limpia: 0 advertencias, 0 errores, verificada directamente contra el toolchain real (no solo revisión de código).
- [x] Los 4 efectos de sonido son audio real extraído del repositorio del juego original, no marcadores de posición.
- [x] Todos los recursos de audio (`Mix_Chunk*` y el dispositivo abierto por `Mix_OpenAudio`) se liberan correctamente al cerrar el programa.
- [x] Todas las dependencias de enlace transitivas (`vorbisfile`, `vorbis`, `ogg`, `modplug`, `stdc++`) quedan documentadas en el `Makefile` del módulo.
- [x] Probado interactivamente por el usuario en PPSSPP, con confirmación explícita de que los 4 sonidos tuvieron el efecto esperado.

**Notas técnicas:** ver §2.2 de este documento para el patrón de integración completo.

---
