import unittest
from unittest.mock import MagicMock, patch, mock_open
import sys
from pathlib import Path
import hashlib
import subprocess

# Add parent directory to path to import SecurityAgent
sys.path.append(str(Path(__file__).parent.parent))
from security_agent import SecurityAgent

class TestSecurityAgent(unittest.TestCase):
    def setUp(self):
        self.agent = SecurityAgent()
        self.plugin_path = Path("/path/to/plugin.vst")
        self.plugin_content = b"fake_plugin_content"
        self.plugin_hash = hashlib.sha256(self.plugin_content).hexdigest()

    @patch("pathlib.Path.exists", return_value=True)
    @patch("builtins.open", new_callable=mock_open, read_data=b"fake_plugin_content")
    def test_verify_plugin_signature_allowlist(self, mock_file, mock_exists):
        """Test that plugins in the allowlist are verified without OS check."""
        # Add hash to allowlist
        self.agent.known_safe_plugins.add(self.plugin_hash)

        # Should return True regardless of OS or signature status
        with patch("subprocess.run") as mock_run:
            result = self.agent.verify_plugin_signature(self.plugin_path)
            self.assertTrue(result)
            mock_run.assert_not_called()

    @patch("pathlib.Path.exists", return_value=True)
    @patch("builtins.open", new_callable=mock_open, read_data=b"fake_plugin_content")
    @patch("platform.system", return_value="Darwin")
    @patch("subprocess.run")
    def test_verify_plugin_signature_macos_valid(self, mock_run, mock_system, mock_file, mock_exists):
        """Test valid signature on macOS."""
        mock_run.return_value.returncode = 0

        result = self.agent.verify_plugin_signature(self.plugin_path)

        self.assertTrue(result)
        mock_run.assert_called_with(
            ["codesign", "-v", "--strict", str(self.plugin_path)],
            capture_output=True,
            timeout=5,
            check=False
        )

    @patch("pathlib.Path.exists", return_value=True)
    @patch("builtins.open", new_callable=mock_open, read_data=b"fake_plugin_content")
    @patch("platform.system", return_value="Darwin")
    @patch("subprocess.run")
    def test_verify_plugin_signature_macos_invalid(self, mock_run, mock_system, mock_file, mock_exists):
        """Test invalid signature on macOS."""
        mock_run.return_value.returncode = 1

        result = self.agent.verify_plugin_signature(self.plugin_path)

        self.assertFalse(result)

    @patch("pathlib.Path.exists", return_value=True)
    @patch("builtins.open", new_callable=mock_open, read_data=b"fake_plugin_content")
    @patch("platform.system", return_value="Windows")
    @patch("subprocess.run")
    def test_verify_plugin_signature_windows_valid(self, mock_run, mock_system, mock_file, mock_exists):
        """Test valid signature on Windows."""
        mock_run.return_value.stdout = b"Valid"
        mock_run.return_value.returncode = 0

        result = self.agent.verify_plugin_signature(self.plugin_path)

        self.assertTrue(result)
        # Verify correct command for Windows
        args, kwargs = mock_run.call_args
        self.assertIn("Get-AuthenticodeSignature", args[0][2])
        self.assertIn("Valid", str(mock_run.return_value.stdout))

    @patch("pathlib.Path.exists", return_value=True)
    @patch("builtins.open", new_callable=mock_open, read_data=b"fake_plugin_content")
    @patch("platform.system", return_value="Windows")
    @patch("subprocess.run")
    def test_verify_plugin_signature_windows_invalid(self, mock_run, mock_system, mock_file, mock_exists):
        """Test invalid signature on Windows."""
        mock_run.return_value.stdout = b"Invalid"
        mock_run.return_value.returncode = 0

        result = self.agent.verify_plugin_signature(self.plugin_path)

        self.assertFalse(result)

    @patch("pathlib.Path.exists", return_value=True)
    @patch("builtins.open", new_callable=mock_open, read_data=b"fake_plugin_content")
    @patch("platform.system", return_value="Linux")
    @patch("subprocess.run")
    def test_verify_plugin_signature_linux(self, mock_run, mock_system, mock_file, mock_exists):
        """Test behavior on Linux (should return False)."""
        result = self.agent.verify_plugin_signature(self.plugin_path)

        self.assertFalse(result)
        mock_run.assert_not_called()

    @patch("pathlib.Path.exists", return_value=True)
    @patch("builtins.open", new_callable=mock_open, read_data=b"fake_plugin_content")
    @patch("platform.system", return_value="Darwin")
    @patch("subprocess.run")
    def test_verify_plugin_signature_timeout(self, mock_run, mock_system, mock_file, mock_exists):
        """Test timeout handling."""
        mock_run.side_effect = subprocess.TimeoutExpired(cmd="codesign", timeout=5)

        result = self.agent.verify_plugin_signature(self.plugin_path)

        self.assertFalse(result)

    @patch("pathlib.Path.exists", return_value=True)
    @patch("builtins.open", new_callable=mock_open, read_data=b"fake_plugin_content")
    @patch("platform.system", return_value="Darwin")
    @patch("subprocess.run")
    def test_verify_plugin_signature_missing_tool(self, mock_run, mock_system, mock_file, mock_exists):
        """Test handling of missing tools (FileNotFoundError)."""
        mock_run.side_effect = FileNotFoundError

        result = self.agent.verify_plugin_signature(self.plugin_path)

        self.assertFalse(result)

if __name__ == "__main__":
    unittest.main()
