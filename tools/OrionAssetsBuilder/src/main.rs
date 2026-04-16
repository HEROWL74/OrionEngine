use std::env;
use std::fs;
use std::path::Path;
use std::process::Command;

fn main() -> std::io::Result<()> {
    let args: Vec<String> = env::args().collect();
    let get_arg = |key: &str| args.iter().position(|x| x == key).map(|pos| &args[pos + 1]);

    // パスの準備
    let src_root = Path::new(get_arg("--src").expect("Missing --src"));
    let build_dir = Path::new(get_arg("--build-dir").expect("Missing --build-dir"));

    let icon_path = src_root.join("engine-assets/images/OrionEngineIcon.ico");

    // ツール類は build_dir からの相対で解決
    let injector_exe = build_dir.join("tools/OrionResourceInjector.exe");
    let editor_exe = build_dir.join("editor/OrionEditor.exe");

    println!("--- [1/2] Synchronizing Assets ---");
    let asset_dirs = vec!["fonts", "skybox", "images"];
    for dir in asset_dirs {
        let src = src_root.join("engine-assets").join(dir);
        let dst = build_dir.join("editor/engine-assets").join(dir);
        if src.exists() {
            smart_copy_dir(&src, &dst)?;
            println!("  Synced: {}", dir);
        }
    }

    println!("--- [2/2] Injecting Resources ---");
    if injector_exe.exists() && editor_exe.exists() && icon_path.exists() {
        let status = Command::new(injector_exe)
            .arg(&editor_exe)
            .arg(&icon_path)
            .status()?;

        if status.success() {
            println!("Successfully injected icon to OrionEditor.exe");
        }
    }

    println!("--- All Processes Complete ---");
    Ok(())
}

fn smart_copy_dir(src: &Path, dst: &Path) -> std::io::Result<()> {
    if !dst.exists() {
        fs::create_dir_all(dst)?;
    }
    for entry in fs::read_dir(src)? {
        let entry = entry?;
        let ty = entry.file_type()?;
        let dest_path = dst.join(entry.file_name());
        if ty.is_file() {
            fs::copy(entry.path(), &dest_path)?;
        } else if ty.is_dir() {
            smart_copy_dir(&entry.path(), &dest_path)?;
        }
    }
    Ok(())
}
