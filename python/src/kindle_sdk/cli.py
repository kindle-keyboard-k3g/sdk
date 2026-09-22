"""
Kindle SDK Command-Line Interface.

Provides developer commands for toolchain diagnostics, template instantiation,
hardware profile inspection, package signing, and desktop emulation.
"""

import argparse
import sys
import subprocess
from pathlib import Path
from .toolchains import check_toolchains
from .project.init import init_project, TEMPLATES
from .profiles import load_profile
from .signing.keystore import KeystoreGenerator
from .signing.jarsigner import JarSigner

def cmd_doctor(args: argparse.Namespace) -> int:
    """
    Executes the 'doctor' subcommand to audit local toolchain installations.

    Args:
        args: Parsed command-line arguments.

    Returns:
        0 if essential tools are present, 1 otherwise.
    """
    print("=== Kindle SDK Toolchain Doctor ===")
    status = check_toolchains()
    all_ok = True
    for tool, available in status.items():
        sym = "[OK]" if available else "[MISSING (Docker fallback available)]"
        print(f"  {tool:22}: {sym}")
        if not available and tool in ["docker", "python3"]:
            all_ok = False
    return 0 if all_ok else 1

def cmd_init(args: argparse.Namespace) -> int:
    """
    Executes the 'init' subcommand to scaffold a new project from a starter template.

    Args:
        args: Parsed command-line arguments containing target_dir, template, and name.

    Returns:
        0 on success.
    """
    target = Path(args.target_dir)
    print(f"Initializing Kindle project '{args.name}' with template '{args.template}' at {target}...")
    init_project(target, args.template, args.name)
    print("SUCCESS: Project created successfully!")
    return 0

def cmd_profiles(args: argparse.Namespace) -> int:
    """
    Executes the 'profiles' subcommand to display supported Kindle hardware configurations.

    Args:
        args: Parsed command-line arguments.

    Returns:
        0 on success.
    """
    print("=== Supported Kindle Hardware Profiles ===")
    for name in ["k3", "dx"]:
        p = load_profile(name)
        print(f"Profile: {p.name}")
        print(f"  Model:     {p.model_name}")
        print(f"  SoC/CPU:   {p.soc} ({p.cpu})")
        print(f"  RAM:       {p.ram_bytes // (1024*1024)} MB")
        print(f"  Display:   {p.display.width}x{p.display.height} ({', '.join(p.display.formats)})")
    return 0

def cmd_sign(args: argparse.Namespace) -> int:
    """
    Executes the 'sign' subcommand to generate developer keystores or triple-sign JAR / .azw2 packages.

    Args:
        args: Parsed command-line arguments.

    Returns:
        0 on success, non-zero on error.
    """
    target = Path(args.target)
    if not target.exists():
        print(f"Error: Target file not found: {target}", file=sys.stderr)
        return 1

    keystore_path = Path(args.keystore)
    password = args.password

    # If keystore doesn't exist, generate it automatically
    if not keystore_path.exists():
        print(f"Keystore not found. Generating developer keystore with dk, di, dn aliases at {keystore_path}...")
        gen = KeystoreGenerator()
        gen.generate(keystore_path, password)
        print("Developer keystore generated successfully.")

    print(f"Signing package {target} with triple developer keys...")
    signer = JarSigner()
    signer.sign(target, keystore_path, password, ["dkDeveloper", "diDeveloper", "dnDeveloper"])

    if signer.verify(target):
        print(f"SUCCESS: Package {target} signed and verified successfully!")
        return 0
    else:
        print(f"ERROR: Signature verification failed for {target}", file=sys.stderr)
        return 1

def cmd_emulate(args: argparse.Namespace) -> int:
    """
    Executes the 'emulate' subcommand to launch a Kindlet in the desktop simulator.

    Args:
        args: Parsed command-line arguments.

    Returns:
        Exit code of simulator runner.
    """
    target = Path(args.target)
    if not target.exists():
        print(f"Error: Target file not found: {target}", file=sys.stderr)
        return 1

    # Locate emulator JAR
    repo_root = Path(__file__).resolve().parent.parent.parent.parent
    emulator_jar = repo_root / "java" / "emulator" / "dist" / "kindle-emulator.jar"
    api_jar = repo_root / "java" / "kindlet-api" / "dist" / "kindlet-api.jar"
    bridge_jar = repo_root / "java" / "kindlet-bridge" / "dist" / "kindlet-bridge.jar"

    if not emulator_jar.exists():
        print(f"Error: Emulator JAR not found at {emulator_jar}. Run ant in java/emulator first.", file=sys.stderr)
        return 2

    classpath = f"{emulator_jar}:{api_jar}:{bridge_jar}"
    cmd = ["java", "-cp", classpath, "com.amazon.kindle.emulator.EmulatorLauncher", str(target.resolve())]
    if args.headless:
        cmd.append("--headless")
    if args.width:
        cmd.extend(["--width", str(args.width)])
    if args.height:
        cmd.extend(["--height", str(args.height)])

    print(f"Starting Kindle Simulator: {' '.join(cmd)}")
    res = subprocess.run(cmd)
    return res.returncode

def main() -> int:
    """
    Main entry point for the kindle-sdk CLI executable.

    Returns:
        Exit code integer.
    """
    parser = argparse.ArgumentParser(prog="kindle-sdk", description="Kindle SDK command line interface")
    subparsers = parser.add_subparsers(dest="subcommand", help="Available subcommands")

    # doctor
    subparsers.add_parser("doctor", help="Check local toolchains and build dependencies")

    # profiles
    subparsers.add_parser("profiles", help="List supported Kindle hardware profiles")

    # init
    init_parser = subparsers.add_parser("init", help="Create a new Kindle project from a template")
    init_parser.add_argument("target_dir", help="Destination folder for project")
    init_parser.add_argument("--template", choices=TEMPLATES, default="pure-kindlet", help="Project template")
    init_parser.add_argument("--name", default="MyKindleApp", help="Application title")

    # sign
    sign_parser = subparsers.add_parser("sign", help="Sign a JAR or .azw2 package with Kindle developer keys")
    sign_parser.add_argument("target", help="Path to JAR or .azw2 to sign")
    sign_parser.add_argument("--keystore", default="developer.keystore", help="Path to keystore file (auto-generated if missing)")
    sign_parser.add_argument("--password", default="password123", help="Keystore and key password (min 6 chars)")

    # emulate
    emu_parser = subparsers.add_parser("emulate", help="Launch Kindlet package in desktop simulator")
    emu_parser.add_argument("target", help="Path to JAR or .azw2 package to emulate")
    emu_parser.add_argument("--headless", action="store_true", help="Run headlessly (create -> start -> stop -> destroy)")
    emu_parser.add_argument("--width", type=int, default=600, help="Display width")
    emu_parser.add_argument("--height", type=int, default=800, help="Display height")

    args = parser.parse_args()
    if not args.subcommand:
        parser.print_help()
        return 1

    if args.subcommand == "doctor":
        return cmd_doctor(args)
    elif args.subcommand == "init":
        return cmd_init(args)
    elif args.subcommand == "profiles":
        return cmd_profiles(args)
    elif args.subcommand == "sign":
        return cmd_sign(args)
    elif args.subcommand == "emulate":
        return cmd_emulate(args)

    return 0

if __name__ == "__main__":
    sys.exit(main())
