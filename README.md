# Password Vault

**English** | [Türkçe](README.tr.md)

A desktop password manager written in C++20 and Qt 6. All entries are kept in a single encrypted vault file, unlocked with one master password.

> 🚧 Work in progress. This README is the project plan and will be completed at v1.0.0.

## Plan

### MVP
- **Master password:** create a vault with a strong master password (length and variety checks). The key is derived with **Argon2id** (libsodium); the master password itself is never stored.
- **Encrypted vault file:** the whole vault (titles, user names, passwords, URLs, notes) is encrypted with **XChaCha20-Poly1305**. Any change to the file is detected and refused. Saving is atomic, so a crash cannot corrupt the vault.
- **Entries:** title, user name, password, URL, notes, category, favourite; add, edit, delete; search and category filter.
- **Password generator:** length, character sets, ambiguous characters off, or a passphrase of random words; strength meter.
- **Clipboard:** copy user name or password; the clipboard is cleared automatically after 30 seconds.
- **Locking:** lock button and auto-lock after inactivity; the key is wiped from memory when locked. Wrong master password attempts are slowed down.
- **Change master password.**
- **Desktop app:** dark and light theme, Turkish and English, icon, version, About window, vault in the user's folder, Windows installer.
- **Tests:** encryption round trip, tampering detection, wrong password, generator rules, strength scoring, with Qt Test.

### Extras
- **Password health report:** weak, reused and old passwords with an overall score.
- **Breach check (Have I Been Pwned):** k-anonymity; only the first 5 characters of the password's SHA-1 hash are sent. Optional, needs internet.
- **Two-factor codes (TOTP):** 6-digit codes with a countdown for entries that have a TOTP secret.
- **Import and export:** import from Chrome, Edge and Bitwarden CSV; export an encrypted backup.
- **Demo vault:** a separate sample vault with about 30 fictional entries, opened with a known password.

### Future Plans
- Browser extension for auto-fill.
- Sync between devices.
- Attachments (files) in entries.
- Windows Hello unlock.

## Tech Stack
- C++20, Qt 6 (Widgets, Network, Test)
- libsodium (Argon2id, XChaCha20-Poly1305)
- CMake, MinGW-w64 (MSYS2 UCRT64)
- windeployqt, Inno Setup
