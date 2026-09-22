import json
from dataclasses import dataclass
from pathlib import Path
from typing import List, Dict, Any

class ProfileValidationError(Exception):
    """Raised when target profile fails validation against schema."""
    pass

@dataclass(frozen=True)
class DisplayProfile:
    width: int
    height: int
    formats: List[str]
    ppi: int = 167
    eink_panel: str = "Pearl"

@dataclass(frozen=True)
class TargetProfile:
    name: str
    model_name: str
    soc: str
    cpu: str
    native_abi: str
    ram_bytes: int
    linux_kernel: str
    glibc_version: str
    display: DisplayProfile
    ioctls: Dict[str, str]

def _get_profiles_dir() -> Path:
    # Look for profiles/ relative to project root
    base = Path(__file__).resolve().parent.parent.parent.parent
    profiles_dir = base / "profiles"
    if not profiles_dir.exists():
        profiles_dir = Path("profiles").resolve()
    return profiles_dir

def validate_profile(data: Dict[str, Any]) -> None:
    required = ["name", "soc", "cpu", "native_abi", "ram_bytes", "display", "ioctls"]
    for field in required:
        if field not in data:
            raise ProfileValidationError(f"Missing required field: {field}")

    if data.get("soc") not in ["imx353", "imx31"]:
        raise ProfileValidationError(f"Invalid SoC: {data.get('soc')}")

    if data.get("cpu") != "armv6":
        raise ProfileValidationError(f"Invalid CPU architecture: {data.get('cpu')}")

    if not isinstance(data.get("ram_bytes"), int) or data["ram_bytes"] <= 0:
        raise ProfileValidationError("ram_bytes must be a positive integer")

    display = data.get("display")
    if not isinstance(display, dict):
        raise ProfileValidationError("display must be a dictionary")

    if display.get("width", 0) <= 0 or display.get("height", 0) <= 0:
        raise ProfileValidationError("display dimensions must be positive integers")

    valid_formats = {"gray4", "gray8", "gray16"}
    formats = display.get("formats", [])
    if not formats or not set(formats).issubset(valid_formats):
        raise ProfileValidationError(f"Invalid display formats: {formats}")

def load_profile(name: str) -> TargetProfile:
    profiles_dir = _get_profiles_dir()
    profile_path = profiles_dir / f"{name}.json"
    if not profile_path.exists():
        raise FileNotFoundError(f"Profile not found: {profile_path}")

    with open(profile_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    validate_profile(data)

    disp = data["display"]
    display_profile = DisplayProfile(
        width=disp["width"],
        height=disp["height"],
        formats=disp["formats"],
        ppi=disp.get("ppi", 167),
        eink_panel=disp.get("eink_panel", "Pearl")
    )

    return TargetProfile(
        name=data["name"],
        model_name=data.get("model_name", data["name"]),
        soc=data["soc"],
        cpu=data["cpu"],
        native_abi=data["native_abi"],
        ram_bytes=data["ram_bytes"],
        linux_kernel=data.get("linux_kernel", "2.6.x"),
        glibc_version=data.get("glibc_version", "2.5"),
        display=display_profile,
        ioctls=data["ioctls"]
    )
