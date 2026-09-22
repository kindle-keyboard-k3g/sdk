import shutil
from pathlib import Path

TEMPLATES = ["pure-kindlet", "native-process-kindlet", "standalone-native"]

class ProjectInitError(Exception):
    pass

def init_project(target_dir: Path, template_name: str, app_name: str) -> None:
    if template_name not in TEMPLATES:
        raise ProjectInitError(f"Unknown template: {template_name}. Available: {TEMPLATES}")

    target_dir = Path(target_dir)
    if target_dir.exists() and any(target_dir.iterdir()):
        raise ProjectInitError(f"Target directory is not empty: {target_dir}")

    # Look for templates relative to sdk root
    base = Path(__file__).resolve().parent.parent.parent.parent
    tmpl_dir = base / "templates" / template_name
    if not tmpl_dir.exists():
        tmpl_dir = Path("templates") / template_name

    target_dir.mkdir(parents=True, exist_ok=True)
    for item in tmpl_dir.iterdir():
        if item.is_dir():
            shutil.copytree(item, target_dir / item.name)
        else:
            shutil.copy2(item, target_dir / item.name)
