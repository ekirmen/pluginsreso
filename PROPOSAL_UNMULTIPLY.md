# Propuesta de Nuevo Plugin: **UnMultiply (Alpha Master)**

Basado en la referencia de "SiPS UnMultiply", este plugin está diseñado para la extracción avanzada de canales alfa y el refinamiento de bordes, proporcionando un control total sobre cómo se genera y procesa la transparencia en tiempo real.

## 🎯 Objetivo
Permitir a los usuarios de Resolume extraer transparencia de contenidos que no la tienen (o que tienen una premultiplicación incorrecta) utilizando pesos de color personalizados y herramientas de limpieza de bordes.

---

## 🛠 Parámetros Propuestos

| Categoría | Parámetro | Descripción |
| :--- | :--- | :--- |
| **Generación de Alfa** | `Mode` | Selector de algoritmo: Max RGB, Luminanza, Promedio, o un Canal específico. |
| | `R Weight`, `G Weight`, `B Weight` | Control de la importancia de cada canal en la generación de la máscara. |
| **Refinamiento** | `Black Clip` | Elimina el ruido en las zonas oscuras (umbral mínimo). |
| | `White Point` | Define el nivel de blanco puro para la opacidad máxima. |
| | `Alpha Gamma` | Ajusta la curva de contraste del canal alfa extraído. |
| | `Feather` | Suavizado de los bordes del alfa para evitar cortes "duros". |
| **Procesamiento** | `Unpremult Col` | Des-premultiplica el color original (útil si el contenido vino con fondo negro "pegado"). |
| | `Premult Output` | Vuelve a premultiplicar el resultado final para un blending correcto en Resolume. |
| | `By Input Alpha` | Si el contenido ya tiene alfa, lo usa como base para el recorte. |
| **Limpieza de Bordes** | `Fringe Suppr.` | Suprime los bordes de color no deseados (flecos) en los recortes. |
| | `Fringe Knee` | Ajusta la dureza de la supresión de bordes. |

---

## 🔥 Mejoras Sugeridas ("O mejor")

Para superar el estándar de la imagen, propongo añadir:

1.  **Modo de Visualización (Preview)**: Un toggle para ver solo el canal Alfa (blanco y negro) o el "Checkboard" de fondo, facilitando el ajuste fino.
2.  **Color Keying Complementario**: Opción de seleccionar un color específico para extraer (Chroma Key básico integrado).
3.  **Invert Alpha**: Un botón rápido para invertir la máscara generada.
4.  **Edge Erosion/Dilation**: Contraer o expandir el borde del alfa un par de píxeles (crucial para eliminar bordes negros o blancos).

---

## 📂 Estructura de Archivos
Se creará la carpeta `source/plugins/UnMultiply/` con:
- `UnMultiply.h`: Definición de la clase.
- `UnMultiply.cpp`: Lógica del shader GLSL y parámetros FFGL.
- `CMakeLists.txt`: Configuración para la compilación.

¿Qué te parece esta propuesta? Si estás de acuerdo, procederé a crear la estructura y el código base.
