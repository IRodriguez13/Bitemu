//! Bitemu - Cliente de escritorio (estilo PCSX2)
//!
//! - Arrancar con o sin ROM: `bitemu` (ventana vacía) o `bitemu -rom juego.gb`
//! - Ctrl+O: Abrir ROM (diálogo de archivo)
//! - F3: Pausa / Reanudar
//! - F4: Reset consola
//! - Escape / Ctrl+Q: Salir
//! - Título de ventana: nombre del juego o "Bitemu - Game Boy"
//! - Últimas ROMs guardadas en directorio de configuración del SO (hasta 10)
//!   (Linux: ~/.config/bitemu, macOS: ~/Library/Application Support/bitemu, Windows: %APPDATA%\bitemu)
//!
//! Convención joypad hacia C: 1 = pulsado, 0 = soltado (invertimos respecto a teclas).

use minifb::{Key, Scale, ScaleMode, Window, WindowOptions};
use std::env;
use std::ffi::{c_void, CString};
use std::fs;
use std::io::{BufRead, BufReader, Write};
use std::path::Path;

const WIDTH: usize = 160;
const HEIGHT: usize = 144;
/** Escala de ventana: 4x = 640×576 (tamaño cómodo; el juego rellena la ventana con Stretch). */
const SCALE: usize = 4;
const MAX_RECENT: usize = 10;

const JOYPAD_BIT_RIGHT: u8 = 0;
const JOYPAD_BIT_LEFT: u8 = 1;
const JOYPAD_BIT_UP: u8 = 2;
const JOYPAD_BIT_DOWN: u8 = 3;
const JOYPAD_BIT_A: u8 = 4;
const JOYPAD_BIT_B: u8 = 5;
const JOYPAD_BIT_SELECT: u8 = 6;
const JOYPAD_BIT_START: u8 = 7;

#[link(name = "bitemu", kind = "static")]
extern "C" {
    fn bitemu_create() -> *mut c_void;
    fn bitemu_destroy(emu: *mut c_void);
    fn bitemu_load_rom(emu: *mut c_void, path: *const i8) -> bool;
    fn bitemu_run_frame(emu: *mut c_void) -> bool;
    fn bitemu_get_framebuffer(emu: *const c_void) -> *const u8;
    fn bitemu_set_input(emu: *mut c_void, state: u8);
    fn bitemu_reset(emu: *mut c_void);
}

fn recent_path() -> std::path::PathBuf {
    directories::ProjectDirs::from("com", "bitemu", "bitemu")
        .map(|d| {
            let dir = d.config_dir();
            let _ = fs::create_dir_all(dir);
            dir.join("recent")
        })
        .unwrap_or_else(|| std::path::PathBuf::from("recent"))
}

fn load_recent() -> Vec<String> {
    let path = recent_path();
    let f = match fs::File::open(&path) {
        Ok(f) => f,
        Err(_) => return Vec::new(),
    };
    BufReader::new(f)
        .lines()
        .filter_map(|l| l.ok())
        .filter(|l| !l.trim().is_empty() && Path::new(l.trim()).exists())
        .take(MAX_RECENT)
        .collect()
}

fn save_recent(list: &[String]) {
    let path = recent_path();
    if let Ok(mut f) = fs::File::create(&path) {
        for p in list.iter().take(MAX_RECENT) {
            let _ = writeln!(f, "{}", p);
        }
    }
}

fn add_to_recent(list: &mut Vec<String>, path: &str) {
    list.retain(|p| p != path);
    list.insert(0, path.to_string());
    if list.len() > MAX_RECENT {
        list.truncate(MAX_RECENT);
    }
    save_recent(list);
}

fn rom_title(path: &str) -> String {
    Path::new(path)
        .file_name()
        .and_then(|n| n.to_str())
        .unwrap_or(path)
        .to_string()
}

fn open_rom_dialog() -> Option<String> {
    rfd::FileDialog::new()
        .add_filter("ROMs Game Boy", &["gb", "gbc"])
        .set_title("Abrir ROM")
        .pick_file()
        .and_then(|p| p.into_os_string().into_string().ok())
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
            eprintln!("Uso: {} [-rom <archivo.gb>] [--cli]", args[0]);
            eprintln!("  -rom    Cargar ROM al iniciar (opcional en GUI)");
            eprintln!("  --cli   Modo headless (requiere -rom)");
            eprintln!();
            eprintln!("GUI: Ctrl+O Abrir ROM | F3 Pausa | F4 Reset | Escape Salir");
            std::process::exit(0);
        }
        i += 1;
    }

    if cli_mode {
        let path = rom_path.unwrap_or_else(|| {
            eprintln!("Error: --cli requiere -rom <archivo>");
            std::process::exit(1);
        });
        if !Path::new(&path).exists() {
            eprintln!("Error: archivo no encontrado: {}", path);
            std::process::exit(1);
        }
        let emu = unsafe { bitemu_create() };
        if emu.is_null() {
            eprintln!("Error: no se pudo crear emulador");
            std::process::exit(1);
        }
        let path_c = CString::new(path.as_str()).unwrap();
        if !unsafe { bitemu_load_rom(emu, path_c.as_ptr()) } {
            eprintln!("Error: no se pudo cargar ROM: {}", path);
            unsafe { bitemu_destroy(emu) };
            std::process::exit(1);
        }
        while unsafe { bitemu_run_frame(emu) } {}
        unsafe { bitemu_destroy(emu) };
        return;
    }

    // GUI: ventana 4x (640×576); Stretch para que el juego llene toda la ventana
    let win_width = WIDTH * SCALE;
    let win_height = HEIGHT * SCALE;
    let mut window = Window::new(
        "Bitemu - Game Boy",
        win_width,
        win_height,
        WindowOptions {
            scale: Scale::X4,
            scale_mode: ScaleMode::Stretch,
            ..WindowOptions::default()
        },
    )
    .unwrap_or_else(|e| {
        eprintln!("Error al crear ventana: {}", e);
        std::process::exit(1);
    });
    // Centrar ventana en pantalla (heurística 1920x1080)
    let (sw, sh) = (1920isize, 1080isize);
    window.set_position((sw - win_width as isize).max(0) / 2, (sh - win_height as isize).max(0) / 2);

    let mut buffer: Vec<u32> = vec![0; WIDTH * HEIGHT];
    let mut emu: *mut c_void = std::ptr::null_mut();
    let mut current_rom: Option<String> = rom_path.clone();
    let mut paused = false;
    let mut recent = load_recent();
    let mut prev_f3 = false;
    let mut prev_f4 = false;

    // Si se pasó -rom al arrancar, cargar
    if let Some(ref path) = current_rom {
        if Path::new(path).exists() {
            emu = unsafe { bitemu_create() };
            if !emu.is_null() {
                let path_c = CString::new(path.as_str()).unwrap();
                if unsafe { bitemu_load_rom(emu, path_c.as_ptr()) } {
                    add_to_recent(&mut recent, path);
                    window.set_title(&format!("Bitemu - {}", rom_title(path)));
                } else {
                    unsafe { bitemu_destroy(emu) };
                    emu = std::ptr::null_mut();
                    current_rom = None;
                }
            }
        }
    }

    while window.is_open() {
        let ctrl = window.is_key_down(Key::LeftCtrl) || window.is_key_down(Key::RightCtrl);

        if ctrl && window.is_key_down(Key::Q) {
            break;
        }
        if ctrl && window.is_key_down(Key::O) {
            if let Some(path) = open_rom_dialog() {
                if !path.is_empty() && Path::new(&path).exists() {
                    if !emu.is_null() {
                        unsafe { bitemu_destroy(emu) };
                    }
                    emu = unsafe { bitemu_create() };
                    if !emu.is_null() {
                        let path_c = CString::new(path.as_str()).unwrap();
                        if unsafe { bitemu_load_rom(emu, path_c.as_ptr()) } {
                            current_rom = Some(path.clone());
                            add_to_recent(&mut recent, &path);
                            window.set_title(&format!("Bitemu - {}", rom_title(&path)));
                            paused = false;
                        } else {
                            unsafe { bitemu_destroy(emu) };
                            emu = std::ptr::null_mut();
                            current_rom = None;
                        }
                    }
                }
            }
        }

        if !emu.is_null() {
            let f3 = window.is_key_down(Key::F3);
            if f3 && !prev_f3 {
                paused = !paused;
                let title = current_rom.as_ref().map(|s| rom_title(s.as_str())).unwrap_or_else(|| "Game Boy".into());
                window.set_title(&format!(
                    "Bitemu - {} {}",
                    title,
                    if paused { "[Pausado]" } else { "" }
                ));
            }
            prev_f3 = f3;

            let f4 = window.is_key_down(Key::F4);
            if f4 && !prev_f4 {
                unsafe { bitemu_reset(emu) };
            }
            prev_f4 = f4;

            if !paused {
                /* Estado para C: 1 = pulsado. Actualizamos input cada frame. */
                let mut joypad = 0u8;
                if window.is_key_down(Key::W) || window.is_key_down(Key::Up) {
                    joypad |= 1 << JOYPAD_BIT_UP;
                }
                if window.is_key_down(Key::S) || window.is_key_down(Key::Down) {
                    joypad |= 1 << JOYPAD_BIT_DOWN;
                }
                if window.is_key_down(Key::A) || window.is_key_down(Key::Left) {
                    joypad |= 1 << JOYPAD_BIT_LEFT;
                }
                if window.is_key_down(Key::D) || window.is_key_down(Key::Right) {
                    joypad |= 1 << JOYPAD_BIT_RIGHT;
                }
                if window.is_key_down(Key::J) || window.is_key_down(Key::Z) {
                    joypad |= 1 << JOYPAD_BIT_A;
                }
                if window.is_key_down(Key::K) || window.is_key_down(Key::X) {
                    joypad |= 1 << JOYPAD_BIT_B;
                }
                if window.is_key_down(Key::U) {
                    joypad |= 1 << JOYPAD_BIT_SELECT;
                }
                if window.is_key_down(Key::I) || window.is_key_down(Key::Enter) {
                    joypad |= 1 << JOYPAD_BIT_START;
                }
                unsafe { bitemu_set_input(emu, joypad) };
                unsafe { bitemu_run_frame(emu) };
            }

            let fb = unsafe { bitemu_get_framebuffer(emu) };
            if !fb.is_null() {
                for i in 0..(WIDTH * HEIGHT) {
                    let g = unsafe { *fb.add(i) };
                    buffer[i] = (g as u32) << 16 | (g as u32) << 8 | (g as u32);
                }
            }
        } else {
            window.set_title("Bitemu - Game Boy  [Ctrl+O Abrir ROM]");
        }

        if window.is_key_down(Key::Escape) {
            break;
        }

        window.update_with_buffer(&buffer, WIDTH, HEIGHT).unwrap();
    }

    if !emu.is_null() {
        unsafe { bitemu_destroy(emu) };
    }
}
