using System;
using System.Diagnostics;
using System.IO;
using System.Windows;

namespace VortexSetup
{
    public partial class App : Application
    {
        protected override void OnStartup(StartupEventArgs e)
        {
            // Parse arguments
            bool isSilent = false;
            foreach (string arg in e.Args)
            {
                if (arg.Equals("/S", StringComparison.OrdinalIgnoreCase) || 
                    arg.Equals("/s", StringComparison.OrdinalIgnoreCase))
                {
                    isSilent = true;
                }
            }

            if (isSilent)
            {
                string exeName = Path.GetFileNameWithoutExtension(Environment.ProcessPath ?? "");
                int exitCode;
                if (exeName.Equals("uninst", StringComparison.OrdinalIgnoreCase) || 
                    exeName.Equals("uninstall", StringComparison.OrdinalIgnoreCase))
                {
                    // Run silent uninstallation and exit
                    exitCode = RunSilentUninstallation();
                }
                else
                {
                    // Run silent installation and exit
                    exitCode = RunSilentInstallation(e.Args);
                }

                Shutdown(exitCode);
                return;
            }

            base.OnStartup(e);

            // Programmatically launch the main window when running in normal interactive GUI mode
            MainWindow mainWindow = new MainWindow();
            mainWindow.Show();
        }

        private int RunSilentUninstallation()
        {
            try
            {
                string targetDir = AppDomain.CurrentDomain.BaseDirectory;
                try
                {
                    SetupBackend.EnsureCoreUninstaller(targetDir);
                }
                catch
                {
                    string? installedPath = SetupBackend.GetInstalledPathFromRegistry();
                    if (string.IsNullOrWhiteSpace(installedPath))
                    {
                        return 2;
                    }

                    targetDir = installedPath;
                    SetupBackend.EnsureCoreUninstaller(targetDir);
                }

                string coreUninstPath = Path.Combine(targetDir, "uninst_core.exe");

                if (File.Exists(coreUninstPath))
                {
                    int exitCode = SetupBackend.RunElevatedAndWait(coreUninstPath, "/S");
                    if (exitCode != 0)
                    {
                        return exitCode;
                    }
                }
                else
                {
                    return 2;
                }

                // Delete self after exit
                string selfPath = Environment.ProcessPath ?? "";
                string cmdArgs = string.Format("/c timeout /t 2 /nobreak >nul && del /f /q \"{0}\"", selfPath);
                if (File.Exists(coreUninstPath))
                {
                    cmdArgs += string.Format(" && del /f /q \"{0}\"", coreUninstPath);
                }
                cmdArgs += string.Format(" && rmdir /s /q \"{0}\"", targetDir);

                ProcessStartInfo cleanupPsi = new ProcessStartInfo
                {
                    FileName = "cmd.exe",
                    Arguments = cmdArgs,
                    CreateNoWindow = true,
                    UseShellExecute = false,
                    WindowStyle = ProcessWindowStyle.Hidden
                };
                Process.Start(cleanupPsi);

                return 0;
            }
            catch
            {
                return 1;
            }
        }

        private int RunSilentInstallation(string[] args)
        {
            try
            {
                string tempDir = Path.Combine(Path.GetTempPath(), "VortexInstallTemp_" + Guid.NewGuid().ToString("N"));
                string tempInstallerPath = SetupBackend.ExtractEmbeddedInstaller(tempDir);

                if (File.Exists(tempInstallerPath))
                {
                    // Pass all args through
                    string arguments = string.Join(" ", args);
                    int exitCode = SetupBackend.RunElevatedAndWait(tempInstallerPath, arguments);
                    if (exitCode != 0)
                    {
                        return exitCode;
                    }

                    string? installDir = SetupBackend.GetInstalledPathFromRegistry();
                    if (!string.IsNullOrWhiteSpace(installDir))
                    {
                        SetupBackend.CopySelfAsUninstaller(installDir);
                    }
                }
                else
                {
                    return 3;
                }

                // Clean up temp
                try
                {
                    File.Delete(tempInstallerPath);
                    Directory.Delete(tempDir, true);
                }
                catch { }

                return 0;
            }
            catch
            {
                return 1;
            }
        }
    }
}
