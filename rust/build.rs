fn main() {
    let root = std::path::PathBuf::from(std::env::var("CARGO_MANIFEST_DIR").unwrap())
        .parent()
        .unwrap()
        .to_path_buf();

    cc::Build::new()
        .files(&[
            root.join("src/bitemu.c"),
            root.join("core/engine.c"),
            root.join("core/timing.c"),
            root.join("core/input.c"),
            root.join("core/utils/log.c"),
            root.join("core/utils/helpers.c"),
            root.join("be1/console.c"),
            root.join("be1/cpu.c"),
            root.join("be1/ppu.c"),
            root.join("be1/apu.c"),
            root.join("be1/timer.c"),
            root.join("be1/memory.c"),
            root.join("be1/input.c"),
        ])
        .include(&root)
        .include(root.join("include"))
        .flag("-Wall")
        .flag("-Wextra")
        .compile("bitemu");
}
