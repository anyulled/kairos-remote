// Kairos-remote — Parametric 3D Enclosure (OpenSCAD source)
// Target hardware: Waveshare ESP32-S3-Touch-LCD-1.46 + 3.7V LiPo (350-500 mAh)
// Print orientation: front bezel down (body) / outer face down (back cover), no supports needed.

$fn = 96;

// --- Parametric Dimensions (mm) ---
body_outer_d     = 54.0;
body_height      = 23.0;
wall_thickness   = 2.6;

pcb_pocket_d     = 45.4;  // 44.77 mm cover glass + 0.63 mm radial clearance
pcb_pocket_depth = 13.0;  // 12.30 mm board stack + clearance
screen_window_d  = 38.4;  // Active 1.46" display (37.1 mm) + chamfer reveal
front_lip_h      = 1.8;

lipo_len         = 38.5;  // Fits 502535 / 602535 350-500 mAh LiPo cell
lipo_wid         = 28.5;
lipo_depth       = 8.0;

cover_thickness  = 2.2;
lip_height       = 2.0;
lip_clearance    = 0.25;

// Side cutouts (USB-C & physical PWR/BOOT button access)
usbc_w           = 11.5;
usbc_h           = 5.5;
btn_slot_w       = 10.0;
btn_slot_h       = 4.5;

// M2 brass/self-tapping screw bosses (180 deg apart outside PCB pocket)
screw_offset_r   = 24.4;
m2_pilot_d       = 1.75;
m2_through_d     = 2.25;
m2_head_d        = 4.2;

// Lanyard loop & speaker acoustic slots
lanyard_d        = 3.6;

module rounded_cylinder(d, h, r_edge) {
    hull() {
        translate([0, 0, r_edge])
            rotate_extrude()
                translate([d/2 - r_edge, 0, 0])
                    circle(r = r_edge);
        translate([0, 0, h - r_edge])
            rotate_extrude()
                translate([d/2 - r_edge, 0, 0])
                    circle(r = r_edge);
    }
}

module main_body() {
    difference() {
        union() {
            rounded_cylinder(body_outer_d, body_height, 3.0);
            // Integrated lanyard ear at bottom (-Y)
            translate([0, -body_outer_d/2 - 2.0, body_height/2])
                rotate([0, 90, 0])
                    cylinder(d = 9.0, h = 8.0, center = true);
        }

        // Front circular window for 1.46" display (print face Z=0)
        translate([0, 0, -0.1])
            cylinder(d1 = screen_window_d + 1.6, d2 = screen_window_d, h = front_lip_h + 0.2);

        // Waveshare ESP32-S3-Touch-LCD-1.46 main pocket
        translate([0, 0, front_lip_h])
            cylinder(d = pcb_pocket_d, h = pcb_pocket_depth + 0.1);

        // Isolated lower LiPo compartment with 1.4 mm separator floor (printed via bridge/shelves)
        translate([0, 0, front_lip_h + pcb_pocket_depth + 1.4])
            cube([lipo_len, lipo_wid, lipo_depth + 2.0], center = true);

        // JST 1.25 mm wire pass-through slot between PCB pocket and LiPo bay
        translate([0, 12.0, front_lip_h + pcb_pocket_depth - 0.5])
            cube([8.0, 5.0, 4.0], center = true);

        // Rear cover recess
        translate([0, 0, body_height - lip_height])
            cylinder(d = pcb_pocket_d + 2.0, h = lip_height + 0.2);

        // USB-C port cutout (+X side)
        translate([body_outer_d/2 - 4.0, 0, front_lip_h + 9.2])
            cube([10.0, usbc_w, usbc_h], center = true);

        // Physical PWR/BOOT button access (-X side)
        translate([-body_outer_d/2 + 4.0, 0, front_lip_h + 9.2])
            cube([10.0, btn_slot_w, btn_slot_h], center = true);

        // Lanyard hole
        translate([0, -body_outer_d/2 - 2.2, body_height/2])
            rotate([0, 90, 0])
                cylinder(d = lanyard_d, h = 14.0, center = true);

        // M2 mounting holes
        for (sx = [-1, 1]) {
            translate([0, sx * screw_offset_r, body_height - 9.0])
                cylinder(d = m2_pilot_d, h = 9.5);
        }
    }
}

module back_cover() {
    difference() {
        union() {
            cylinder(d = body_outer_d - 0.4, h = cover_thickness);
            translate([0, 0, cover_thickness])
                cylinder(d = pcb_pocket_d + 2.0 - 2*lip_clearance, h = lip_height - 0.2);
        }

        // Acoustic speaker slots aligned with onboard PCM5101 speaker
        for (dy = [-6, -2, 2, 6]) {
            translate([0, dy, -0.1])
                cube([18.0, 1.8, cover_thickness + lip_height + 1.0], center = true);
        }

        // M2 countersunk screw holes
        for (sy = [-1, 1]) {
            translate([0, sy * screw_offset_r, -0.1]) {
                cylinder(d = m2_through_d, h = cover_thickness + lip_height + 1.0);
                cylinder(d1 = m2_head_d, d2 = m2_through_d, h = 1.4);
            }
        }
    }
}

// Render both parts side by side
main_body();
translate([body_outer_d + 10.0, 0, 0])
    back_cover();
