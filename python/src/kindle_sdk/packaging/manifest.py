from dataclasses import dataclass
from typing import Optional

@dataclass(frozen=True)
class ManifestSpec:
    main_class: str
    implementation_title: str
    implementation_version: str = "1.0.0"
    sdk_specification_version: str = "2.1"
    toolbar_mode: str = "persistent"
    cover_image: Optional[str] = None

class ManifestValidationError(Exception):
    pass

class ManifestBuilder:

    VALID_TOOLBAR_MODES = {"persistent", "transient", "none"}

    def build(self, spec: ManifestSpec) -> bytes:
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
