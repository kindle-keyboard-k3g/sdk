import subprocess
from pathlib import Path
from typing import List, Optional

class JarSignerError(Exception):
    pass

class JarSigner:

    def __init__(self, jarsigner_bin: str = "jarsigner"):
        self.jarsigner_bin = jarsigner_bin

    def sign(
        self,
        jar_path: Path,
        keystore_path: Path,
        storepass: str,
        aliases: List[str],
        sigalg: str = "SHA256withRSA",
        digestalg: str = "SHA-256"
    ) -> None:
        """
        Sequentially signs the target JAR with each specified alias (dk, di, dn).
        """
        jar_path = Path(jar_path)
        keystore_path = Path(keystore_path)

        if not jar_path.exists():
            raise FileNotFoundError(f"Target JAR not found: {jar_path}")
        if not keystore_path.exists():
            raise FileNotFoundError(f"Keystore not found: {keystore_path}")

        for alias in aliases:
            cmd = [
                self.jarsigner_bin,
                "-keystore", str(keystore_path),
                "-storepass", storepass,
                "-sigalg", sigalg,
                "-digestalg", digestalg,
                str(jar_path),
                alias
            ]
            try:
                subprocess.run(cmd, capture_output=True, text=True, check=True)
            except (subprocess.CalledProcessError, FileNotFoundError) as e:
                raise JarSignerError(f"Failed signing with alias {alias}: {e}")

    def verify(self, jar_path: Path) -> bool:
        cmd = [self.jarsigner_bin, "-verify", str(jar_path)]
        try:
            res = subprocess.run(cmd, capture_output=True, text=True)
            return res.returncode == 0 and "jar verified." in res.stdout.lower()
        except FileNotFoundError:
            return False
