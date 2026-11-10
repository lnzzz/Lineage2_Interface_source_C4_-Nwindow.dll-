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
- **Inventario ancho** (`src/custom/NCustomInventory.cpp`): 640 x 372 en lugar de 256 x 401. A la izquierda el
  paperdoll: armadura y capa en una columna, joyas en la otra, arma y escudo abajo, el **modelo 3D del personaje** en el
  medio y los tatuajes debajo (las casillas vacías son los dibujos del fondo original, recortados). A la derecha, pestañas
  (Todo, Armas, Armaduras, Joyas, Consumibles, Materiales, Otros, Quest) sobre una grilla de 10 x 7, y abajo "Ordenar", la
  cantidad de ítems y la caja original de adena y peso con la basura y el cristalizar. Las pestañas de tipo ponen primero
  los ítems que coinciden y muestran solo esos; "Todo" devuelve el orden que había. "Ordenar" ordena la pestaña visible por
  tipo, grado, parte del cuerpo e ítem, y el orden queda en el archivo de `..\iwo`. Qué etc items son consumibles o
  materiales lo dice `system\NWindowItems.txt` (`<id> <C|M>` por línea), que arma el build del cliente del server. Las
  pestañas originales siguen existiendo (cambian de página) pero fuera de la vista. Una posición guardada para la ventana
  angosta que la deje fuera de la pantalla se corrige al abrirla.
  - El modelo: `FActorSceneNode` de Engine.dll (lo que usa `UCanvas::DrawActor`, que en este cliente no sirve: deja el
    `DrawScale` del pawn cambiado y pone la cámara en un lugar que no muestra nada) con una cámara propia en el origen
    mirando a +X, el pawn movido delante de ella durante la llamada (posición y rotación vuelven después). Antes de
    dibujar se vacía el lote de tiles 2D del canvas (`FCanvasUtil::Flush`, `UCanvas` +0xA8, como hace el `DrawActor` no
    virtual del motor): si no, el fondo de la ventana se dibuja después y tapa el modelo. Después el viewport del render
    interface pasa a ser la caja del modelo (`SetViewport`, +0x0C) y se limpia solo la profundidad (`Clear`, +0x10), así
    que el fondo traslúcido de la ventana queda detrás; el tamaño del `UViewport` (+0x94 / +0x98) es el de la caja
    mientras tanto, porque el nodo saca de ahí la proyección.
  - El borde de arriba de los paneles sale de la parte derecha de esa fila del fondo original: en el resto están los
    huecos transparentes de las pestañas originales. La esquina de arriba a la derecha original está cortada en diagonal
    contra el marco de la ventana: se arma con pedazos rectos (el canvas no dibuja partes espejadas, con ancho negativo).
  - La caja del modelo tiene la línea gris de 1 px de las casillas, a la par de la primera y la última casilla de cada
    columna. Los dibujos de las casillas vacías van 1 px más abajo que en el fondo original, para que el marco de un ítem
    equipado caiga justo sobre su borde.
- **GameGuard del login**: la respuesta automática devuelve la misma consulta en vez de ceros (como el parche que el
  cliente usaba con la DLL original).

La configuración queda en `system\NWindowCustom.ini` (`[ShortCut]` y `[AutoPotion]`).

## Correcciones de la reconstrucción

Valen para los dos builds (no dependen de `NW_CUSTOM`):

- **Nombres de las ventanas de estado** (`NCNameCtrlView::SetText` en `src/nwindow/NCStatusWnd.cpp`): guardaba el color
  donde el control lee la alineación (+0x1F8) y la alineación donde lee el color (+0x21C), así que no se veía el nombre
  propio, ni el del target, la mascota o el summon. La DLL original guarda el tercer argumento en +0x1F8 y el cuarto en
  +0x21C.
- **Cliente colgado al hacer clic sobre dos ventanas superpuestas** (`NCWnd::OnLButtonDown` en `src/nwindow/NCWnd.cpp`):
  devolvía 0 cuando la ventana no se arrastra y no tiene un ancestro que se arrastre; la DLL original devuelve 1 por los
  tres caminos (0x100363C6 salta al mismo `mov eax, 1`). Con 0, `NCVirtualWndMain::SetFocusingWindow` seguía con la
  ventana hermana siguiente, pero `SetFocus` ya había movido la tocada al principio de la lista que recorre: dos ventanas
  superpuestas (el chat de un NPC sobre la ventana de skills) se pasaban el foco para siempre y el juego quedaba al 100 %
  de un núcleo. Visto en vivo en un cliente colgado: `GFocusEntry` alternaba entre `NCHtmlViewer` y `NCSkillItemWnd`.

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
