using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using Microsoft.Win32;

namespace VortexSetup
{
    internal static class SetupBackend
    {
        private const string EmbeddedInstallerResourceName = "VortexSetup.nsis_installer.exe";

        public static string ExtractEmbeddedInstaller(string tempDir)
        {
            Directory.CreateDirectory(tempDir);

            string tempInstallerPath = Path.Combine(tempDir, "vortex_nsis_installer.exe");
            using Stream? resourceStream = Assembly.GetExecutingAssembly()
                .GetManifestResourceStream(EmbeddedInstallerResourceName);

            if (resourceStream == null)
            {
                throw new InvalidOperationException("Could not find embedded NSIS installer in resources.");
            }

            using FileStream fileStream = new FileStream(tempInstallerPath, FileMode.Create, FileAccess.Write);
            resourceStream.CopyTo(fileStream);

            return tempInstallerPath;
        }

        public static void CopySelfAsUninstaller(string targetDir)
        {
            string selfPath = Environment.ProcessPath ?? "";
            if (!File.Exists(selfPath))
            {
                throw new FileNotFoundException("Could not find setup wrapper executable.", selfPath);
            }

            string targetUninstPath = Path.Combine(targetDir, "uninst.exe");
            if (!string.Equals(selfPath, targetUninstPath, StringComparison.OrdinalIgnoreCase))
            {
                File.Copy(selfPath, targetUninstPath, true);
            }
        }

        public static string? GetInstalledPathFromRegistry()
        {
            foreach (RegistryView registryView in new[] { RegistryView.Registry64, RegistryView.Registry32 })
            {
                using RegistryKey? key = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, registryView)
                    .OpenSubKey(@"Software\Vortex");
                string? installDir = key?.GetValue("InstallLocation") as string;
                if (!string.IsNullOrWhiteSpace(installDir))
                {
                    return installDir;
                }
            }

            return null;
        }

        public static void EnsureCoreUninstaller(string targetDir)
        {
            if (File.Exists(Path.Combine(targetDir, "uninst_core.exe")))
            {
                return;
            }

            string tempDir = Path.Combine(Path.GetTempPath(), "VortexUninstallRepair_" + Guid.NewGuid().ToString("N"));
            try
            {
                string tempInstallerPath = ExtractEmbeddedInstaller(tempDir);
                RunElevatedAndWait(tempInstallerPath, $"/S /REPAIRUNINSTALLER=1 /D={targetDir}");

                if (!File.Exists(Path.Combine(targetDir, "uninst_core.exe")))
                {
                    throw new FileNotFoundException("Could not repair core uninstaller executable.", Path.Combine(targetDir, "uninst_core.exe"));
                }
            }
            finally
            {
                try
                {
                    Directory.Delete(tempDir, true);
                }
                catch
                {
                }
            }
        }

        public static int RunElevatedAndWait(string fileName, string arguments)
        {
            if (!File.Exists(fileName))
            {
                throw new FileNotFoundException("Executable not found.", fileName);
            }

            ProcessStartInfo psi = new ProcessStartInfo
            {
                FileName = fileName,
                Arguments = arguments,
                UseShellExecute = true,
                Verb = "runas"
            };

            using Process? process = Process.Start(psi);
            if (process == null)
            {
                throw new InvalidOperationException($"Failed to start elevated process: {fileName}");
            }

            process.WaitForExit();
            return process.ExitCode;
        }
    }
}
