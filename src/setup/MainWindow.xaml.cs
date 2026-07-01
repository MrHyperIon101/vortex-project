using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Threading.Tasks;
using System.Windows;
using Microsoft.Win32;

namespace VortexSetup
{
    public partial class MainWindow : Wpf.Ui.Controls.FluentWindow
    {
        private enum WizardPage
        {
            Welcome = 1,
            License = 2,
            Upgrade = 3,
            Features = 4,
            Options = 5,
            Installing = 6,
            Finish = 7,
            UninstallWelcome = 8,
            Uninstalling = 9,
            UninstallFinish = 10
        }

        private WizardPage _currentPage = WizardPage.Welcome;
        private bool _isUninstallMode = false;
        private bool _isExistingInstallDetected = false;
        private string _existingInstallPath = string.Empty;

        public MainWindow()
        {
            InitializeComponent();
            DetectUninstallMode();
            Loaded += MainWindow_Loaded;
        }

        private void DetectUninstallMode()
        {
            try
            {
                string exeName = Path.GetFileNameWithoutExtension(Environment.ProcessPath ?? "");
                if (exeName.Equals("uninst", StringComparison.OrdinalIgnoreCase) ||
                    exeName.Equals("uninstall", StringComparison.OrdinalIgnoreCase))
                {
                    _isUninstallMode = true;
                    _currentPage = WizardPage.UninstallWelcome;
                }
            }
            catch { }
        }

        private void MainWindow_Loaded(object sender, RoutedEventArgs e)
        {
            LoadLicense();
            DetectExistingInstallation();
            UpdateWizardState();
        }

        private void LoadLicense()
        {
            try
            {
                var assembly = Assembly.GetExecutingAssembly();
                using (Stream stream = assembly.GetManifestResourceStream("VortexSetup.license.txt")!)
                {
                    if (stream != null)
                    {
                        using (StreamReader reader = new StreamReader(stream))
                        {
                            TxtLicense.Text = reader.ReadToEnd();
                        }
                    }
                    else
                    {
                        TxtLicense.Text = "GNU GENERAL PUBLIC LICENSE\nVersion 2, June 1991\n\n(Could not load full license text from resources)";
                    }
                }
            }
            catch (Exception ex)
            {
                TxtLicense.Text = $"Error loading license: {ex.Message}";
            }
        }

        private void DetectExistingInstallation()
        {
            try
            {
                // Check 64-bit registry first
                using (var key = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Registry64)
                                            .OpenSubKey(@"Software\Vortex"))
                {
                    if (key != null)
                    {
                        var path = key.GetValue("InstallLocation") as string;
                        if (IsValidInstallPath(path))
                        {
                            _isExistingInstallDetected = true;
                            _existingInstallPath = path!;
                        }
                    }
                }

                // If not found, check 32-bit registry view
                if (!_isExistingInstallDetected)
                {
                    using (var key = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Registry32)
                                                .OpenSubKey(@"Software\Vortex"))
                    {
                        if (key != null)
                        {
                            var path = key.GetValue("InstallLocation") as string;
                            if (IsValidInstallPath(path))
                            {
                                _isExistingInstallDetected = true;
                                _existingInstallPath = path!;
                            }
                        }
                    }
                }

                if (_isExistingInstallDetected)
                {
                    TxtInstallDir.Text = _existingInstallPath;
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"Error detecting existing installation: {ex.Message}");
            }
        }

        private static bool IsValidInstallPath(string? path)
        {
            return !string.IsNullOrEmpty(path)
                && Directory.Exists(path)
                && (File.Exists(Path.Combine(path, "uninst_core.exe"))
                    || File.Exists(Path.Combine(path, "uninst.exe")));
        }

        private void UpdateWizardState()
        {
            // Toggle visibility of wizard grids
            PageWelcome.Visibility = _currentPage == WizardPage.Welcome ? Visibility.Visible : Visibility.Collapsed;
            PageLicense.Visibility = _currentPage == WizardPage.License ? Visibility.Visible : Visibility.Collapsed;
            PageUpgrade.Visibility = _currentPage == WizardPage.Upgrade ? Visibility.Visible : Visibility.Collapsed;
            PageFeatures.Visibility = _currentPage == WizardPage.Features ? Visibility.Visible : Visibility.Collapsed;
            PageOptions.Visibility = _currentPage == WizardPage.Options ? Visibility.Visible : Visibility.Collapsed;
            PageInstalling.Visibility = _currentPage == WizardPage.Installing ? Visibility.Visible : Visibility.Collapsed;
            PageFinish.Visibility = _currentPage == WizardPage.Finish ? Visibility.Visible : Visibility.Collapsed;
            PageUninstallWelcome.Visibility = _currentPage == WizardPage.UninstallWelcome ? Visibility.Visible : Visibility.Collapsed;
            PageUninstalling.Visibility = _currentPage == WizardPage.Uninstalling ? Visibility.Visible : Visibility.Collapsed;
            PageUninstallFinish.Visibility = _currentPage == WizardPage.UninstallFinish ? Visibility.Visible : Visibility.Collapsed;

            // Update Back / Next / Cancel buttons
            BtnBack.Visibility = (_currentPage == WizardPage.Welcome || _currentPage == WizardPage.Installing || _currentPage == WizardPage.Finish || _currentPage == WizardPage.UninstallWelcome || _currentPage == WizardPage.Uninstalling || _currentPage == WizardPage.UninstallFinish)
                ? Visibility.Collapsed : Visibility.Visible;

            if (_currentPage == WizardPage.Finish || _currentPage == WizardPage.UninstallFinish)
            {
                BtnNext.Visibility = Visibility.Visible;
                BtnNext.Content = "Finish";
                BtnNext.IsEnabled = true;
                BtnCancel.Visibility = Visibility.Collapsed;
            }
            else if (_currentPage == WizardPage.Installing || _currentPage == WizardPage.Uninstalling)
            {
                BtnNext.Visibility = Visibility.Collapsed;
                BtnCancel.Visibility = Visibility.Collapsed;
            }
            else if (_currentPage == WizardPage.UninstallWelcome)
            {
                BtnNext.Visibility = Visibility.Visible;
                BtnNext.Content = "Uninstall";
                BtnNext.IsEnabled = true;
                BtnCancel.Visibility = Visibility.Visible;
            }
            else
            {
                BtnNext.Visibility = Visibility.Visible;
                BtnNext.Content = "Next >";
                BtnCancel.Visibility = Visibility.Visible;

                if (_currentPage == WizardPage.License)
                {
                    BtnNext.IsEnabled = ChkAgree.IsChecked == true;
                }
                else
                {
                    BtnNext.IsEnabled = true;
                }
            }
        }

        private void BtnNext_Click(object sender, RoutedEventArgs e)
        {
            if (_currentPage == WizardPage.Finish)
            {
                Close();
                return;
            }
            if (_currentPage == WizardPage.UninstallFinish)
            {
                Close();
                return;
            }

            switch (_currentPage)
            {
                case WizardPage.Welcome:
                    _currentPage = WizardPage.License;
                    break;
                case WizardPage.License:
                    if (_isExistingInstallDetected)
                    {
                        _currentPage = WizardPage.Upgrade;
                    }
                    else
                    {
                        _currentPage = WizardPage.Features;
                    }
                    break;
                case WizardPage.Upgrade:
                    _currentPage = WizardPage.Features;
                    break;
                case WizardPage.Features:
                    _currentPage = WizardPage.Options;
                    break;
                case WizardPage.Options:
                    _currentPage = WizardPage.Installing;
                    UpdateWizardState();
                    StartInstallation();
                    return; // Avoid updating state again since StartInstallation handles it
                case WizardPage.UninstallWelcome:
                    _currentPage = WizardPage.Uninstalling;
                    UpdateWizardState();
                    StartUninstallation();
                    return;
            }

            UpdateWizardState();
        }

        private void BtnBack_Click(object sender, RoutedEventArgs e)
        {
            switch (_currentPage)
            {
                case WizardPage.License:
                    _currentPage = WizardPage.Welcome;
                    break;
                case WizardPage.Upgrade:
                    _currentPage = WizardPage.License;
                    break;
                case WizardPage.Features:
                    if (_isExistingInstallDetected)
                    {
                        _currentPage = WizardPage.Upgrade;
                    }
                    else
                    {
                        _currentPage = WizardPage.License;
                    }
                    break;
                case WizardPage.Options:
                    _currentPage = WizardPage.Features;
                    break;
            }

            UpdateWizardState();
        }

        private void BtnCancel_Click(object sender, RoutedEventArgs e)
        {
            string title = _isUninstallMode ? "Cancel Uninstallation" : "Cancel Setup";
            string msg = _isUninstallMode ? "Are you sure you want to cancel the uninstallation?" : "Are you sure you want to cancel the installation?";
            var result = MessageBox.Show(this, msg, title, MessageBoxButton.YesNo, MessageBoxImage.Warning);
            if (result == MessageBoxResult.Yes)
            {
                Close();
            }
        }

        private void ChkAgree_CheckedChanged(object sender, RoutedEventArgs e)
        {
            if (BtnNext != null)
            {
                BtnNext.IsEnabled = ChkAgree.IsChecked == true;
            }
        }

        private void BtnBrowse_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new Microsoft.Win32.SaveFileDialog
            {
                Title = "Select Installation Directory",
                Filter = "Directories|*.none",
                FileName = "Select Folder",
                InitialDirectory = TxtInstallDir.Text
            };

            if (dialog.ShowDialog(this) == true)
            {
                string? folder = Path.GetDirectoryName(dialog.FileName);
                if (!string.IsNullOrEmpty(folder))
                {
                    TxtInstallDir.Text = folder;
                }
            }
        }

        private async void StartInstallation()
        {
            try
            {
                TxtInstallingStatus.Text = "Extracting temporary installer files...";

                string tempDir = Path.Combine(Path.GetTempPath(), "VortexInstallTemp_" + Guid.NewGuid().ToString("N"));
                Directory.CreateDirectory(tempDir);

                string tempInstallerPath = Path.Combine(tempDir, "vortex_nsis_installer.exe");

                // Extract embedded NSIS installer resource
                var assembly = Assembly.GetExecutingAssembly();
                using (Stream resourceStream = assembly.GetManifestResourceStream("VortexSetup.nsis_installer.exe")!)
                {
                    if (resourceStream == null)
                    {
                        throw new Exception("Could not find embedded NSIS installer in resources.");
                    }

                    using (FileStream fileStream = new FileStream(tempInstallerPath, FileMode.Create, FileAccess.Write))
                    {
                        await resourceStream.CopyToAsync(fileStream);
                    }
                }

                TxtInstallingStatus.Text = "Running system configuration and installing files...";

                // Build NSIS command-line arguments
                // Always runs silently
                string arguments = "/S";

                // Add component options
                arguments += $" /DESKTOP={(ChkDesktopShortcut.IsChecked == true ? "1" : "0")}";
                arguments += $" /STARTMENU={(ChkStartMenuShortcut.IsChecked == true ? "1" : "0")}";
                arguments += $" /STARTUP={(ChkStartup.IsChecked == true ? "1" : "0")}";
                arguments += $" /FIREWALL={(ChkFirewall.IsChecked == true ? "1" : "0")}";
                arguments += $" /PATHLIMIT={(ChkPathLimit.IsChecked == true ? "1" : "0")}";

                // Upgrade vs Reinstall option
                if (_isExistingInstallDetected && RadioCleanInstall.IsChecked == true)
                {
                    arguments += " /REINSTALL=1";
                }
                else
                {
                    arguments += " /REINSTALL=0";
                }

                // NSIS requires /D to be the final argument.
                if (!string.IsNullOrWhiteSpace(TxtInstallDir.Text))
                {
                    arguments += $" /D={TxtInstallDir.Text}";
                }

                // Run the NSIS installer silently
                ProcessStartInfo psi = new ProcessStartInfo
                {
                    FileName = tempInstallerPath,
                    Arguments = arguments,
                    UseShellExecute = true, // Required for running as admin via runas
                    Verb = "runas" // Elevate privilege to admin
                };

                using (Process process = new Process { StartInfo = psi })
                {
                    process.Start();

                    // Wait asynchronously for the process to exit
                    await Task.Run(() => process.WaitForExit());

                    if (process.ExitCode != 0)
                    {
                        throw new Exception($"Installation process returned error code {process.ExitCode}");
                    }
                }

                // Save WebUI and Network configuration choices
                SaveConfiguration();

                string installDir = SetupBackend.GetInstalledPathFromRegistry() ?? TxtInstallDir.Text;
                if (!string.IsNullOrWhiteSpace(installDir) && Directory.Exists(installDir))
                {
                    SetupBackend.CopySelfAsUninstaller(installDir);
                }

                // Cleanup temp files
                try
                {
                    File.Delete(tempInstallerPath);
                    Directory.Delete(tempDir, true);
                }
                catch (Exception cleanupEx)
                {
                    Debug.WriteLine($"Warning: Temp cleanup failed: {cleanupEx.Message}");
                }

                _currentPage = WizardPage.Finish;
                UpdateWizardState();
            }
            catch (Exception ex)
            {
                MessageBox.Show(this, $"An error occurred during installation:\n{ex.Message}", "Installation Failed", MessageBoxButton.OK, MessageBoxImage.Error);
                _currentPage = WizardPage.Options;
                UpdateWizardState();
            }
        }

        private void SaveConfiguration()
        {
            try
            {
                string appData = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
                string vortexDir = Path.Combine(appData, "Vortex");
                string iniPath = Path.Combine(vortexDir, "Vortex.ini");

                if (!Directory.Exists(vortexDir))
                {
                    Directory.CreateDirectory(vortexDir);
                }

                System.Collections.Generic.List<string> lines = new System.Collections.Generic.List<string>();
                if (File.Exists(iniPath))
                {
                    lines.AddRange(File.ReadAllLines(iniPath));
                }

                // Filter out keys we are about to overwrite
                for (int i = lines.Count - 1; i >= 0; i--)
                {
                    string line = lines[i].Trim();
                    if (line.StartsWith("WebUI\\Enabled") ||
                        line.StartsWith("WebUI\\Port") ||
                        line.StartsWith("WebUI\\LocalHostAuth") ||
                        line.StartsWith("WebUI\\Username") ||
                        line.StartsWith("WebUI\\Password_PBKDF2") ||
                        line.StartsWith("Connection\\MobileDataModeActive"))
                    {
                        lines.RemoveAt(i);
                    }
                }

                // Find [Preferences] section
                int prefIdx = -1;
                for (int i = 0; i < lines.Count; i++)
                {
                    if (lines[i].Trim() == "[Preferences]")
                    {
                        prefIdx = i;
                        break;
                    }
                }

                System.Collections.Generic.List<string> newSettings = new System.Collections.Generic.List<string>();

                // Add Connection settings
                bool isMobile = RadioMobileNet.IsChecked == true;
                newSettings.Add(string.Format("Connection\\MobileDataModeActive={0}", isMobile ? "true" : "false"));

                // Add WebUI settings
                newSettings.Add("WebUI\\Enabled=false");

                if (prefIdx != -1)
                {
                    lines.InsertRange(prefIdx + 1, newSettings);
                }
                else
                {
                    lines.Add("[Preferences]");
                    lines.AddRange(newSettings);
                }

                // Ensure LegalNotice accepted is written
                int legalNoticeIdx = -1;
                for (int i = 0; i < lines.Count; i++)
                {
                    if (lines[i].Trim() == "[LegalNotice]")
                    {
                        legalNoticeIdx = i;
                        break;
                    }
                }

                if (legalNoticeIdx != -1)
                {
                    bool hasAccepted = false;
                    for (int j = legalNoticeIdx + 1; j < lines.Count; j++)
                    {
                        if (lines[j].StartsWith("[")) break;
                        if (lines[j].Trim().Replace(" ", "").Equals("Accepted=true", StringComparison.OrdinalIgnoreCase))
                        {
                            hasAccepted = true;
                            break;
                        }
                    }
                    if (!hasAccepted)
                    {
                        lines.Insert(legalNoticeIdx + 1, "Accepted=true");
                    }
                }
                else
                {
                    lines.Add("[LegalNotice]");
                    lines.Add("Accepted=true");
                }

                File.WriteAllLines(iniPath, lines.ToArray());
                Debug.WriteLine("Vortex.ini successfully updated by setup wrapper.");
            }
            catch (Exception ex)
            {
                Debug.WriteLine("Failed to write configuration: " + ex.Message);
            }
        }

        private async void StartUninstallation()
        {
            try
            {
                TxtUninstallingStatus.Text = "Running uninstaller...";

                string targetDir = AppDomain.CurrentDomain.BaseDirectory;
                string coreUninstPath = Path.Combine(targetDir, "uninst_core.exe");

                if (!File.Exists(coreUninstPath))
                {
                    targetDir = TxtInstallDir.Text;
                    SetupBackend.EnsureCoreUninstaller(targetDir);
                    coreUninstPath = Path.Combine(targetDir, "uninst_core.exe");
                }

                if (!File.Exists(coreUninstPath))
                {
                    throw new Exception("Could not find core uninstaller executable uninst_core.exe.");
                }

                string uninstallArguments = BuildUninstallArguments();

                // Run the core NSIS uninstaller silently with the selected components
                ProcessStartInfo psi = new ProcessStartInfo
                {
                    FileName = coreUninstPath,
                    Arguments = uninstallArguments,
                    UseShellExecute = true,
                    Verb = "runas" // Elevate privilege to admin
                };

                using (Process process = new Process { StartInfo = psi })
                {
                    process.Start();
                    await Task.Run(() => process.WaitForExit());

                    if (process.ExitCode != 0)
                    {
                        throw new Exception($"Uninstallation process returned error code {process.ExitCode}");
                    }
                }

                // Delete self after a short delay
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

                _currentPage = WizardPage.UninstallFinish;
                UpdateWizardState();
            }
            catch (Exception ex)
            {
                MessageBox.Show(this, string.Format("An error occurred during uninstallation:\n{0}", ex.Message), "Uninstallation Failed", MessageBoxButton.OK, MessageBoxImage.Error);
                _currentPage = WizardPage.UninstallWelcome;
                UpdateWizardState();
            }
        }

        private string BuildUninstallArguments()
        {
            return string.Format(
                "/S /REMOVE_FIREWALL={0} /REMOVE_CONF={1} /REMOVE_CACHE={2}",
                ChkUninstallFirewall.IsChecked == true ? "1" : "0",
                ChkUninstallConfig.IsChecked == true ? "1" : "0",
                ChkUninstallCache.IsChecked == true ? "1" : "0");
        }
    }
}
