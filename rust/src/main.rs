//! Bitemu - Frontend GUI
//! bitemu -rom juego.gb  → ventana
//! bitemu -rom juego.gb --cli → headless

use minifb::{Key, Scale, ScaleMode, Window, WindowOptions};
use std::env;
use std::process;

const WIDTH: usize = 160;
const HEIGHT: usize = 144;
const SCALE: usize = 4;

#[repr(C)]
struct Bitemu;

#[link(name = "bitemu", kind = "static")]
extern "C" {
    fn bitemu_create() -> *mut Bitemu;
    fn bitemu_destroy(emu: *mut Bitemu);
    fn bitemu_load_rom(emu: *mut Bitemu, path: *const i8) -> bool;
    fn bitemu_run_frame(emu: *mut Bitemu) -> bool;
    fn bitemu_get_framebuffer(emu: *const Bitemu) -> *const u8;
    fn bitemu_set_input(emu: *mut Bitemu, state: u8);
}

fn main() {
    let args: Vec<String> = env::args().collect();
    let mut rom_path: Option<String> = None;
    let mut cli_mode = false;

    let mut i = 1;
    while i < args.len() {
        if args[i] == "-rom" && i + 1 < args.len() {
            rom_path = Some(args[i + 1].clone());
            i += 2;
            continue;
        }
        if args[i] == "--cli" {
            cli_mode = true;
        }
        if args[i] == "-h" || args[i] == "--help" {
            eprintln!("Uso: {} -rom <archivo.gb> [--cli]", args[0]);
            eprintln!("  -rom    Cargar ROM");
            eprintln!("  --cli   Modo headless (sin ventana)");
            process::exit(0);
        }
        i += 1;
    }

    let rom_path = rom_path.unwrap_or_else(|| {
        eprintln!("Error: se requiere -rom <archivo>");
        process::exit(1);
    });

    if !std::path::Path::new(&rom_path).exists() {
        eprintln!("Error: archivo no encontrado: {}", rom_path);
        process::exit(1);
    }

    let emu = unsafe { bitemu_create() };
    if emu.is_null() {
        eprintln!("Error: no se pudo crear emulador");
        process::exit(1);
    }

    let path_c = std::ffi::CString::new(rom_path.as_str()).unwrap();
    if !unsafe { bitemu_load_rom(emu, path_c.as_ptr()) } {
        eprintln!("Error: no se pudo cargar ROM: {}", rom_path);
        unsafe { bitemu_destroy(emu) };
        process::exit(1);
    }

    if cli_mode {
        while unsafe { bitemu_run_frame(emu) } {}
        unsafe { bitemu_destroy(emu) };
        return;
    }

    let mut buffer: Vec<u32> = vec![0; WIDTH * HEIGHT];

    let mut window = Window::new(
        "Bitemu - Game Boy",
        WIDTH * SCALE,
        HEIGHT * SCALE,
        WindowOptions {
            scale: Scale::X4,
            scale_mode: ScaleMode::Stretch,
            ..WindowOptions::default()
        },
    )
    .unwrap_or_else(|e| {
        eprintln!("Error al crear ventana: {}", e);
        unsafe { bitemu_destroy(emu) };
        process::exit(1);
    });

    while window.is_open() && !window.is_key_down(Key::Escape) {
        let mut joypad: u8 = 0xFF;
        if window.is_key_down(Key::W) {
            joypad &= !(1 << 2);
        }
        if window.is_key_down(Key::S) {
            joypad &= !(1 << 3);
        }
        if window.is_key_down(Key::A) {
            joypad &= !(1 << 1);
        }
        if window.is_key_down(Key::D) {
            joypad &= !(1 << 0);
        }
        if window.is_key_down(Key::J) {
            joypad &= !(1 << 4);
        }
        if window.is_key_down(Key::K) {
            joypad &= !(1 << 5);
        }
        if window.is_key_down(Key::U) {
            joypad &= !(1 << 6);
        }
        if window.is_key_down(Key::I) {
            joypad &= !(1 << 7);
        }
        unsafe { bitemu_set_input(emu, joypad) };

        if !unsafe { bitemu_run_frame(emu) } {
            break;
        }

        let fb = unsafe { bitemu_get_framebuffer(emu) };
        if !fb.is_null() {
            for i in 0..(WIDTH * HEIGHT) {
                let g = unsafe { *fb.add(i) };
                buffer[i] = (g as u32) << 16 | (g as u32) << 8 | (g as u32);
            }
        }

        window.update_with_buffer(&buffer, WIDTH, HEIGHT).unwrap();
    }

    unsafe { bitemu_destroy(emu) };
}
