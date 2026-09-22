"""
Kindle SDK Command-Line Interface.

Provides developer commands for toolchain diagnostics, template instantiation,
and hardware profile inspection.
"""

import argparse
import sys
from pathlib import Path
from .toolchains import check_toolchains
from .project.init import init_project, TEMPLATES
from .profiles import load_profile

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

    return 0

if __name__ == "__main__":
    sys.exit(main())
