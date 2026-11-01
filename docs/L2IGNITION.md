# L2 Ignition: cambios propios

Rama `l2ignition` del fork de [luannbr/Lineage2_Interface_source_C4_-Nwindow.dll-](https://github.com/luannbr/Lineage2_Interface_source_C4_-Nwindow.dll-).
Todo lo agregado va detrás de `NW_CUSTOM` (opción de CMake `NWD_CUSTOM`, encendida por defecto) y solo entra en el
build standalone; el build hook sigue siendo fiel al original.

## Qué agrega

- **Barras de atajos extra** (`src/custom/NCustomShortCuts.cpp`): hasta 4 barras más que muestran otras páginas de las
  mismas 120 casillas (el servidor no cambia). Se apilan junto a la barra principal, giran, se ocultan y se mueven con
  ella. Botón **+** en la barra principal. Arrastrar un atajo de una barra a otra lo mueve.
- **Teclas por barra**: cada barra (la principal también) usa F1–F12, la fila de números, QWERTY, ASDF, ZXCV o ninguna,
  por posición física de tecla (sirve con cualquier distribución). Todo menos F requiere el modo "chat con Enter".
- **Ventana "Barras de atajos"** (Alt+S, clic derecho sobre una barra fuera de las casillas o el ícono del menú):
  chat con Enter (la misma opción de Opciones del juego), cantidad de barras extra y teclas de cada barra.
- **Autopociones** (`src/custom/NCustomAutoPotion.cpp`, Alt+A o el ícono del menú): HP, MP y CP; la poción se arrastra
  desde el inventario, cada fila tiene Auto, porcentaje y tiempo mínimo entre usos.
- **Menú inferior**: dos íconos más (barras y autopociones).
- **Ventana de buffs** (`src/nwindow/NCAbnormalStatusWnd.cpp`): hasta 40 íconos en 4 filas de 10 (la original guarda 31
  y dibuja 30). La lista secundaria y el ícono extra van en la fila siguiente. El server manda 40 con `BuffIconRows = 4`.
- **GameGuard del login**: la respuesta automática devuelve la misma consulta en vez de ceros (como el parche que el
  cliente usaba con la DLL original).

La configuración queda en `system\NWindowCustom.ini` (`[ShortCut]` y `[AutoPotion]`).

## Compilar

Visual Studio 2022 o 2026 con x86 C++, CMake (el que trae Visual Studio sirve) y Python 3 con `pefile`. Copiar a `orig/`
los archivos del `system` del cliente C4 (ver `orig/README.md`).

```bat
cmake -S . -B build -G "Visual Studio 18 2026" -A Win32 -DNWD_GAME_SOURCES=ON
cmake --build build --config Release --target NWindowStandalone
```

Sale `build\standalone\Release\NWindow.dll`. En el repo del servidor va a `ops/client/nwindow/NWindow.dll`, y
`ops/client/build_system.py` la pone en el `system` que se reparte.

Probado con el cliente oficial NA de C4 (NWindow.dll del 13/01/2006, Engine.dll/Core.dll de enero de 2006) contra L2JLisvus.
