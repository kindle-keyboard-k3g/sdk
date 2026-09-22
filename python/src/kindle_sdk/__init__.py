"""
Kindle SDK core package.

Provides profiles, toolchain checks, project scaffolding, manifest generation,
.azw2 packaging, and cryptographic signing tools targeting legacy Amazon Kindle devices.
"""

__version__ = "1.0.0"
__author__ = "Kindle SDK Contributors"

from .profiles import load_profile, validate_profile, TargetProfile, DisplayProfile, ProfileValidationError
from .toolchains import check_toolchains

__all__ = [
    "load_profile",
    "validate_profile",
    "TargetProfile",
    "DisplayProfile",
    "ProfileValidationError",
    "check_toolchains",
]
