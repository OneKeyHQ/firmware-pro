<p align="center">
<img width="200" src="https://github.com/rayston92/graph_bed/blob/e3b2c938fc5b17d68531f69178908afb16266e6a/img/onekey_logo_badge_border.png?raw=trueg"/>
</p>

---

[![Github Stars](https://img.shields.io/github/stars/OneKeyHQ/firmware-pro?t&logo=github&style=for-the-badge&labelColor=000)](https://github.com/OneKeyHQ/firmware-pro/stargazers)
[![Version](https://img.shields.io/github/release/OneKeyHQ/firmware-pro.svg?style=for-the-badge&labelColor=000)](https://github.com/OneKeyHQ/firmware-pro/releases)
[![](https://img.shields.io/github/contributors-anon/OneKeyHQ/firmware-pro?style=for-the-badge&labelColor=000)](https://github.com/OneKeyHQ/firmware-pro/graphs/contributors)
[![Last commit](https://img.shields.io/github/last-commit/OneKeyHQ/firmware-pro.svg?style=for-the-badge&labelColor=000)](https://github.com/OneKeyHQ/firmware-pro/commits/onekey)
[![Issues](https://img.shields.io/github/issues-raw/OneKeyHQ/firmware-pro.svg?style=for-the-badge&labelColor=000)](https://github.com/OneKeyHQ/firmware-pro/issues?q=is%3Aissue+is%3Aopen)
[![Pull Requests](https://img.shields.io/github/issues-pr-raw/OneKeyHQ/firmware-pro.svg?style=for-the-badge&labelColor=000)](https://github.com/OneKeyHQ/firmware-pro/pulls?q=is%3Apr+is%3Aopen)
[![Twitter Follow](https://img.shields.io/twitter/follow/OneKeyHQ?style=for-the-badge&labelColor=000)](https://twitter.com/OneKeyHQ)


## Community & Support

- [Community Forum](https://github.com/orgs/OneKeyHQ/discussions). Best for: help with building, discussion about best practices.
- [GitHub Issues](https://github.com/OneKeyHQ/firmware-pro/issues). Best for: bugs and errors you encounter using OneKey.


## 🚀 Getting Onboard

1. Install [nix](https://nixos.org/download.html)
2. Pulling the latest code via the git command line tool,  setting up the development environment

```
  git clone --recurse-submodules https://github.com/OneKeyHQ/onekey-firmware.git
  cd onekey-firmware
  nix-shell
  poetry install
```

3. Run the build with:

```
   cd core && poetry run make build_unix
```

4. Now you can start the emulator

```
   poetry run ./emu.py
```

5. You can now install the command line client utility to interact with the emulator

```
   cd python && poetry run python3 -m pip install .
```

## ✏ Contribute

- Adding a small feature or a fix

  If your change is somewhat subtle, feel free to file a PR in one of the appropriate repositories directly. See the PR requirements noted at [CONTRIBUTING.md](docs/misc/contributing.md)

- Add new coin/token/network to the official onekey firmware

  See [COINS.md](docs/misc/COINS.md)

Also please have a look at the [docs](docs/SUMMARY.md) before contributing. The misc chapter should be read in particular because it contains some useful assorted knowledge.

## 🔒 Security

- Please read [Bug Bounty Rules](https://github.com/OneKeyHQ/app-monorepo/blob/onekey/docs/BUG_RULES.md), we have detailed the exact plan in this article.
- Please report suspected security vulnerabilities in private to dev@onekey.so
- Please do NOT create publicly viewable issues for suspected security vulnerabilities.
- As an open source project, although we are not yet profitable, we try to give some rewards to white hat hackers who disclose vulnerabilities to us in a timely manner.
