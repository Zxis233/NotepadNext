# Notepad Next

![Build Notepad Next](https://github.com/dail8859/NotepadNext/workflows/Build%20Notepad%20Next/badge.svg)

A cross-platform, reimplementation of Notepad++.

Though the application overall is stable and usable, it should not be considered safe for critically important work.

There are numerous bugs and half working implementations. Pull requests are greatly appreciated.

![screenshot](/doc/screenshot.png)

# Installation

Packages are available for Windows, Linux, and MacOS.

Below are the supported distribution mechanisms. There may be other ways to download/install the application, but this project will likely not be able to offer any support for those since they are made available by other individuals.

## Windows
Windows packages are available as an installer or a stand-alone zip file on the [release](https://github.com/dail8859/NotepadNext/releases) page. The installer provides additional components such as an auto-updater and Windows context menu integration. You can easily install it with Winget:

```powershell
winget install dail8859.NotepadNext
```

## Linux
Linux packages can be obtained by downloading the stand-alone AppImage on the [release](https://github.com/dail8859/NotepadNext/releases) page or by installing the [flatpak](https://flathub.org/apps/details/com.github.dail8859.NotepadNext) by executing:

```bash
flatpak install flathub com.github.dail8859.NotepadNext
```

If you are using Ubuntu and prefer an up-to-date deb version, you can use the [PPA supporting Ubuntu 22.04 and newer](https://launchpad.net/~quentiumyt/+archive/ubuntu/notepadnext) provided by
[Quentin Lienhardt](https://github.com/QuentiumYT). You can add it by executing:

```bash
sudo add-apt-repository ppa:quentiumyt/notepadnext
sudo apt update
sudo apt install notepadnext
```

## MacOS
MacOS disk images can be downloaded from the [release](https://github.com/dail8859/NotepadNext/releases) page.

It can also be installed using brew:
```bash
brew tap dail8859/notepadnext
brew install notepadnext
```

If you receive warnings that Notepad Next is "damaged", this is an Apple complaining that I have not paid them money to "trust" me. You can bypass this by running:

```bash
xattr -d com.apple.quarantine /Applications/NotepadNext.app
```

#### MacOS Tweaks

By default, MacOS enables font smoothing which causes text to appear quite differently from the Windows version. This can be disabled system-wide using the following command:

```bash
defaults -currentHost write -g AppleFontSmoothing -int 0
```

A restart is required for this to take effect.

# Appearance

Open **Settings > Preferences > Theme** and choose **Follow system**, **Light**,
or **Dark**. New installations follow the system by default. An explicitly saved
choice from the older Dark mode checkbox is preserved as Light or Dark.

Dark mode uses a GitHub Dark-inspired palette: a `#0d1117` editing background,
blue-gray interface surfaces, blue selections, coral keywords, light-blue
strings, and muted comments. Light mode uses a white editing background,
`#f6f8fa` interface surfaces, dark-gray text and blue selections. Its syntax
colors follow [GitHub's VS Code Light Default theme](https://github.com/primer/github-vscode-theme):
red keywords, dark-blue strings, purple functions, orange type/entity names,
green HTML tags and JSON property names, blue Markdown headings and green quotes.
JSON, HTML/XML, TOML, Markdown and Python have dedicated mappings where needed;
other languages use lexer metadata. Token boundaries depend on Lexilla, so this
does not reproduce every TextMate or language-server distinction: for example,
the C++ lexer shares one style for ordinary functions, user types and variables.
Both modes use the same layout and font settings.

Markdown headings share one color across all six levels. Code and GitHub-style
math expressions use distinct existing palette colors, including inline math,
`$$` blocks and `math` fences.

Changes take effect immediately across all document tabs and the Lua console,
and the selected mode is remembered across restarts. Follow system also reacts
to system appearance changes while the application is running on Qt 6.5 or later
when the platform reports them. On older Qt versions or platforms that do not
report a color scheme, the initial system palette provides the fallback.
A user-provided `custom.css` can override the application interface colors.

Under **Settings > Preferences > Default Font**, **Tab title font** selects
the default interface font or the editor's configured font family. Tab titles
keep the interface font size, independently of the editor's font size.
The choice is remembered and updates all document tabs immediately, including
tabs in split views. New and restored tabs use the same setting.

# File encodings

The **Encoding** menu supports UTF-8, UTF-8 BOM, UTF-16 LE/BE BOM, GBK,
and Shift_JIS. Each document keeps its own encoding; switching tabs updates
the menu selection and status bar.

- **Reopen with Encoding** reads the current file from disk again. Use it to
  correct text displayed with the wrong encoding. Unsaved changes require
  confirmation before they are discarded. A Unicode BOM takes precedence.
- **Convert to Encoding** preserves the current text and selects the encoding
  for the next save. Use **Save** or **Save All** to write the conversion.
  Characters that cannot be represented without loss cause an error instead
  of being replaced with question marks.

The encoding used to successfully reopen or save a file is remembered by its
full path, even after closing the file or restarting the application. Different
files with the same name keep separate choices. Save As, Save a Copy As, and
Rename also remember the encoding for the destination file. Selecting a
conversion without saving does not change the remembered on-disk encoding.

A Unicode BOM takes precedence over the remembered choice. Without a BOM or a
remembered choice, files initially use UTF-8. If a remembered codec can no longer
decode a changed file, opening falls back to the default behavior so another
encoding can be selected manually.
If the input is not valid UTF-8, its bytes are retained for reopening and saving
is blocked until the original encoding is selected. Session restoration keeps
each document's encoding and any pending encoding change; session snapshots
are decoded independently of this history.

# Translations

Translations are contributed by the community. All translations are managed using Crowdin at `https://crowdin.com/project/notepadnext`. If there is a language missing you would like to contribute, feel free to start a discussion on Crowdin.

# Development
Current development is done using QtCreator with the Microsoft Visual C++ (msvc) compiler. Qt 6.5 is the currently supported Qt version. Older versions of Qt are likely to work but are not tested. Any fixes for older versions will be accepted as long as they do not introduce complex fixes. This application is also known to build successfully on various Linux distributions and macOS. Other platforms/compilers should be usable with minor modifications.

If you are familiar with building C++ Qt desktop applications with Qt Creator, then this should be as simple as opening `CMakeLists` and build/run the project.

If you are new to building C++ Qt desktop applications, there is a more detailed guide [here](/doc/Building.md).


# License
This code is released under the [GNU General Public License version 3](https://www.gnu.org/licenses/gpl-3.0.txt).
