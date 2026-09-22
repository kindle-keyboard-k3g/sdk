"""
Starter template scaffolding and project initialization.

Copies starter boilerplate templates for Pure Kindlets, Native Process Kindlets,
and Standalone Native daemons into new project directories.
"""

import shutil
from pathlib import Path

TEMPLATES = ["pure-kindlet", "native-process-kindlet", "standalone-native"]

class ProjectInitError(Exception):
    """Raised when project scaffolding fails due to invalid parameters or destination state."""
    pass

def init_project(target_dir: Path, template_name: str, app_name: str) -> None:
    """
    Initializes a new Kindle application project directory using a designated starter template.

    Args:
        target_dir: Destination path for the new project.
        template_name: Name of starter template ('pure-kindlet', 'native-process-kindlet', 'standalone-native').
        app_name: Name of the application to scaffold.

    Raises:
        ProjectInitError: If template_name is not recognized or target_dir is non-empty.
    """
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
