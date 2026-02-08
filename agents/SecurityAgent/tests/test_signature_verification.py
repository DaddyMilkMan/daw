import unittest
from unittest.mock import patch, MagicMock, mock_open
import sys
import os
from pathlib import Path
import subprocess

# Add parent directory to path to import security_agent
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../')))

from security_agent import SecurityAgent

class TestSignatureVerification(unittest.TestCase):
    def setUp(self):
        self.agent = SecurityAgent()
        self.test_plugin_path = Path("/path/to/plugin.vst3")

    @patch('pathlib.Path.exists', return_value=True)
    @patch('builtins.open', new_callable=mock_open, read_data=b'file_content')
    @patch('hashlib.sha256')
    def test_allowlist_bypass(self, mock_sha256, mock_file, mock_exists):
        """Test that allowlisted plugins bypass signature verification."""
        # Setup mock hash
        mock_hasher = MagicMock()
        mock_hasher.hexdigest.return_value = "known_hash"
        mock_sha256.return_value = mock_hasher

        # Add hash to allowlist
        self.agent.known_safe_plugins.add("known_hash")

        # Mock _verify_signature to ensure it's NOT called
        with patch.object(self.agent, '_verify_signature') as mock_verify:
            result = self.agent.verify_plugin_signature(self.test_plugin_path)

            self.assertTrue(result)
            mock_verify.assert_not_called()

    @patch('pathlib.Path.exists', return_value=True)
    @patch('builtins.open', new_callable=mock_open, read_data=b'file_content')
    @patch('hashlib.sha256')
    @patch('platform.system')
    @patch('subprocess.run')
    def test_macos_signature_valid(self, mock_run, mock_system, mock_sha256, mock_file, mock_exists):
        """Test valid macOS signature."""
        mock_system.return_value = "Darwin"
        mock_hasher = MagicMock()
        mock_hasher.hexdigest.return_value = "unknown_hash"
        mock_sha256.return_value = mock_hasher

        # Mock subprocess success
        mock_run.return_value = MagicMock(returncode=0)

        # Mock _verify_signature method existence since we are testing public API
        # but we want to test the internal logic too. Ideally we test verify_plugin_signature
        # which calls _verify_signature.
        # However, _verify_signature is not yet implemented, so this test expects failure
        # until implementation. But wait, I'm writing tests FIRST.
        # So I will mock the internal methods if I were testing verify_plugin_signature,
        # OR I can test the _verify_signature method directly if I make it public or access it.
        # I will stick to testing verify_plugin_signature and let it call the real methods.

        # NOTE: Since _verify_signature is not implemented yet, this test will fail or error out
        # when running against current code. That is expected.

        result = self.agent.verify_plugin_signature(self.test_plugin_path)

        self.assertTrue(result)
        mock_run.assert_called_with(
            ["codesign", "-v", "--strict", str(self.test_plugin_path)],
            capture_output=True,
            text=True,
            timeout=10,
            check=False
        )

    @patch('pathlib.Path.exists', return_value=True)
    @patch('builtins.open', new_callable=mock_open, read_data=b'file_content')
    @patch('hashlib.sha256')
    @patch('platform.system')
    @patch('subprocess.run')
    def test_macos_signature_invalid(self, mock_run, mock_system, mock_sha256, mock_file, mock_exists):
        """Test invalid macOS signature."""
        mock_system.return_value = "Darwin"
        mock_hasher = MagicMock()
        mock_hasher.hexdigest.return_value = "unknown_hash"
        mock_sha256.return_value = mock_hasher

        mock_run.return_value = MagicMock(returncode=1)

        result = self.agent.verify_plugin_signature(self.test_plugin_path)

        self.assertFalse(result)

    @patch('pathlib.Path.exists', return_value=True)
    @patch('builtins.open', new_callable=mock_open, read_data=b'file_content')
    @patch('hashlib.sha256')
    @patch('platform.system')
    @patch('subprocess.run')
    def test_windows_signature_valid(self, mock_run, mock_system, mock_sha256, mock_file, mock_exists):
        """Test valid Windows signature."""
        mock_system.return_value = "Windows"
        mock_hasher = MagicMock()
        mock_hasher.hexdigest.return_value = "unknown_hash"
        mock_sha256.return_value = mock_hasher

        mock_run.return_value = MagicMock(returncode=0, stdout="Valid\n")

        result = self.agent.verify_plugin_signature(self.test_plugin_path)

        self.assertTrue(result)
        # Verify strict PowerShell command
        args, kwargs = mock_run.call_args
        command_list = args[0]
        # Command string is the last element
        command_str = command_list[-1]
        self.assertIn("Get-AuthenticodeSignature", command_str)
        self.assertIn("Status.ToString()", command_str)

        self.assertIn("-NoProfile", command_list)
        self.assertIn("-NonInteractive", command_list)

    @patch('pathlib.Path.exists', return_value=True)
    @patch('builtins.open', new_callable=mock_open, read_data=b'file_content')
    @patch('hashlib.sha256')
    @patch('platform.system')
    @patch('subprocess.run')
    def test_windows_signature_invalid(self, mock_run, mock_system, mock_sha256, mock_file, mock_exists):
        """Test invalid Windows signature."""
        mock_system.return_value = "Windows"
        mock_hasher = MagicMock()
        mock_hasher.hexdigest.return_value = "unknown_hash"
        mock_sha256.return_value = mock_hasher

        # Test "NotSigned"
        mock_run.return_value = MagicMock(returncode=0, stdout="NotSigned\n")
        self.assertFalse(self.agent.verify_plugin_signature(self.test_plugin_path))

        # Test "HashMismatch"
        mock_run.return_value = MagicMock(returncode=0, stdout="HashMismatch\n")
        self.assertFalse(self.agent.verify_plugin_signature(self.test_plugin_path))

        # Test empty output
        mock_run.return_value = MagicMock(returncode=0, stdout="")
        self.assertFalse(self.agent.verify_plugin_signature(self.test_plugin_path))

    @patch('pathlib.Path.exists', return_value=True)
    @patch('builtins.open', new_callable=mock_open, read_data=b'file_content')
    @patch('hashlib.sha256')
    @patch('platform.system')
    def test_linux_fail_closed(self, mock_system, mock_sha256, mock_file, mock_exists):
        """Test that unsupported platforms (Linux) fail closed for unknown plugins."""
        mock_system.return_value = "Linux"
        mock_hasher = MagicMock()
        mock_hasher.hexdigest.return_value = "unknown_hash"
        mock_sha256.return_value = mock_hasher

        # Should return False and NOT crash
        result = self.agent.verify_plugin_signature(self.test_plugin_path)
        self.assertFalse(result)

    @patch('pathlib.Path.exists', return_value=True)
    @patch('builtins.open', new_callable=mock_open, read_data=b'file_content')
    @patch('hashlib.sha256')
    @patch('platform.system')
    @patch('subprocess.run')
    def test_subprocess_timeout(self, mock_run, mock_system, mock_sha256, mock_file, mock_exists):
        """Test handling of subprocess timeout."""
        mock_system.return_value = "Darwin"
        mock_hasher = MagicMock()
        mock_hasher.hexdigest.return_value = "unknown_hash"
        mock_sha256.return_value = mock_hasher

        mock_run.side_effect = subprocess.TimeoutExpired(cmd="codesign", timeout=10)

        result = self.agent.verify_plugin_signature(self.test_plugin_path)
        self.assertFalse(result)

    @patch('builtins.open', side_effect=FileNotFoundError)
    def test_file_not_found_hashing(self, mock_file):
        """Test handling of missing file during hashing."""
        result = self.agent.verify_plugin_signature(Path("/non/existent/file"))
        self.assertFalse(result)

    @patch('pathlib.Path.exists', return_value=True)
    @patch('builtins.open', new_callable=mock_open, read_data=b'file_content')
    @patch('hashlib.sha256')
    @patch('platform.system')
    @patch('subprocess.run')
    def test_windows_signature_with_quotes(self, mock_run, mock_system, mock_sha256, mock_file, mock_exists):
        """Test Windows signature verification with quotes in filename."""
        mock_system.return_value = "Windows"
        mock_hasher = MagicMock()
        mock_hasher.hexdigest.return_value = "unknown_hash"
        mock_sha256.return_value = mock_hasher

        mock_run.return_value = MagicMock(returncode=0, stdout="Valid\n")

        # Plugin path with single quote
        bad_path = Path("/path/to/bad'plugin.vst3")

        result = self.agent.verify_plugin_signature(bad_path)

        self.assertTrue(result)

        args, kwargs = mock_run.call_args
        command_list = args[0]
        command_str = command_list[-1]

        # Check for escaped single quote: bad''plugin.vst3
        expected_escaped_path = "/path/to/bad''plugin.vst3"
        self.assertIn(expected_escaped_path, command_str)

if __name__ == '__main__':
    unittest.main()
