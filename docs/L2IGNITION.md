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
- **Buffs de la party, la mascota y el summon** (`NCPartyStatusWnd::OnPaint`, `NCPetStatusWnd::OnPaint` y
  `NCSummonedStatusWnd::OnPaint` en `src/nwindow/NCStatusWnd.cpp`, constantes en `NCStatusWnd.h`): como las 3
  primeras filas de la ventana de buffs propia, en filas de 10 y hasta 30 íconos, en el orden en que los manda el
  server (la original, 2 filas de 10 de 16 px desde y 5). Los íconos miden 15 px desde el borde de arriba: las 3 filas
  ocupan 45 de los 46 px de cada miembro, así que la party no cambia de alto. El server manda como mucho 30
  (`PartySpelled`): si hay más, deja afuera los últimos buffs y nunca los debuffs. Antes mandaba 20 y pisaba los
  primeros con los que sobraban. Los debuffs (con marco) se dibujan al final.
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
  - **Botón de efecto de las armas Triumph** (`PaintEffectButton`, `OverEffectButton` y `ClickEffectButton`, enganchados en
    `NCEquipItemWnd`):
    - Es una estrella (`icon.etc_deco_star_i00`, 12 px sobre un cuadrado oscuro de 14 px) en la esquina de arriba a la
      derecha del casillero del arma. Aparece cuando el arma equipada figura como `<id> F` en `NWindowItems.txt`.
    - El clic pide al server el menú de efecto y color de esa arma: el bypass `triumphfx menu <object id>`, que atiende
      `custom.TriumphEffectManager` en el server de L2 Ignition. El arma no se levanta.
    - El doble clic sobre la estrella no la desequipa.
    - Con el mouse encima no sale el tooltip del ítem, y la estrella se ve más clara.
    - Una DLL vieja ignora las líneas `F`, y sin esas líneas la estrella no aparece.
  - **Botón del casco de los sets Light Crystal** (`PaintHelmetButton`, `OverHelmetButton` y `ClickHelmetButton`, con
    los mismos ganchos):
    - Es la misma estrella, en el casillero del casco (slot 1).
    - Aparece cuando el casco equipado figura como `<id> H` en `NWindowItems.txt`.
    - El clic pide el menú para mostrar u ocultar el casco del costume: el bypass `wchelm menu <object id>`, que
      atiende `custom.WhiteCrystalHelmetManager`.
    - Lo demás funciona igual que la estrella del arma: no levanta el casco, el doble clic no lo desequipa y no muestra
      el tooltip.
- **"Mute All Sounds"** (`src/custom/NCustomAudio.cpp`): un checkbox en Opciones → Audio, debajo de los cuatro volúmenes.
  Tildarlo guarda los niveles que muestran, los pone en Off y los deja en gris; destildarlo devuelve los guardados. Se
  aplica con OK / Aplicar, como las demás opciones (el `Apply` original pone los volúmenes; Off corta lo que suena), y
  ahí se guardan el estado y los niveles en `[Audio]` (`MuteAll`, `Level0` .. `Level3`: sonido, música y las dos voces,
  0 = "5" .. 5 = Off). Cancelar no cambia nada: al abrir la página se ve lo guardado.
- **Efectos de las armas Triumph** (`src/custom/NCustomTriumph.cpp`): dos checkboxes en Opciones → Game, debajo de
  "3D Arrow", donde C4 sacó la calidad de las capturas: "Ver efectos de mis armas Triumph" y "Ver efectos de armas
  Triumph de otros". Vienen tildados.
  - Se guardan con OK / Aplicar en `[Triumph]` (`ShowOwn`, `ShowOthers`).
  - Dentro del mundo, se mandan al server con el bypass `triumphfx view <propias> <de otros>`.
  - El server los olvida con el personaje, así que la DLL los vuelve a mandar 3 s después de cada entrada al mundo
    (`Tick`, desde `NCustomShortCuts::Tick`), solo si alguno está destildado.
  - Los dibuja el server: manda el arma sin efecto a quien no los quiere ver.
- **Filtro anisotrópico y texturas HD** (`src/custom/NCustomGraphics.cpp`): dos checkboxes en Opciones → Video, en la
  columna de los checkboxes: "Filtro anisotropico" al lado de "Texture Filter" y "Texturas HD (al reiniciar)" debajo.
  Vienen destildados, así el cliente se ve como el original. Se guardan con OK / Aplicar en `[Graphics]`
  (`Anisotropy`, `HDTextures`).
  - **Filtro anisotrópico**, se aplica al instante. El `l2.ini` repartido trae `LevelOfAnisotropy=1`, y D3DDrv solo lo
    usa en `SetRes`; su caché de estados y `SetUseTrilinear` vuelven a poner el filtro lineal.
    - Por eso la DLL envuelve `IDirect3DDevice9::SetSamplerState` (slot 69 de la vtable del device, que está en
      `UD3DRenderDevice+0x4758`).
    - Con el checkbox tildado, un filtro de minificación lineal pasa a anisotrópico y el máximo de anisotropía pasa al
      del device (hasta 16x).
    - El filtro de punto ("Texture Filter" destildado) y los filtros de magnificación y de mips quedan como los pide el
      motor.
    - Al cambiar el checkbox se vuelven a poner los 16 samplers desde lo último que pidió el motor.
  - **Texturas HD**, se aplica al reiniciar. Si existe la carpeta `TexturesHD` al lado de `system`, la DLL agrega
    `../TexturesHD/*.utx` como primera ruta de `GSys->Paths` (`USystem+0x64`, la lista `Paths` de `[Core.System]`).
    - Lo hace en el primer tick de la consola, en la pantalla de login, antes de que cargue el mundo.
    - `appFindPackageFile` recorre esa lista en cada carga, así que un paquete de `TexturesHD` con el mismo nombre que
      uno del juego le gana al de `Textures`.
    - Sin la carpeta el checkbox queda en gris. El cliente no vuelve a cargar paquetes ya cargados: por eso pide reiniciar.
  - Probado en el juego (11/21): el piso de la selección de personaje y el pasto del Coliseo, con el filtro prendido y
    apagado en vivo y al arrancar. Un `IgnIcons.utx` teñido de rojo en `TexturesHD` reemplazó al de `SysTextures` solo
    con el checkbox tildado. El clic real en los checkboxes no se probó: los clics por PostMessage no llegan a la
    interfaz.
- **GameGuard del login**: la respuesta automática devuelve la misma consulta en vez de ceros (como el parche que el
  cliente usaba con la DLL original).
- **Un cliente por PC** (`src/custom/NCustomSingleClient.cpp`, desde el constructor de `NConsoleWnd`):
  - El primer cliente toma el mutex `Global\L2Ignition.SingleClient` y lo tiene hasta cerrarse. `Global\` hace que
    cuente también otra sesión de Windows de la misma PC.
  - Un segundo cliente lo encuentra tomado, esconde su ventana, avisa "Ya hay un cliente de L2 Ignition abierto en esta
    PC" y se cierra antes de la pantalla de login.
  - Si el `ServerAddr` de `l2.ini` es la misma PC (`127.x`, `localhost`, `::1`), no se toma el mutex. Así se pueden
    abrir varios clientes de prueba contra un server local, también al lado de uno que juega en el server real.
  - Es solo una traba: se saltea cerrando el handle del mutex o usando la `NWindow.dll` original. El control firme es
    el HWID que verifica el server.
- **HWID** (`src/custom/NCustomHwid.cpp`, desde `NConsoleWnd::ReceiveCharacterInfoX`): cada vez que llega la lista de
  personajes, antes de elegir uno, manda al server el bypass `hwid 1 <smbios> <win> <disk> <firma>`.
  - Cada campo son los primeros 16 bytes de `SHA-256("L2Ignition.hwid.1|<nombre>|<valor>")` en hex, o `-` si no hay
    dato. Los valores crudos no salen de la PC.
  - `smbios`: el UUID del sistema (tipo 1) y la serie de la placa madre (tipo 2) de `GetSystemFirmwareTable('RSMB')`,
    sin los valores de relleno de fábrica.
  - `win`: el `MachineGuid` del registro, en la vista de 64 bits (`KEY_WOW64_64KEY`, porque el cliente es de 32 bits).
  - `disk`: la serie del disco físico donde está Windows. Se lee con `IOCTL_STORAGE_QUERY_PROPERTY` sobre un handle
    abierto sin acceso, así que no pide permisos de administrador.
  - `firma`: los primeros 16 bytes de `HMAC-SHA256(clave, "L2Ignition.hwid.sig.1|<cuenta>|<smbios> <win> <disk>")` en
    hex.
    - La cuenta, en minúsculas, la guarda `NConsoleWnd::RequestAuthLogin` en cada login.
    - La clave no está en el repo: ver "Compilar". Sin clave, la firma es `-`.
  - El server de L2 Ignition decide con eso qué PC es y revisa la firma (`docs/hwid.md` de su repo).

La configuración queda en `system\NWindowCustom.ini` (`[ShortCut]`, `[AutoPotion]`, `[Damage]`, `[Audio]` y `[Triumph]`).

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

**La clave de la firma del HWID** nunca va a git. Para la DLL que se reparte, hay que configurar con un archivo local
que tenga la línea `HWID_SECRET=<64 hex>`: en L2 Ignition es `ops/credentials.local.txt` del repo del servidor, la
misma clave que el server tiene en `HWID_SECRET`.

```bat
cmake build -DNWD_HWID_KEY_FILE=C:/Users/drone/Desktop/L2Server/ops/credentials.local.txt
```

- CMake escribe `build/generated/NCustomHwidKey.h` con la clave enmascarada con XOR. La máscara sale de la clave, así
  que el header solo cambia si cambia la clave.
- El valor queda en la caché de CMake, así que alcanza con hacerlo una vez por carpeta `build`.
- Sin la opción, la DLL compila igual pero manda la firma `-`. Si el server tiene la clave, no le acepta el HWID.

Probado con el cliente oficial NA de C4 (NWindow.dll del 13/01/2006, Engine.dll/Core.dll de enero de 2006) contra L2JLisvus.
