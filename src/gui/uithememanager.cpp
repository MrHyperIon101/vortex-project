/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2023-2026  Vladimir Golovnev <glassez@yandex.ru>
 * Copyright (C) 2019, 2021  Prince Gupta <jagannatharjun11@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link this program with the OpenSSL project's "OpenSSL" library (or with
 * modified versions of it that use the same license as the "OpenSSL" library),
 * and distribute the linked executables. You must obey the GNU General Public
 * License in all respects for all of the code used other than "OpenSSL".  If
 * you modify file(s), you may extend this exception to your version of the
 * file(s), but you are not obligated to do so. If you do not wish to do so,
 * delete this exception statement from your version.
 */

#include "uithememanager.h"

#include <QApplication>
#include <QFile>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPixmapCache>
#include <QResource>
#include <QIconEngine>
#include <QStyle>
#include <QStyleHints>
#include <QSvgRenderer>

#include <QDebug>
#include "base/logger.h"
#include "base/path.h"
#include "base/preferences.h"
#include "uithemecommon.h"

#ifdef Q_OS_WIN
#include "base/utils/reg.h"
#endif

using namespace Qt::Literals::StringLiterals;

namespace
{
    const UIThemeManager *g_activeManager = nullptr;
    Path g_registeredThemePath;

    bool isDarkTheme()
    {
        if (g_activeManager)
        {
#ifdef QBT_HAS_COLORSCHEME_OPTION
            const ColorScheme scheme = g_activeManager->colorScheme();
            if (scheme == ColorScheme::Dark)
                return true;
            if (scheme == ColorScheme::Light)
                return false;
#endif
        }
        return (qApp && qApp->styleHints()) ? (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark) : false;
    }

    class DynamicIconEngine : public QIconEngine
    {
    public:
        DynamicIconEngine(const QString &iconId, const QString &fallback)
            : m_iconId(iconId)
            , m_fallback(fallback)
        {}

        void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override
        {
            if (g_activeManager)
            {
                QIcon icon = g_activeManager->getActualIcon(m_iconId, m_fallback);
                icon.paint(painter, rect, Qt::AlignCenter, mode, state);
            }
        }

        QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override
        {
            if (g_activeManager)
            {
                QIcon icon = g_activeManager->getActualIcon(m_iconId, m_fallback);
                return icon.pixmap(size, mode, state);
            }
            return QPixmap();
        }

        QIconEngine *clone() const override
        {
            return new DynamicIconEngine(m_iconId, m_fallback);
        }

    private:
        QString m_iconId;
        QString m_fallback;
    };
}

UIThemeManager *UIThemeManager::m_instance = nullptr;

void UIThemeManager::freeInstance()
{
    delete m_instance;
    m_instance = nullptr;
    g_activeManager = nullptr;
}

void UIThemeManager::initInstance()
{
    if (!m_instance)
        m_instance = new UIThemeManager;
}

UIThemeManager::UIThemeManager()
    : m_useCustomTheme {false}
#ifdef QBT_HAS_COLORSCHEME_OPTION
    , m_colorSchemeSetting {u"Appearance/ColorScheme"_s}
#endif
    , m_trayIconStyleSetting {u"Appearance/TrayIconStyle"_s}
#if (defined(Q_OS_UNIX) && !defined(Q_OS_MACOS))
    , m_useSystemIcons {Preferences::instance()->useSystemIcons()}
#endif
{
    g_activeManager = this;
#ifdef Q_OS_WIN
    updateSystemColorMode();
#endif

    if (!QApplication::setStyle(u"Fusion"_s))
        LogMsg(tr("Set app style failed. Unknown style: \"Fusion\""), Log::WARNING);

#ifdef QBT_HAS_COLORSCHEME_OPTION
    applyColorScheme();
#endif

    connect(QApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, &UIThemeManager::onColorSchemeChanged);

    m_themeSource = std::make_unique<DefaultThemeSource>();

    applyStyleSheet();
}

UIThemeManager *UIThemeManager::instance()
{
    return m_instance;
}

#ifdef QBT_HAS_COLORSCHEME_OPTION
ColorScheme UIThemeManager::colorScheme() const
{
    return m_colorSchemeSetting.get(ColorScheme::System);
}

void UIThemeManager::setColorScheme(const ColorScheme value)
{
    if (value == colorScheme())
        return;

    m_colorSchemeSetting = value;
    applyColorScheme();
    onColorSchemeChanged();
}

void UIThemeManager::applyColorScheme() const
{
    switch (colorScheme())
    {
    case ColorScheme::System:
    default:
        qApp->styleHints()->unsetColorScheme();
        break;
    case ColorScheme::Light:
        qApp->styleHints()->setColorScheme(Qt::ColorScheme::Light);
        break;
    case ColorScheme::Dark:
        qApp->styleHints()->setColorScheme(Qt::ColorScheme::Dark);
        break;
    }
}
#endif

TrayIconStyle UIThemeManager::trayIconStyle() const
{
    return m_trayIconStyleSetting.get(TrayIconStyle::Normal);
}

void UIThemeManager::setTrayIconStyle(const TrayIconStyle value)
{
    if (value == trayIconStyle())
        return;

    m_trayIconStyleSetting = value;
}

void UIThemeManager::applyStyleSheet() const
{
    if (!m_themeSource)
        return;
    QString qss = QString::fromUtf8(m_themeSource->readStyleSheet());
    if (qss.isEmpty())
        return;

    const bool dark = isDarkTheme();

    QHash<QString, QString> vars;
    if (dark)
    {
        vars[u"@window_bg"_s] = u"rgba(14, 14, 17, 0.85)"_s; // almost black (#0E0E11 with Mica opacity)
        vars[u"@sidebar_bg"_s] = u"#13131A"_s;
        vars[u"@panel_bg"_s] = u"#16161A"_s;
        vars[u"@item_bg"_s] = u"#1C1C21"_s;
        vars[u"@text_color"_s] = u"#F0F0F5"_s;
        vars[u"@text_secondary"_s] = u"#6B6B7A"_s;
        vars[u"@border_color"_s] = u"rgba(255, 255, 255, 0.06)"_s;
        vars[u"@button_bg"_s] = u"rgba(255, 255, 255, 0.08)"_s;
        vars[u"@button_hover"_s] = u"rgba(255, 255, 255, 0.12)"_s;
        vars[u"@button_pressed"_s] = u"rgba(255, 255, 255, 0.04)"_s;
        vars[u"@input_bg"_s] = u"#1C1C21"_s;
        vars[u"@item_hover"_s] = u"rgba(255, 255, 255, 0.05)"_s;
        vars[u"@accent_color"_s] = u"#6366F1"_s; // Vortex Indigo-500
        vars[u"@accent_hover"_s] = u"#4F46E5"_s; // Vortex Indigo-600
        vars[u"@selection_bg"_s] = u"rgba(99, 102, 241, 0.22)"_s;
        vars[u"@seeding_color"_s] = u"#22C55E"_s;
        vars[u"@download_color"_s] = u"#3B82F6"_s;
        vars[u"@error_color"_s] = u"#FF4D4D"_s;
        vars[u"@header_bg"_s] = u"#16161A"_s;
        vars[u"@scrollbar_handle"_s] = u"rgba(255, 255, 255, 0.15)"_s;
        vars[u"@scrollbar_hover"_s] = u"rgba(255, 255, 255, 0.3)"_s;
        vars[u"@progress_bg"_s] = u"rgba(255, 255, 255, 0.06)"_s;
    }
    else
    {
        vars[u"@window_bg"_s] = u"rgba(242, 242, 247, 0.85)"_s; // Light gray (#F2F2F7 with Mica opacity)
        vars[u"@sidebar_bg"_s] = u"#FFFFFF"_s;
        vars[u"@panel_bg"_s] = u"#FFFFFF"_s;
        vars[u"@item_bg"_s] = u"#F8F8FC"_s;
        vars[u"@text_color"_s] = u"#111114"_s;
        vars[u"@text_secondary"_s] = u"#8A8A98"_s;
        vars[u"@border_color"_s] = u"rgba(0, 0, 0, 0.08)"_s;
        vars[u"@button_bg"_s] = u"rgba(0, 0, 0, 0.03)"_s;
        vars[u"@button_hover"_s] = u"rgba(0, 0, 0, 0.06)"_s;
        vars[u"@button_pressed"_s] = u"rgba(0, 0, 0, 0.1)"_s;
        vars[u"@input_bg"_s] = u"#F8F8FC"_s;
        vars[u"@item_hover"_s] = u"rgba(0, 0, 0, 0.03)"_s;
        vars[u"@accent_color"_s] = u"#4F46E5"_s; // Vortex Indigo-600
        vars[u"@accent_hover"_s] = u"#3730A3"_s; // Vortex Indigo-800
        vars[u"@selection_bg"_s] = u"rgba(79, 70, 229, 0.12)"_s;
        vars[u"@seeding_color"_s] = u"#22C55E"_s;
        vars[u"@download_color"_s] = u"#3B82F6"_s;
        vars[u"@error_color"_s] = u"#FF4D4D"_s;
        vars[u"@header_bg"_s] = u"#F8F8FC"_s;
        vars[u"@scrollbar_handle"_s] = u"rgba(0, 0, 0, 0.15)"_s;
        vars[u"@scrollbar_hover"_s] = u"rgba(0, 0, 0, 0.3)"_s;
        vars[u"@progress_bg"_s] = u"rgba(0, 0, 0, 0.06)"_s;
    }

    for (auto it = vars.constBegin(); it != vars.constEnd(); ++it)
        qss.replace(it.key(), it.value());

    if (qApp)
        qApp->setStyleSheet(qss);
}

void UIThemeManager::onColorSchemeChanged()
{
    // Clear icon caches so SVG icons are re-rendered with correct theme colors
    m_icons.clear();
    m_darkModeIcons.clear();
    QPixmapCache::clear();

    applyStyleSheet();
    emit themeChanged();

    // workaround to refresh styled controls once color scheme is changed
    QApplication::setStyle(QApplication::style()->name());
}

void UIThemeManager::reloadTheme()
{
    m_useCustomTheme = false;
#if (defined(Q_OS_UNIX) && !defined(Q_OS_MACOS))
    m_useSystemIcons = Preferences::instance()->useSystemIcons();
#endif

    // Force Fusion style
    QApplication::setStyle(u"Fusion"_s);

    // Unregister the previous theme resource if registered
    if (!g_registeredThemePath.isEmpty())
    {
        QResource::unregisterResource(g_registeredThemePath.data(), u"/uitheme"_s);
        g_registeredThemePath = Path();
    }

    // Reset the theme source to default
    m_themeSource = std::make_unique<DefaultThemeSource>();

    // Clear caches
    m_icons.clear();
    m_darkModeIcons.clear();
    m_flags.clear();
    QPixmapCache::clear();

    qApp->setPalette(qApp->style()->standardPalette());

    applyStyleSheet();
    emit themeChanged();

    // Workaround to refresh styled controls
    QApplication::setStyle(QApplication::style()->name());
}

QIcon UIThemeManager::getActualIcon(const QString &iconId, [[maybe_unused]] const QString &fallback, const ColorMode colorMode) const
{
    auto &icons = (colorMode == ColorMode::Dark) ? m_darkModeIcons : m_icons;

    const auto iter = icons.find(iconId);
    if (iter != icons.end())
        return *iter;

#if (defined(Q_OS_UNIX) && !defined(Q_OS_MACOS))
    // Don't cache system icons because users might change them at run time
    if (m_useSystemIcons)
    {
        auto icon = QIcon::fromTheme(iconId);
        if (icon.isNull() || icon.availableSizes().isEmpty())
            icon = QIcon::fromTheme(fallback, QIcon(m_themeSource->getIconPath(iconId, colorMode).data()));
        return icon;
    }
#endif

    const QString iconPath = m_themeSource->getIconPath(iconId, colorMode).data();
    QIcon icon;

    // For SVG icons using currentColor, recolor based on dark/light mode
    if (iconPath.endsWith(u".svg", Qt::CaseInsensitive))
    {
        QFile svgFile(iconPath);
        if (svgFile.open(QIODevice::ReadOnly))
        {
            QByteArray svgData = svgFile.readAll();
            svgFile.close();

            // Replace currentColor with the appropriate color for the mode
            const QByteArray targetColor = (colorMode == ColorMode::Dark)
                    ? QByteArrayLiteral("#F0F0F5")   // light text for dark backgrounds
                    : QByteArrayLiteral("#111114");   // dark text for light backgrounds
            svgData.replace("currentColor", targetColor);

            // Render the recolored SVG at multiple sizes for crisp display
            QSvgRenderer renderer(svgData);
            if (renderer.isValid())
            {
                for (const int size : {16, 20, 24, 32})
                {
                    QPixmap pixmap(size, size);
                    pixmap.fill(Qt::transparent);
                    QPainter painter(&pixmap);
                    renderer.render(&painter);
                    painter.end();
                    icon.addPixmap(pixmap);
                }
            }
        }
    }

    if (icon.isNull())
        icon = QIcon(iconPath);

    icons[iconId] = icon;
    return icon;
}

QIcon UIThemeManager::getActualIcon(const QString &iconId, const QString &fallback) const
{
    const auto colorMode = isDarkTheme() ? ColorMode::Dark : ColorMode::Light;
    return getActualIcon(iconId, fallback, colorMode);
}

QIcon UIThemeManager::getIcon(const QString &iconId, const QString &fallback) const
{
    return QIcon(new DynamicIconEngine(iconId, fallback));
}

QIcon UIThemeManager::getSystrayIcon() const
{
#ifdef Q_OS_WIN
    const auto colorMode = m_systemColorMode;
#else
    const auto colorMode = (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark)
            ? ColorMode::Dark : ColorMode::Light;
#endif
    const QString fallback = (colorMode == ColorMode::Light)
            ? u"qbittorrent-tray-light"_s : u"qbittorrent-tray-dark"_s;

    switch (trayIconStyle())
    {
    case TrayIconStyle::Normal:
        return getActualIcon(u"qbittorrent-tray"_s, {}, colorMode);

    case TrayIconStyle::Monochrome:
        return getActualIcon(u"qbittorrent-tray-mono"_s, fallback, colorMode);
    }

    Q_UNREACHABLE_RETURN({});
}

QIcon UIThemeManager::getFlagIcon(const QString &countryIsoCode) const
{
    if (countryIsoCode.isEmpty())
        return {};

    const QString key = countryIsoCode.toLower();
    const auto iter = m_flags.constFind(key);
    if (iter != m_flags.cend())
        return *iter;

    const QIcon icon {u":/icons/flags/" + key + u".svg"};
    m_flags[key] = icon;
    return icon;
}

QPixmap UIThemeManager::getScaledPixmap(const QString &iconId, const int height) const
{
    // (workaround) svg images require the use of `QIcon()` to load and scale losslessly,
    // otherwise other image classes will convert it to pixmap first and follow-up scaling will become lossy.

    Q_ASSERT(height > 0);

    const QString cacheKey = iconId + u'@' + QString::number(height);

    QPixmap pixmap;
    if (!QPixmapCache::find(cacheKey, &pixmap))
    {
        pixmap = getIcon(iconId).pixmap(height);
        QPixmapCache::insert(cacheKey, pixmap);
    }

    return pixmap;
}

QColor UIThemeManager::getColor(const QString &id) const
{
    const QColor color = m_themeSource->getColor(id, (isDarkTheme() ? ColorMode::Dark : ColorMode::Light));
    return color;
}

#ifdef Q_OS_WIN
void UIThemeManager::updateSystemColorMode()
{
    const HKEY hkRoot = HKEY_CURRENT_USER;
    const REGSAM samDesired = KEY_READ;

    HKEY hkPersonalize = nullptr;
    const LSTATUS status = ::RegOpenKeyExW(hkRoot, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, samDesired, &hkPersonalize);
    if (status != ERROR_SUCCESS)
        return;

    [[maybe_unused]] const auto hkPersonalizeGuard = qScopeGuard([&hkPersonalize] { ::RegCloseKey(hkPersonalize); });

    const ulong val = Utils::Reg::getULongValue(hkPersonalize, u"SystemUsesLightTheme"_s);
    const auto systemUsesLightTheme = static_cast<bool>(val);
    const auto systemColorMode = systemUsesLightTheme ? ColorMode::Light : ColorMode::Dark;
    if (systemColorMode != m_systemColorMode)
    {
        m_systemColorMode = systemColorMode;
        onColorSchemeChanged();
    }
}
#endif

void UIThemeManager::applyPalette() const
{
    struct ColorDescriptor
    {
        QString id;
        QPalette::ColorRole colorRole;
        QPalette::ColorGroup colorGroup;
    };

    const ColorDescriptor paletteColorDescriptors[] =
    {
        {u"Palette.Window"_s, QPalette::Window, QPalette::Normal},
        {u"Palette.WindowText"_s, QPalette::WindowText, QPalette::Normal},
        {u"Palette.Base"_s, QPalette::Base, QPalette::Normal},
        {u"Palette.AlternateBase"_s, QPalette::AlternateBase, QPalette::Normal},
        {u"Palette.Text"_s, QPalette::Text, QPalette::Normal},
        {u"Palette.ToolTipBase"_s, QPalette::ToolTipBase, QPalette::Normal},
        {u"Palette.ToolTipText"_s, QPalette::ToolTipText, QPalette::Normal},
        {u"Palette.BrightText"_s, QPalette::BrightText, QPalette::Normal},
        {u"Palette.Highlight"_s, QPalette::Highlight, QPalette::Normal},
        {u"Palette.HighlightedText"_s, QPalette::HighlightedText, QPalette::Normal},
        {u"Palette.Button"_s, QPalette::Button, QPalette::Normal},
        {u"Palette.ButtonText"_s, QPalette::ButtonText, QPalette::Normal},
        {u"Palette.Link"_s, QPalette::Link, QPalette::Normal},
        {u"Palette.LinkVisited"_s, QPalette::LinkVisited, QPalette::Normal},
        {u"Palette.Light"_s, QPalette::Light, QPalette::Normal},
        {u"Palette.Midlight"_s, QPalette::Midlight, QPalette::Normal},
        {u"Palette.Mid"_s, QPalette::Mid, QPalette::Normal},
        {u"Palette.Dark"_s, QPalette::Dark, QPalette::Normal},
        {u"Palette.Shadow"_s, QPalette::Shadow, QPalette::Normal},
        {u"Palette.WindowTextDisabled"_s, QPalette::WindowText, QPalette::Disabled},
        {u"Palette.TextDisabled"_s, QPalette::Text, QPalette::Disabled},
        {u"Palette.ToolTipTextDisabled"_s, QPalette::ToolTipText, QPalette::Disabled},
        {u"Palette.BrightTextDisabled"_s, QPalette::BrightText, QPalette::Disabled},
        {u"Palette.HighlightedTextDisabled"_s, QPalette::HighlightedText, QPalette::Disabled},
        {u"Palette.ButtonTextDisabled"_s, QPalette::ButtonText, QPalette::Disabled}
    };

    QPalette palette = qApp->palette();
    for (const ColorDescriptor &colorDescriptor : paletteColorDescriptors)
    {
        // For backward compatibility, the palette color overrides are read from the section of the "light mode" colors
        const QColor newColor = m_themeSource->getColor(colorDescriptor.id, ColorMode::Light);
        if (newColor.isValid())
            palette.setColor(colorDescriptor.colorGroup, colorDescriptor.colorRole, newColor);
    }

    qApp->setPalette(palette);
}
