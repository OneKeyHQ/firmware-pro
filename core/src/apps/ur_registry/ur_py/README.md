# [Foundation Devices Python UR Library](https://github.com/Foundation-Devices/foundation-ur-py)

**UR Implementation in Python -- ported from the [C++ Reference Implementation by Blockchain Commons](https://github.com/BlockchainCommons/bc-ur)**

## Introduction

URs ("Uniform Resources") are a method for encoding structured binary data for transport in URIs and QR Codes. They are described in [BCR-2020-005](https://github.com/BlockchainCommons/Research/blob/master/papers/bcr-2020-005-ur.md).

There is also another reference implementation in Swift: [URKit](https://github.com/blockchaincommons/URKit), and a demo app that uses it to display and read multi-part animated QR codes: [URDemo](https://github.com/blockchaincommons/URDemo).

## Installation

The code is not yet available in a package format, so just copy the files into your project.

### Dependencies

Either `hashlib` in a normal Python environment or `uhashlib` in MicroPython must be available.

## Use

1. Include the source folder in your Python project

2. Import the encoder and decoder:
    ```
    from ur.ur_encoder import UREncoder
    from ur.ur_decoder import URDecoder
    ```

3. Write some test code:

    ```
        ur = make_message_ur(32767)
        max_fragment_len = 1000
        first_seq_num = 100
        encoder = UREncoder(ur, max_fragment_len, first_seq_num)
        decoder = URDecoder()
        while True:
            part = encoder.next_part()
            decoder.receive_part(part)
            if decoder.is_complete():
                break

        if decoder.is_success():
            assert(decoder.result == ur)
        else:
            print('{}'.format(decoder.result))
            assert(False)
    ```

## Notes for Maintainers

Before accepting a PR that can affect build or unit tests, make sure the following command succeeds:

```
python test.py
```

Ensure that you add new unit tests for new or modified functionality.

## Origin, Authors, Copyright & Licenses

Unless otherwise noted (either in this [/README.md](./README.md) or in the file's header comments) the contents of this repository are Copyright © 2020 Foundation Devices, Inc., and are [licensed](./LICENSE) under the [spdx:BSD-2-Clause Plus Patent License](https://spdx.org/licenses/BSD-2-Clause-Patent.html).

This code is a Python port of the original C++ reference implementation by Blockchain Commons.  See
[Blockchain Commons UR Library](https://github.com/BlockchainCommons/bc-ur) for the original version.

## Contributing

TBD

We encourage public contributions through issues and pull-requests! Please review [CONTRIBUTING.md](./CONTRIBUTING.md) for details on our development process. All contributions to this repository require a GPG signed [Contributor License Agreement](./CLA.md).

### Questions & Support

If you have questions or problems, please use this repository's [issues](./issues) feature.

### Credits

The following people directly contributed to this repository. You can add your name here by getting involved — the first step is to learn how to contribute from our [CONTRIBUTING.md](./CONTRIBUTING.md) documentation.

| Name          | Role         | Github                                             | Email                         | GPG Fingerprint |
| ------------- | ------------ | -------------------------------------------------- | ----------------------------- | --------------- |
| Ken Carpenter | Initial Port | [@FoundationKen](https://github.com/FoundationKen) | \<ken@foundationdevices.com\> | TBD             |

## Responsible Disclosure

We want to keep all our software safe for everyone. If you have discovered a security vulnerability, we appreciate your help in disclosing it to us in a responsible manner. We are unfortunately not able to offer bug bounties at this time.

We do ask that you offer us good faith and use best efforts not to leak information or harm any user, their data, or our developer community. Please give us a reasonable amount of time to fix the issue before you publish it. Do not defraud our users or us in the process of discovery. We promise not to bring legal action against researchers who point out a problem provided they do their best to follow the these guidelines.

### Reporting a Vulnerability

Please report suspected security vulnerabilities in private via email to ken@foundationdevices.com (do not use this email for support). Please do NOT create publicly viewable issues for suspected security vulnerabilities.

The following keys may be used to communicate sensitive information to developers:

| Name                      | Fingerprint   |
| ------------------------- | ------------- |
| ken@foundationdevices.com | Coming Soon.. |

You can import a key by running the following command with that individual’s fingerprint: `gpg --recv-keys "<fingerprint>"` Ensure that you put quotes around fingerprints that contain spaces.

## Version History

### 0.1.0, 08/20/2020 - Initial release

* Initial testing release.
