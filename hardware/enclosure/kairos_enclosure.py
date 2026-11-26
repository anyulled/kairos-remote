"""Parametric 3D enclosure generator for Kairos-remote (Waveshare ESP32-S3-Touch-LCD-1.46).

Generates printable STL and CAD STEP files for:
  - kairos_body: main pebble-shaped circular housing with front bezel window,
    isolated LiPo compartment (38 x 28 x 7.5 mm), USB-C cutout, side button access,
    lanyard eyelet, and M2 mounting bosses.
  - kairos_back_cover: snap/M2 rear lid with countersunk holes and speaker slots.
"""

from pathlib import Path

from build123d import (
    Align,
    Axis,
    Box,
    BuildPart,
    Cylinder,
    Locations,
    Mode,
    chamfer,
    export_step,
    export_stl,
    fillet,
)

# --- Parametric dimensions (mm) ---
BODY_OUTER_D = 54.0
BODY_HEIGHT = 23.0
FRONT_LIP_H = 1.8
SCREEN_WINDOW_D = 38.4   # 37.1 mm active IPS area + chamfered reveal
PCB_POCKET_D = 45.4      # 44.77 mm Waveshare 1.46 cover glass + radial clearance
PCB_POCKET_H = 13.0      # 12.30 mm stack height + clearance
DIVIDER_THICKNESS = 1.4  # Isolates LiPo cell from PCB underside

LIPO_LEN = 38.5          # Fits 502535 / 602535 350-500 mAh LiPo
LIPO_WID = 28.5
LIPO_H = 8.0

COVER_THICK = 2.2
COVER_LIP_H = 2.0
COVER_LIP_CLEARANCE = 0.25

USBC_W = 11.5
USBC_H = 5.5
BTN_SLOT_W = 10.0
BTN_SLOT_H = 4.5

SCREW_OFFSET_R = 24.4
M2_PILOT_R = 0.88        # M2 self-tapping / brass insert pilot
M2_THROUGH_R = 1.15
M2_HEAD_R = 2.1

LANYARD_HOLE_R = 1.8


def build_body():
    with BuildPart() as part:
        # Main ergonomic cylindrical body
        Cylinder(
            radius=BODY_OUTER_D / 2.0,
            height=BODY_HEIGHT,
            align=(Align.CENTER, Align.CENTER, Align.MIN),
        )
        # Round top & bottom outer edges for comfortable palm grip
        outer_edges = part.edges().filter_by(lambda e: e.length > 100.0)
        fillet(outer_edges, radius=2.6)

        # Integrated lanyard eyelet ear at -Y
        with Locations((0.0, -BODY_OUTER_D / 2.0 - 1.5, BODY_HEIGHT / 2.0)):
            Cylinder(
                radius=4.5,
                height=8.0,
                rotation=(0.0, 90.0, 0.0),
                align=(Align.CENTER, Align.CENTER, Align.CENTER),
            )

        # Front circular display opening (Z = 0..FRONT_LIP_H)
        with Locations((0.0, 0.0, 0.0)):
            Cylinder(
                radius=SCREEN_WINDOW_D / 2.0,
                height=FRONT_LIP_H,
                align=(Align.CENTER, Align.CENTER, Align.MIN),
                mode=Mode.SUBTRACT,
            )
        # Chamfer front bezel edge for comfortable thumb swipes
        bezel_edge = part.edges().filter_by(
            lambda e: abs(e.center().Z) < 0.05 and abs(e.radius - SCREEN_WINDOW_D / 2.0) < 0.2
        )
        if bezel_edge:
            chamfer(bezel_edge, length=0.8)

        # Waveshare PCB pocket
        with Locations((0.0, 0.0, FRONT_LIP_H)):
            Cylinder(
                radius=PCB_POCKET_D / 2.0,
                height=PCB_POCKET_H,
                align=(Align.CENTER, Align.CENTER, Align.MIN),
                mode=Mode.SUBTRACT,
            )

        # Isolated LiPo bay above divider shelf (towards rear cover)
        lipo_z = FRONT_LIP_H + PCB_POCKET_H + DIVIDER_THICKNESS
        with Locations((0.0, 0.0, lipo_z)):
            Box(
                LIPO_LEN,
                LIPO_WID,
                BODY_HEIGHT - lipo_z + 0.1,
                align=(Align.CENTER, Align.CENTER, Align.MIN),
                mode=Mode.SUBTRACT,
            )

        # JST 1.25 mm wire pass-through slot between PCB pocket and LiPo bay
        with Locations((0.0, 12.0, FRONT_LIP_H + PCB_POCKET_H - 0.1)):
            Box(
                8.0,
                5.0,
                DIVIDER_THICKNESS + 0.4,
                align=(Align.CENTER, Align.CENTER, Align.MIN),
                mode=Mode.SUBTRACT,
            )

        # Rear cover snap/seat recess
        with Locations((0.0, 0.0, BODY_HEIGHT - COVER_LIP_H)):
            Cylinder(
                radius=(PCB_POCKET_D + 2.0) / 2.0,
                height=COVER_LIP_H + 0.1,
                align=(Align.CENTER, Align.CENTER, Align.MIN),
                mode=Mode.SUBTRACT,
            )

        # USB-C port cutout (+X)
        with Locations((BODY_OUTER_D / 2.0 - 4.0, 0.0, FRONT_LIP_H + 9.2)):
            Box(
                10.0,
                USBC_W,
                USBC_H,
                align=(Align.CENTER, Align.CENTER, Align.CENTER),
                mode=Mode.SUBTRACT,
            )

        # Physical PWR/BOOT button access (-X)
        with Locations((-BODY_OUTER_D / 2.0 + 4.0, 0.0, FRONT_LIP_H + 9.2)):
            Box(
                10.0,
                BTN_SLOT_W,
                BTN_SLOT_H,
                align=(Align.CENTER, Align.CENTER, Align.CENTER),
                mode=Mode.SUBTRACT,
            )

        # Lanyard through-hole
        with Locations((0.0, -BODY_OUTER_D / 2.0 - 1.7, BODY_HEIGHT / 2.0)):
            Cylinder(
                radius=LANYARD_HOLE_R,
                height=14.0,
                rotation=(0.0, 90.0, 0.0),
                align=(Align.CENTER, Align.CENTER, Align.CENTER),
                mode=Mode.SUBTRACT,
            )

        # Two M2 screw pilot holes along Y axis
        for sy in (-1.0, 1.0):
            with Locations((0.0, sy * SCREW_OFFSET_R, BODY_HEIGHT - 9.0)):
                Cylinder(
                    radius=M2_PILOT_R,
                    height=9.2,
                    align=(Align.CENTER, Align.CENTER, Align.MIN),
                    mode=Mode.SUBTRACT,
                )

    return part.part


def build_back_cover():
    with BuildPart() as part:
        # Outer disc
        Cylinder(
            radius=(BODY_OUTER_D - 0.4) / 2.0,
            height=COVER_THICK,
            align=(Align.CENTER, Align.CENTER, Align.MIN),
        )
        # Inner alignment / snap lip
        with Locations((0.0, 0.0, COVER_THICK)):
            Cylinder(
                radius=(PCB_POCKET_D + 2.0) / 2.0 - COVER_LIP_CLEARANCE,
                height=COVER_LIP_H - 0.2,
                align=(Align.CENTER, Align.CENTER, Align.MIN),
            )
        # Chamfer outer rear edge
        bottom_edge = part.edges().filter_by(
            lambda e: abs(e.center().Z) < 0.05 and abs(e.radius - (BODY_OUTER_D - 0.4) / 2.0) < 0.2
        )
        if bottom_edge:
            chamfer(bottom_edge, length=0.8)

        # Speaker acoustic slots aligned with rear PCM5101 speaker
        for dy in (-6.0, -2.0, 2.0, 6.0):
            with Locations((0.0, dy, -0.1)):
                Box(
                    18.0,
                    1.8,
                    COVER_THICK + COVER_LIP_H + 0.5,
                    align=(Align.CENTER, Align.CENTER, Align.MIN),
                    mode=Mode.SUBTRACT,
                )

        # M2 through holes + countersinks
        for sy in (-1.0, 1.0):
            with Locations((0.0, sy * SCREW_OFFSET_R, -0.1)):
                Cylinder(
                    radius=M2_THROUGH_R,
                    height=COVER_THICK + COVER_LIP_H + 0.5,
                    align=(Align.CENTER, Align.CENTER, Align.MIN),
                    mode=Mode.SUBTRACT,
                )
            with Locations((0.0, sy * SCREW_OFFSET_R, 0.0)):
                Cylinder(
                    radius=M2_HEAD_R,
                    height=1.2,
                    align=(Align.CENTER, Align.CENTER, Align.MIN),
                    mode=Mode.SUBTRACT,
                )

    return part.part


def main():
    out_dir = Path(__file__).resolve().parent
    body = build_body()
    cover = build_back_cover()

    body_stl = out_dir / "kairos_body.stl"
    cover_stl = out_dir / "kairos_back_cover.stl"
    body_step = out_dir / "kairos_body.step"
    cover_step = out_dir / "kairos_back_cover.step"

    export_stl(body, str(body_stl), tolerance=0.02, angular_tolerance=0.1)
    export_stl(cover, str(cover_stl), tolerance=0.02, angular_tolerance=0.1)
    export_step(body, str(body_step))
    export_step(cover, str(cover_step))

    for p in (body_stl, cover_stl, body_step, cover_step):
        assert p.exists() and p.stat().st_size > 1000, f"Export failed: {p}"
        print(f"Exported {p.name} ({p.stat().st_size} bytes)")

    print("Body bbox:", body.bounding_box())
    print("Cover bbox:", cover.bounding_box())


if __name__ == "__main__":
    main()
