"""
Active Content (.azw2) container packaging.

Packages compiled Java bytecode, manifests, embedded native binaries,
and application resources into unsigned Kindlet archive containers.
"""

import zipfile
import shutil
from pathlib import Path
from typing import List, Dict, Optional
from .manifest import ManifestBuilder, ManifestSpec

class Azw2Packager:
    """
    Assembles Active Content containers (.azw2) conforming to the Kindle format.
    """

    def __init__(self):
        """Initializes the packager with an internal ManifestBuilder."""
        self.manifest_builder = ManifestBuilder()

    def package(
        self,
        output_azw2: Path,
        classes_dir: Path,
        manifest_spec: ManifestSpec,
        native_binaries: Optional[Dict[str, Path]] = None,
        resources: Optional[Dict[str, Path]] = None
    ) -> Path:
        """
        Packs Java class files, manifest, and optional native ARM binaries into an unsigned JAR,
        ready for signing and deployment as an .azw2 container.

        Args:
            output_azw2: Destination path for the output .azw2 container file.
            classes_dir: Directory containing compiled .class files.
            manifest_spec: ManifestSpec containing metadata headers.
            native_binaries: Optional dictionary mapping in-archive paths to native binary files on disk.
            resources: Optional dictionary mapping in-archive paths to resource files on disk.

        Returns:
            Path to the newly created .azw2 archive.

        Raises:
            FileNotFoundError: If classes_dir does not exist.
        """
        output_azw2 = Path(output_azw2)
        classes_dir = Path(classes_dir)

        if not classes_dir.exists():
            raise FileNotFoundError(f"Classes directory not found: {classes_dir}")

        output_azw2.parent.mkdir(parents=True, exist_ok=True)

        manifest_data = self.manifest_builder.build(manifest_spec)

        with zipfile.ZipFile(output_azw2, "w", compression=zipfile.ZIP_DEFLATED) as zf:
            # Write MANIFEST.MF first
            zf.writestr("META-INF/MANIFEST.MF", manifest_data)

            # Write class files
            for class_file in classes_dir.rglob("*.class"):
                arcname = str(class_file.relative_to(classes_dir))
                zf.write(class_file, arcname)

            # Write optional native binaries
            if native_binaries:
                for arcname, bin_path in native_binaries.items():
                    if bin_path.exists():
                        zf.write(bin_path, arcname)

            # Write resources
            if resources:
                for arcname, res_path in resources.items():
                    if res_path.exists():
                        zf.write(res_path, arcname)

        return output_azw2
