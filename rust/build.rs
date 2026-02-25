fn main() {
    let root = std::path::PathBuf::from(std::env::var("CARGO_MANIFEST_DIR").unwrap())
        .parent()
        .unwrap()
        .to_path_buf();

    let mut build = cc::Build::new();
    build
        .files(&[
            root.join("src/bitemu.c"),
            root.join("core/engine.c"),
            root.join("core/timing.c"),
            root.join("core/input.c"),
            root.join("core/utils/log.c"),
            root.join("core/utils/helpers.c"),
            root.join("be1/console.c"),
            root.join("be1/cpu/cpu.c"),
            root.join("be1/cpu/cpu_handlers.c"),
            root.join("be1/ppu.c"),
            root.join("be1/apu.c"),
            root.join("be1/timer.c"),
            root.join("be1/memory.c"),
            root.join("be1/input.c"),
        ])
        .include(&root)
        .include(root.join("include"));

    if !cfg!(target_env = "msvc") {
        build.flag("-Wall").flag("-Wextra");
    }

    build.compile("bitemu");
}
