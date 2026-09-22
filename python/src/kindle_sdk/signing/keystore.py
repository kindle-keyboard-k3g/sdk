import subprocess
from pathlib import Path
from typing import Dict, Optional

class KeystoreError(Exception):
    pass

class KeystoreGenerator:

    def __init__(self, keytool_bin: str = "keytool"):
        self.keytool_bin = keytool_bin

    def generate(
        self,
        output_path: Path,
        password: str,
        aliases: Optional[Dict[str, str]] = None,
        key_algorithm: str = "RSA",
        key_size: int = 2048,
        sig_algorithm: str = "SHA256withRSA"
    ) -> None:
        """
        Generates an ephemeral keystore containing Kindle developer aliases (dk, di, dn).
        """
        if not password or len(password) < 6:
            raise KeystoreError("Password must be at least 6 characters")

        if aliases is None:
            aliases = {
                "dkDeveloper": "CN=Kindlet Developer, OU=Developer, O=Kindle Community, C=US",
                "diDeveloper": "CN=Kindlet Interaction, OU=Interaction, O=Kindle Community, C=US",
                "dnDeveloper": "CN=Kindlet Network, OU=Network, O=Kindle Community, C=US"
            }

        output_path = Path(output_path)
        if output_path.exists():
            output_path.unlink()

        for alias, dname in aliases.items():
            cmd = [
                self.keytool_bin,
                "-genkeypair",
                "-alias", alias,
                "-keystore", str(output_path),
                "-storepass", password,
                "-keypass", password,
                "-keyalg", key_algorithm,
                "-keysize", str(key_size),
                "-sigalg", sig_algorithm,
                "-dname", dname,
                "-validity", "3650"
            ]
            try:
                result = subprocess.run(cmd, capture_output=True, text=True, check=True)
            except (subprocess.CalledProcessError, FileNotFoundError) as e:
                raise KeystoreError(f"Failed to generate alias {alias}: {e}")
