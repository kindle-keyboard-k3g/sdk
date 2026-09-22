"""
Kindle Active Content Manifest generation.

Builds META-INF/MANIFEST.MF files complying with Amazon Kindlet and Sun CVM constraints,
including Extension-List directives, toolbar modes, and exact CRLF delimiters.
"""

from dataclasses import dataclass
from typing import Optional

@dataclass(frozen=True)
class ManifestSpec:
    """
    Specification parameters for Kindle Active Content manifest headers.

    Attributes:
        main_class: Fully qualified Java class name implementing com.amazon.kindle.kindlet.Kindlet.
        implementation_title: Human-readable application title shown in Kindle library.
        implementation_version: Application semantic version string.
        sdk_specification_version: Target Amazon Kindlet SDK specification level (default: '2.1').
        toolbar_mode: System status/toolbar display mode ('persistent', 'transient', or 'none').
        cover_image: Optional relative path to book cover thumbnail inside JAR.
    """
    main_class: str
    implementation_title: str
    implementation_version: str = "1.0.0"
    sdk_specification_version: str = "2.1"
    toolbar_mode: str = "persistent"
    cover_image: Optional[str] = None

class ManifestValidationError(Exception):
    """Raised when manifest specification contains invalid or missing required attributes."""
    pass

class ManifestBuilder:
    """
    Constructs compliant META-INF/MANIFEST.MF byte sequences for Amazon Kindlets.
    """

    VALID_TOOLBAR_MODES = {"persistent", "transient", "none"}

    def build(self, spec: ManifestSpec) -> bytes:
        """
        Builds UTF-8 encoded manifest byte content formatted with strict CRLF line endings.

        Args:
            spec: ManifestSpec defining Kindlet properties.

        Returns:
            bytes containing the manifest file ready for injection into META-INF/MANIFEST.MF.

        Raises:
            ManifestValidationError: If main_class or implementation_title are empty, or toolbar_mode is invalid.
        """
        if not spec.main_class:
            raise ManifestValidationError("Main-Class cannot be empty")
        if not spec.implementation_title:
            raise ManifestValidationError("Implementation-Title cannot be empty")
        if spec.toolbar_mode not in self.VALID_TOOLBAR_MODES:
            raise ManifestValidationError(f"Invalid Toolbar-Mode: {spec.toolbar_mode}")

        lines = [
            "Manifest-Version: 1.0",
            f"Main-Class: {spec.main_class}",
            f"Implementation-Title: {spec.implementation_title}",
            f"Implementation-Version: {spec.implementation_version}",
            "Extension-List: SDK",
            "SDK-Extension-Name: com.amazon.kindle.kindlet",
            f"SDK-Specification-Version: {spec.sdk_specification_version}",
            f"Toolbar-Mode: {spec.toolbar_mode}",
        ]

        if spec.cover_image:
            lines.append(f"Amazon-Cover-Image: {spec.cover_image}")

        # Amazon Kindlet requires trailing newline and no trailing spaces
        content = "\r\n".join(lines) + "\r\n\r\n"
        return content.encode("utf-8")
