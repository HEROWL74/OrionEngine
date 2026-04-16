// tools/OrionBuild/src/main.rs

use std::env;
use std::fs;
use std::path::{Path, PathBuf};
use std::process;
use std::time::{SystemTime, UNIX_EPOCH};

// =========================================================
// エントリポイント
// =========================================================

fn main() {
    let args: Vec<String> = env::args().collect();
    let subcommand = args.get(1).map(|s| s.as_str()).unwrap_or("");

    for (i, arg) in args.iter().enumerate() {
        println!("[DEBUG_ARGS] arg[{}]: |{}|", i, arg);
    }
    match subcommand {
        "build" => {
            let opts = BuildOptions::from_args(&args[2..]);
            match run_build(opts) {
                Ok(()) => process::exit(0),
                Err(e) => {
                    log_line("ERROR", &e);
                    process::exit(1);
                }
            }
        }
        _ => {
            eprintln!("Usage: OrionBuild <command> [options]");
            eprintln!("Commands:");
            eprintln!("  build    Build and package the project");
            eprintln!("Options:");
            eprintln!("  --project-root <path>    Path to the project root directory");
            eprintln!("  --editor-root  <path>    Path to the editor root directory");
            eprintln!("  --project-name <name>    Project name");
            eprintln!("  --config       <config>  Build configuration (Debug/Release)");
            eprintln!("  --force                  Force rebuild even if cache exists");
            process::exit(1);
        }
    }
}

// =========================================================
// コマンドライン引数
// =========================================================

struct BuildOptions {
    project_root: Option<PathBuf>,
    editor_root: Option<PathBuf>,
    project_name: Option<String>,
    config: String,
    force_rebuild: bool,
}

impl BuildOptions {
    fn from_args(args: &[String]) -> Self {
        let mut opts = BuildOptions {
            project_root: None,
            editor_root: None,
            project_name: None,
            config: "Release".to_string(),
            force_rebuild: false,
        };

        let mut i = 0;
        while i < args.len() {
            match args[i].as_str() {
                "--project-root" => {
                    i += 1;
                    if let Some(v) = args.get(i) {
                        opts.project_root = Some(PathBuf::from(v));
                    }
                }
                "--editor-root" => {
                    i += 1;
                    if let Some(v) = args.get(i) {
                        opts.editor_root = Some(PathBuf::from(v));
                    }
                }
                "--project-name" => {
                    i += 1;
                    if let Some(v) = args.get(i) {
                        opts.project_name = Some(v.clone());
                    }
                }
                "--config" => {
                    i += 1;
                    if let Some(v) = args.get(i) {
                        opts.config = v.clone();
                    }
                }
                "--force" => {
                    opts.force_rebuild = true;
                }
                _ => {}
            }
            i += 1;
        }

        opts
    }
}

// =========================================================
// ログ出力
// C++ 側の writeLog と同じ形式: [HH:MM:SS] [LEVEL] message
// RunCommandWithOutput がそのままキャプチャしてログに流す
// =========================================================

fn now_hms() -> String {
    // std のみで HH:MM:SS を組み立てる（chrono クレート不使用）
    let secs = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs();
    let s = secs % 60;
    let m = (secs / 60) % 60;
    let h = (secs / 3600) % 24;
    format!("{:02}:{:02}:{:02}", h, m, s)
}

fn log_line(level: &str, message: &str) {
    // stdout に書く → C++ の RunCommandWithOutput がキャプチャする
    println!("[{}] [{}] {}", now_hms(), level, message);
}

fn log_progress(progress_pct: u32, message: &str) {
    log_line("INFO", &format!("[{}%] {}", progress_pct, message));
}

// =========================================================
// パス解決ユーティリティ
// =========================================================

/// 引数で渡されなかった場合のフォールバック:
/// exe から上へ辿って CMakeLists.txt があるディレクトリをソースルートとする
fn find_source_root() -> Option<PathBuf> {
    let exe = env::current_exe().ok()?;
    let mut current = exe.parent()?.to_path_buf();
    loop {
        if current.join("CMakeLists.txt").exists() {
            return Some(current);
        }
        if !current.pop() {
            return None;
        }
    }
}

/// 引数で渡されなかった場合のフォールバック:
/// exe から上へ辿って "editor" フォルダを探す
fn find_editor_root() -> Option<PathBuf> {
    let exe = env::current_exe().ok()?;
    let mut current = exe.parent()?.to_path_buf();
    loop {
        if current.file_name().and_then(|n| n.to_str()) == Some("editor") {
            return Some(current.clone());
        }
        // exeDir 直下にリソースフォルダがあればここがルート
        if current.join("player").exists() || current.join("engine-assets").exists() {
            return Some(current.clone());
        }
        if !current.pop() {
            return None;
        }
    }
}

// =========================================================
// ビルドコンテキスト（解決済みパスをまとめて持つ）
// =========================================================

struct BuildContext {
    project_root: PathBuf,
    editor_root: PathBuf,
    project_name: String,
    config: String,
    force_rebuild: bool,
}

impl BuildContext {
    fn dist_output_dir(&self) -> PathBuf {
        self.project_root.join("dist").join(&self.project_name)
    }

    fn player_dir(&self) -> PathBuf {
        self.editor_root.join("player")
    }

    /// エディタ exe と同階層（DLL が置かれる場所）
    fn editor_exe_dir(&self) -> PathBuf {
        // VS Multi-Config は editor/Release/ または editor/Debug/
        // Ninja は editor/ 直下
        let sub = self.editor_root.join(&self.config);
        if sub.exists() {
            sub
        } else {
            self.editor_root.clone()
        }
    }
}

fn resolve_context(opts: BuildOptions) -> Result<BuildContext, String> {
    let project_root = match opts.project_root {
        Some(p) => p,
        None => find_source_root()
            .map(|r| r.join("project"))
            .ok_or("Could not locate project root. Pass --project-root.")?,
    };

    let editor_root = match opts.editor_root {
        Some(p) => p,
        None => find_editor_root().ok_or("Could not locate editor root. Pass --editor-root.")?,
    };

    let project_name = opts.project_name.unwrap_or_else(|| "OrionGame".to_string());

    Ok(BuildContext {
        project_root,
        editor_root,
        project_name,
        config: opts.config,
        force_rebuild: opts.force_rebuild,
    })
}

// =========================================================
// ビルドステップ
// =========================================================

/// Step ①: dist/{ProjectName}/ を作成
fn prepare_output_directory(ctx: &BuildContext) -> Result<(), String> {
    let out = ctx.dist_output_dir();

    if out.exists() {
        fs::remove_dir_all(&out)
            .map_err(|e| format!("Failed to clean output dir {}: {}", out.display(), e))?;
        log_line(
            "DEBUG",
            &format!("Cleaned previous output: {}", out.display()),
        );
    }

    fs::create_dir_all(&out)
        .map_err(|e| format!("Failed to create output dir {}: {}", out.display(), e))?;

    log_line(
        "DEBUG",
        &format!("DistOutputDir created: {}", out.display()),
    );
    Ok(())
}

/// Step ②: project/Assets/ → dist/{ProjectName}/Assets/
fn copy_assets(ctx: &BuildContext) -> Result<(), String> {
    let src = ctx.project_root.join("Assets");
    let dst = ctx.dist_output_dir().join("Assets");

    log_line("DEBUG", &format!("Assets src: {}", src.display()));
    log_line("DEBUG", &format!("Assets dst: {}", dst.display()));

    if !src.exists() {
        log_line(
            "WARN",
            &format!("Warning: Asset folder not found: {}", src.display()),
        );
        return Ok(()); // Assets がなくてもビルド続行
    }

    if dst.exists() {
        fs::remove_dir_all(&dst).map_err(|e| format!("Failed to clean asset output dir: {}", e))?;
        log_line("DEBUG", &format!("Removed stale assets: {}", dst.display()));
    }

    copy_directory(&src, &dst)
}

/// Step ③: ProjectSettings.json のテンプレート置換してコピー
fn copy_project_settings(ctx: &BuildContext) -> Result<(), String> {
    let src = ctx.project_root.join("ProjectSettings.json");
    let dst = ctx
        .dist_output_dir()
        .join("Assets")
        .join("ProjectSettings.json");

    log_line("DEBUG", &format!("ProjectSettings src: {}", src.display()));
    log_line("DEBUG", &format!("ProjectSettings dst: {}", dst.display()));

    if !src.exists() {
        log_line(
            "WARN",
            &format!("Warning: ProjectSettings.json not found: {}", src.display()),
        );
        return Ok(());
    }

    let content =
        fs::read_to_string(&src).map_err(|e| format!("Cannot read ProjectSettings.json: {}", e))?;

    let content = content
        .replace("__PROJECT_NAME__", &ctx.project_name)
        .replace("__ENGINE_VERSION__", "0.0.0");

    if let Some(parent) = dst.parent() {
        fs::create_dir_all(parent)
            .map_err(|e| format!("Failed to create dir for ProjectSettings: {}", e))?;
    }

    fs::write(&dst, content).map_err(|e| format!("Cannot write ProjectSettings.json: {}", e))?;

    log_line(
        "DEBUG",
        &format!("ProjectSettings written: ProjectName={}", ctx.project_name),
    );
    Ok(())
}

/// Step ④: engine-assets/ のコピー
fn copy_engine_assets(ctx: &BuildContext) -> Result<(), String> {
    // engine-assets は editor_root の隣に置かれる想定
    let src = ctx.editor_root.join("engine-assets");
    let dst = ctx.dist_output_dir().join("engine-assets");

    log_line("DEBUG", &format!("Engine assets src: {}", src.display()));
    log_line("DEBUG", &format!("Engine assets dst: {}", dst.display()));

    if !src.exists() {
        log_line(
            "WARN",
            &format!("Warning: engine-assets not found at: {}", src.display()),
        );
        return Ok(());
    }

    copy_directory(&src, &dst)
}

/// Step ⑤⑥: exe/DLL/依存DLLのコピー
fn copy_player_executable(ctx: &BuildContext) -> Result<(), String> {
    let player_dir = ctx.player_dir();
    let exe_dir = ctx.editor_exe_dir();
    let out_dir = ctx.dist_output_dir();

    // ---- launcher: OrionGame.exe → {ProjectName}.exe にリネームしてコピー ----
    let launcher_src = player_dir.join("OrionGame.exe");
    if !launcher_src.exists() {
        return Err(format!("Launcher not found: {}", launcher_src.display()));
    }

    let dst_exe_name = format!("{}.exe", ctx.project_name);
    fs::create_dir_all(&out_dir).map_err(|e| format!("Failed to create output dir: {}", e))?;

    fs::copy(&launcher_src, out_dir.join(&dst_exe_name))
        .map_err(|e| format!("Failed to copy launcher: {}", e))?;
    log_line(
        "DEBUG",
        &format!("Launcher: OrionGame.exe -> {}", dst_exe_name),
    );

    // ---- OrionRuntime.dll: exe の真横から固定名でコピー ----
    let game_dll_src = exe_dir.join("OrionRuntime.dll");
    if !game_dll_src.exists() {
        return Err(format!(
            "Game DLL not found at exe root: {}",
            game_dll_src.display()
        ));
    }

    fs::copy(&game_dll_src, out_dir.join("OrionRuntime.dll"))
        .map_err(|e| format!("Failed to copy game DLL: {}", e))?;
    log_line("DEBUG", "GameDLL copied from exe root to dist.");

    // ---- 依存DLLのコピー ----
    copy_dependency_dlls(ctx, &out_dir)?;

    Ok(())
}

/// Step ⑦: キャッシュチェック
fn prepare_player_cache(ctx: &BuildContext) -> Result<(), String> {
    let has_launcher = ctx.player_dir().join("OrionGame.exe").exists();
    let has_game_dll = ctx.editor_exe_dir().join("OrionRuntime.dll").exists();

    if !ctx.force_rebuild && has_launcher && has_game_dll {
        log_line(
            "INFO",
            "Cache exists (Launcher in player/, DLL in exe root), skipping cmake build.",
        );
        return Ok(());
    }

    Err(
        "Missing OrionGame.exe (in player/) or OrionRuntime.dll (in exe root).\n\
         Please check your installation."
            .to_string(),
    )
}

// =========================================================
// 依存DLLコピー
// =========================================================

fn copy_dependency_dlls(ctx: &BuildContext, output_dir: &Path) -> Result<(), String> {
    let project_dll = format!("{}.dll", ctx.project_name);

    let skip = |fname: &str| -> bool { fname == project_dll || fname == "OrionRuntime.dll" };

    // player/ と editor_exe_dir/ から収集
    for src_dir in [ctx.player_dir(), ctx.editor_exe_dir()] {
        copy_dlls_from(&src_dir, output_dir, &skip)?;
    }

    Ok(())
}

fn copy_dlls_from(
    src_dir: &Path,
    output_dir: &Path,
    skip: &impl Fn(&str) -> bool,
) -> Result<(), String> {
    if !src_dir.exists() {
        return Ok(());
    }

    let entries = fs::read_dir(src_dir)
        .map_err(|e| format!("Failed to read dir {}: {}", src_dir.display(), e))?;

    for entry in entries.flatten() {
        let path = entry.path();
        if path.extension().and_then(|e| e.to_str()) != Some("dll") {
            continue;
        }
        let fname = path.file_name().and_then(|n| n.to_str()).unwrap_or("");
        if skip(fname) {
            continue;
        }
        fs::copy(&path, output_dir.join(fname))
            .map_err(|e| format!("Failed to copy DLL {}: {}", fname, e))?;
        log_line("DEBUG", &format!("DLL: {}", fname));
    }

    Ok(())
}

// =========================================================
// ディレクトリ再帰コピー
// =========================================================

fn copy_directory(src: &Path, dst: &Path) -> Result<(), String> {
    if !src.exists() {
        log_line("DEBUG", &format!("Skipping (not found): {}", src.display()));
        return Ok(());
    }

    fs::create_dir_all(dst)
        .map_err(|e| format!("Failed to create dir {}: {}", dst.display(), e))?;

    let mut count = 0usize;
    copy_dir_recursive(src, dst, src, &mut count)?;

    log_line(
        "DEBUG",
        &format!(
            "Copied {} files: {} -> {}",
            count,
            src.display(),
            dst.display()
        ),
    );
    Ok(())
}

fn copy_dir_recursive(
    base_src: &Path,
    base_dst: &Path,
    current: &Path,
    count: &mut usize,
) -> Result<(), String> {
    for entry in fs::read_dir(current)
        .map_err(|e| format!("Failed to read dir {}: {}", current.display(), e))?
    {
        let entry = entry.map_err(|e| format!("Dir entry error: {}", e))?;
        let src_path = entry.path();
        let rel = src_path.strip_prefix(base_src).unwrap();
        let dst_path = base_dst.join(rel);

        if src_path.is_dir() {
            fs::create_dir_all(&dst_path)
                .map_err(|e| format!("Failed to create dir {}: {}", dst_path.display(), e))?;
            copy_dir_recursive(base_src, base_dst, &src_path, count)?;
        } else {
            fs::copy(&src_path, &dst_path)
                .map_err(|e| format!("Failed to copy {}: {}", src_path.display(), e))?;
            *count += 1;
        }
    }
    Ok(())
}

// =========================================================
// ビルド実行（全ステップ）
// =========================================================

fn run_build(opts: BuildOptions) -> Result<(), String> {
    let ctx = resolve_context(opts)?;

    log_line(
        "INFO",
        &format!("ProjectRoot  : {}", ctx.project_root.display()),
    );
    log_line(
        "INFO",
        &format!("EditorRoot   : {}", ctx.editor_root.display()),
    );
    log_line(
        "INFO",
        &format!("PlayerDir    : {}", ctx.player_dir().display()),
    );
    log_line(
        "INFO",
        &format!("DistOutput   : {}", ctx.dist_output_dir().display()),
    );
    log_line("INFO", &format!("ProjectName  : {}", ctx.project_name));
    log_line("INFO", &format!("Config       : {}", ctx.config));
    log_line("INFO", &format!("ForceRebuild : {}", ctx.force_rebuild));

    // ステップ実行マクロ的ヘルパー
    macro_rules! step {
        ($pct:expr, $msg:expr, $fn:expr) => {{
            log_progress($pct, $msg);
            $fn.map_err(|e: String| format!("{}: {}", $msg, e))?;
        }};
    }

    step!(
        0,
        "Preparing output directory...",
        prepare_output_directory(&ctx)
    );
    step!(10, "Checking player cache...", prepare_player_cache(&ctx));
    step!(
        50,
        "Copying player executable...",
        copy_player_executable(&ctx)
    );
    step!(65, "Copying assets...", copy_assets(&ctx));
    step!(
        80,
        "Copying ProjectSettings...",
        copy_project_settings(&ctx)
    );
    step!(90, "Copying engine assets...", copy_engine_assets(&ctx));

    log_progress(
        100,
        &format!(
            "Build completed! Output: {}",
            ctx.dist_output_dir().display()
        ),
    );
    Ok(())
}
