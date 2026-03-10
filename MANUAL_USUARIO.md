# Manual Técnico: Plugin Screen Capture

**Versión 2.0**

## 1. Instalación y Ubicación
1.  **Archivo**: Localice `FFGLScreenCapture.dll`.
2.  **Carpeta**: Copie el archivo en `Documentos\Resolume Arena\Extra Effects`.
3.  **Carga**: En Resolume, busque **"Screen Capture"** en la pestaña *Effects* o *Sources* y arrástrelo a un clip.

## 2. Selección de Ventana
*   **Window Select**: Deslice este parámetro para recorrer las ventanas abiertas.
*   **Refresh List**: Actualiza la lista si abrió una app nueva.

> **IMPORTANTE**: La aplicación destino debe estar en **Modo Ventana** (Windowed). No funciona en Pantalla Completa Exclusiva. Si minimiza la ventana, la captura se detendrá.

## 3. Ajustes y Crop (Recorte)
La captura incluye todo el marco de la ventana. Use estos controles para limpiarla:

*   **Crop Left / Right / Top / Bottom**: Recorta los bordes no deseados.
*   **Pos X / Pos Y**: Mueve la imagen dentro del lienzo (0.5 es centro).
*   **Fit Aspect Ratio**: Corrige la distorsión si la imagen se ve estirada.

## 4. Captura de Navegadores (Edge / Chrome)
Si la ventana del navegador se ve **NEGRA**, es por la aceleración de hardware.

### Solución A: Flags de Inicio (Códigos)
Modifique el acceso directo del navegador añadiendo estas "flags" al final del campo *Destino*:

`--disable-gpu --disable-d3d11`

**Ejemplo:**
`"C:\...\msedge.exe" --disable-gpu`

### Solución B: Ajustes
Vaya a `Configuración > Sistema` en el navegador y desactive **"Usar aceleración de hardware"**.

### Solución C: Force Screen Blt
Active el parámetro **Force Screen Blt** en el plugin. *Nota: La ventana debe estar visible en el escritorio para que esto funcione.*
