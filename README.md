<h3 align="center">OneKey Pro Firmware</h3>

<p align="center">
  Open-source firmware for the OneKey Pro hardware wallet.
  <br />
  Security-first · Fully verifiable · Community-driven
</p>

<p align="center">
  <a href="https://github.com/OneKeyHQ/firmware-pro/stargazers"><img src="https://img.shields.io/github/stars/OneKeyHQ/firmware-pro?logo=github&style=for-the-badge&labelColor=000" alt="Stars" /></a>
  <a href="https://github.com/OneKeyHQ/firmware-pro/releases"><img src="https://img.shields.io/github/release/OneKeyHQ/firmware-pro.svg?style=for-the-badge&labelColor=000" alt="Version" /></a>
  <a href="https://github.com/OneKeyHQ/firmware-pro/commits/onekey"><img src="https://img.shields.io/github/last-commit/OneKeyHQ/firmware-pro.svg?style=for-the-badge&labelColor=000" alt="Last commit" /></a>
  <br />
  <a href="https://github.com/OneKeyHQ/firmware-pro/graphs/contributors"><img src="https://img.shields.io/github/contributors-anon/OneKeyHQ/firmware-pro?style=for-the-badge&labelColor=000" alt="Contributors" /></a>
  <a href="https://github.com/OneKeyHQ/firmware-pro/issues?q=is%3Aissue+is%3Aopen"><img src="https://img.shields.io/github/issues-raw/OneKeyHQ/firmware-pro.svg?style=for-the-badge&labelColor=000" alt="Issues" /></a>
  <a href="https://github.com/OneKeyHQ/firmware-pro/pulls?q=is%3Apr+is%3Aopen"><img src="https://img.shields.io/github/issues-pr-raw/OneKeyHQ/firmware-pro.svg?style=for-the-badge&labelColor=000" alt="Pull Requests" /></a>
  <a href="https://twitter.com/OneKeyHQ"><img src="https://img.shields.io/twitter/follow/OneKeyHQ?style=for-the-badge&labelColor=000" alt="Twitter" /></a>
</p>

---

## About

This repository contains the firmware source code for the **OneKey Pro** hardware wallet. Every release is built via GitHub CI, multi-signed by the OneKey team, and can be independently verified against this open-source codebase — ensuring full supply chain transparency from code to device.

> **Verify your device's firmware:** see [Open Source Code Verification](https://help.onekey.so/en/articles/12025839-verifying-onekey-pro-firmware-with-open-source-code) for step-by-step instructions.

## Documentation

📖 &nbsp;**[Deep Wiki — Full Architecture & Codebase Guide](https://deepwiki.com/OneKeyHQ/firmware-pro/1-overview)**

[![DeepWiki](https://github.com/user-attachments/assets/9d7cc41f-17a2-4ba6-87eb-21118225e401)](https://deepwiki.com/OneKeyHQ/firmware-pro/1-overview)

## Getting Started

### Prerequisites

- [Nix](https://nixos.org/download.html) (package manager)
- Git

### Build & Run

```bash
# 1. Clone the repo (with submodules)
git clone --recurse-submodules https://github.com/OneKeyHQ/firmware-pro.git
cd firmware-pro

# 2. Enter the Nix development shell & install dependencies
nix-shell
poetry install

# 3. Build the Unix emulator
cd core && poetry run make build_unix

# 4. Start the emulator
poetry run ./emu.py

# 5. (Optional) Install the CLI client to interact with the emulator
cd ../python && poetry run python3 -m pip install .
```

## Contributing

We welcome contributions of all sizes. Before you start, please read the [contributing docs](docs/SUMMARY.md) — especially the [misc chapter](docs/misc/) for useful background knowledge.

- **Bug fixes & small features** — File a PR directly. See [CONTRIBUTING.md](docs/misc/contributing.md) for PR requirements.
- **New coin / token / network** — Follow the guide in [COINS.md](docs/misc/COINS.md).

## Security

> **If you discover a vulnerability, please report it responsibly.**

- 📬 &nbsp;Email **[security@onekey.so](mailto:security@onekey.so)** — do **not** open a public issue.
- 💰 &nbsp;We run a [Bug Bounty Program](https://github.com/OneKeyHQ/app-monorepo/blob/onekey/docs/BUG_RULES.md) to reward responsible disclosure.

## Community & Support

- 💬 &nbsp;[Community Forum](https://github.com/orgs/OneKeyHQ/discussions) — Questions, ideas, and best practices.
- 🐛 &nbsp;[GitHub Issues](https://github.com/OneKeyHQ/firmware-pro/issues) — Bug reports and feature requests.
- 🐦 &nbsp;[Twitter / X](https://twitter.com/OneKeyHQ) — News and announcements.
