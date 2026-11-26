"""Builds and runs the headless LVGL 8.4 renderer, then composes circular PNG previews."""

import os
import subprocess
import tempfile
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]
LVGL_DIR = ROOT / ".pio" / "libdeps" / "waveshare_esp32s3_146" / "lvgl"
OUT_DIR = ROOT / "docs" / "assets"


def compile_renderer(build_dir: Path) -> Path:
    c_files = sorted((LVGL_DIR / "src").rglob("*.c"))
    cpp_files = [
        ROOT / "tools" / "lvgl_preview" / "render_screens.cpp",
        ROOT / "tools" / "lvgl_preview" / "stubs.cpp",
        ROOT / "src" / "ui" / "ui_manager.cpp",
        ROOT / "src" / "ui" / "screen_nav.cpp",
        ROOT / "src" / "ui" / "screen_stage.cpp",
        ROOT / "src" / "ui" / "screen_timer.cpp",
    ]
    inc_flags = [
        "-DLV_CONF_INCLUDE_SIMPLE",
        f"-I{ROOT / 'tools' / 'lvgl_preview' / 'stubs'}",
        f"-I{ROOT / 'include'}",
        f"-I{ROOT / 'src'}",
        f"-I{LVGL_DIR}",
        f"-I{LVGL_DIR / 'src'}",
    ]
    objs = []
    for src in c_files:
        obj = build_dir / (src.stem + "_" + str(abs(hash(src)))[:6] + ".o")
        objs.append(obj)
        if not obj.exists():
            subprocess.check_call(["cc", "-O2", "-c", *inc_flags, str(src), "-o", str(obj)])

    for src in cpp_files:
        obj = build_dir / (src.stem + "_cpp.o")
        objs.append(obj)
        subprocess.check_call(["c++", "-std=c++17", "-O2", *inc_flags, "-c", str(src), "-o", str(obj)])

    exe = build_dir / "render_screens"
    subprocess.check_call(["c++", "-std=c++17", "-O2", *[str(o) for o in objs], "-lm", "-o", str(exe)])
    return exe


def make_circular_badge(ppm_path: Path, png_path: Path) -> Image.Image:
    raw = Image.open(ppm_path).convert("RGBA")
    size = raw.size[0]
    mask = Image.new("L", (size, size), 0)
    draw_m = ImageDraw.Draw(mask)
    draw_m.ellipse((2, 2, size - 3, size - 3), fill=255)

    out = Image.new("RGBA", (size + 16, size + 16), (11, 15, 23, 0))
    draw_o = ImageDraw.Draw(out)
    draw_o.ellipse((0, 0, size + 15, size + 15), fill=(26, 30, 38, 255), outline=(64, 72, 86, 255), width=3)
    out.paste(raw, (8, 8), mask)
    out.save(png_path)
    return out


def compose_triptych(cards: list[tuple[Image.Image, str, str]], out_path: Path) -> None:
    w, h = 1420, 560
    canvas = Image.new("RGBA", (w, h), (15, 20, 30, 255))
    draw = ImageDraw.Draw(canvas)

    try:
        font_title = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", 24)
        font_sub = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", 18)
    except Exception:
        font_title = ImageFont.load_default()
        font_sub = ImageFont.load_default()

    xs = [46, 496, 946]
    for (img, title, sub), x in zip(cards, xs):
        canvas.paste(img, (x, 28), img)
        cx = x + img.width // 2
        draw.text((cx, 476), title, fill=(248, 250, 252, 255), font=font_title, anchor="mt")
        draw.text((cx, 510), sub, fill=(148, 163, 184, 255), font=font_sub, anchor="mt")

    canvas.convert("RGB").save(out_path)


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        exe = compile_renderer(tmp_path)
        subprocess.check_call([str(exe), str(tmp_path)])

        c1 = make_circular_badge(tmp_path / "screen1_nav.ppm", OUT_DIR / "screen1_nav.png")
        c2 = make_circular_badge(tmp_path / "screen2_stage.ppm", OUT_DIR / "screen2_stage.png")
        c3 = make_circular_badge(tmp_path / "screen3_timer.ppm", OUT_DIR / "screen3_timer.png")
        compose_triptych(
            [
                (c1, "Pantalla 1: Navegación principal", "4 arcos + SPACE (avance)"),
                (c2, "Pantalla 2: Control de escenario", "ESC · B · HOME · F + ENTER"),
                (c3, "Pantalla 3: Cronómetro", "MM:SS · PLAY/STOP · +1/+2/+5 min"),
            ],
            OUT_DIR / "screens_preview.png",
        )
    print("Generated LVGL previews in", OUT_DIR)


if __name__ == "__main__":
    main()
