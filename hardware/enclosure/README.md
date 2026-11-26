# Carcasa Ergonómica — Kairos-remote

Alojamiento compacto tipo guijarro (`Ø54 mm × 25.2 mm`) diseñado específicamente para la placa **Waveshare ESP32-S3-Touch-LCD-1.46** y una celda **LiPo 3.7V de 350–500 mAh** (ej. `502535` o `602535` con conector JST 1.25 mm).

## Archivos incluidos

- [kairos_enclosure.py](kairos_enclosure.py): Fuente paramétrica principal en Python (`build123d`) que exporta los sólidos B-Rep y mallas.
- [kairos_enclosure.scad](kairos_enclosure.scad): Fuente paramétrica equivalente en OpenSCAD.
- `kairos_body.stl` / `kairos_body.step`: Cuerpo principal con bisel frontal para pantalla de 1.46", acceso USB-C, ventana para botones físicos laterales, ojal para cordón de muñeca (*lanyard*) y bahía interna aislada para la LiPo.
- `kairos_back_cover.stl` / `kairos_back_cover.step`: Tapa trasera con encaje a presión, ranuras acústicas para el altavoz integrado y alojamientos avellanados para 2 tornillos **M2×6 mm**.

## Parámetros recomendados de impresión 3D

| Parámetro | Recomendación |
| :--- | :--- |
| **Material** | PLA+ o PETG (mate oscuro recomendado para escenarios) |
| **Altura de capa** | `0.20 mm` (boquilla `0.4 mm`) |
| **Perímetros** | `3` paredes (`1.2 mm`) |
| **Relleno** | `20%` Gyroid |
| **Soportes** | **No necesarios** (imprimir el cuerpo principal boca abajo con el bisel apoyado en la cama, y la tapa trasera con la cara exterior apoyada en la cama) |
| **Tornillería** | 2 × tornillos **M2 × 6 mm** (opcional si se usa solo encaje a presión) |

## Regenerar STL y STEP

```bash
uv run --python 3.12 --with build123d python hardware/enclosure/kairos_enclosure.py
```
