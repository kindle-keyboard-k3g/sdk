"""
Developer keystore generation.

Generates local development keystores containing Kindlet certificate aliases
(dk, di, dn) using JDK keytool or OpenSSL fallback.
"""

import subprocess
import shutil
from pathlib import Path
from typing import Dict, Optional

class KeystoreError(Exception):
    """Raised when keystore generation fails."""
    pass

class KeystoreGenerator:
    """
    Automates generation of developer keystores containing Kindle developer key aliases.
    """

    def __init__(self, keytool_bin: Optional[str] = None):
        """
        Initializes the generator with an optional path to the keytool binary.

        Args:
            keytool_bin: Optional explicit path to keytool executable.
        """
        self.keytool_bin = keytool_bin or shutil.which("keytool")

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
        Generates a keystore containing Kindle developer aliases (dk, di, dn).
        Uses keytool if available, otherwise generates RSA keypairs and PKCS12 keystore via OpenSSL.

        Args:
            output_path: Path where generated keystore will be saved.
            password: Password for both keystore and private key protection (minimum 6 characters).
            aliases: Optional dictionary mapping alias names to X.500 distinguished names.
            key_algorithm: Asymmetric key algorithm (default: 'RSA').
            key_size: Key length in bits (default: 2048).
            sig_algorithm: Signature algorithm (default: 'SHA256withRSA').

        Raises:
            KeystoreError: If password is too short or keytool/openssl execution fails.
        """
        if not password or len(password) < 6:
            raise KeystoreError("Password must be at least 6 characters")

        if aliases is None:
            aliases = {
                "dkDeveloper": "/CN=Kindlet Developer/OU=Developer/O=Kindle Community/C=US",
                "diDeveloper": "/CN=Kindlet Interaction/OU=Interaction/O=Kindle Community/C=US",
                "dnDeveloper": "/CN=Kindlet Network/OU=Network/O=Kindle Community/C=US"
            }

        output_path = Path(output_path)
        if output_path.exists():
            output_path.unlink()

        if self.keytool_bin and shutil.which(self.keytool_bin):
            for alias, dname in aliases.items():
                # Convert slash format to standard comma format if needed
                dname_str = dname.replace("/", ", ").strip(", ")
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
                    "-dname", dname_str,
                    "-validity", "3650"
                ]
                try:
                    subprocess.run(cmd, capture_output=True, text=True, check=True)
                except subprocess.CalledProcessError as e:
                    raise KeystoreError(f"keytool failed for alias {alias}: {e.stderr}")
        else:
            # Fallback to OpenSSL generating self-signed certificates and PKCS12 store
            openssl = shutil.which("openssl")
            if not openssl:
                raise KeystoreError("Neither keytool nor openssl is available to generate keystores")

            tmp_dir = output_path.parent / f".tmp_ks_{output_path.stem}"
            tmp_dir.mkdir(parents=True, exist_ok=True)
            try:
                for alias, dname in aliases.items():
                    key_file = tmp_dir / f"{alias}.key"
                    crt_file = tmp_dir / f"{alias}.crt"
                    p12_file = tmp_dir / f"{alias}.p12"

                    # Generate RSA key and self-signed cert
                    cmd_req = [
                        openssl, "req", "-x509", "-newkey", f"rsa:{key_size}",
                        "-keyout", str(key_file), "-out", str(crt_file),
                        "-days", "3650", "-nodes", "-subj", dname
                    ]
                    subprocess.run(cmd_req, capture_output=True, check=True)

                    # Export to PKCS12
                    cmd_pkcs12 = [
                        openssl, "pkcs12", "-export",
                        "-in", str(crt_file), "-inkey", str(key_file),
                        "-out", str(p12_file),
                        "-name", alias,
                        "-passout", f"pass:{password}"
                    ]
                    subprocess.run(cmd_pkcs12, capture_output=True, check=True)

                # Use the primary alias p12 as output keystore
                primary_p12 = tmp_dir / f"{list(aliases.keys())[0]}.p12"
                output_path.write_bytes(primary_p12.read_bytes())
            finally:
                shutil.rmtree(tmp_dir, ignore_errors=True)
