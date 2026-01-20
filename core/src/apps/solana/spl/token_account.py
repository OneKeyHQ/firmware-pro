from typing import TYPE_CHECKING

from trezor.crypto import base58

from ..constents import SPL_ASSOCIATED_TOKEN_ACCOUNT_PROGRAM_ID

if TYPE_CHECKING:
    from trezor.messages import SolanaTxATADetails

SEED_CONSTANT = "ProgramDerivedAddress"


def try_finding_associated_token_account(
    owner_address: bytes,
    program_id: bytes,
    mint_address: bytes,
    associated_token_address: bytes,
) -> bool:
    from trezor.crypto.hashlib import sha256

    # based on the following sources:
    # https://spl.solana.com/associated-token-account#finding-the-associated-token-account-address
    # https://github.com/solana-labs/solana/blob/8fbe033eaca693ed8c3e90b19bc3f61b32885e5e/sdk/program/src/pubkey.rs#L495
    for seed_bump in range(255, 0, -1):
        seed = (
            owner_address
            + program_id
            + mint_address
            + bytes([seed_bump])
            + SPL_ASSOCIATED_TOKEN_ACCOUNT_PROGRAM_ID.get()
            + SEED_CONSTANT.encode("utf-8")
        )

        account = sha256(seed).digest()

        if account == associated_token_address:
            return True

    return False


def try_get_token_account_owner_address(
    token_account_address: bytes,
    token_program: bytes,
    token_mint: bytes,
    ata_details: list[SolanaTxATADetails],
) -> bytes | None:
    for ata_detail in ata_details:
        if (
            base58.decode(ata_detail.associated_token_address) == token_account_address
            and base58.decode(ata_detail.program_id) == token_program
            and base58.decode(ata_detail.mint_address) == token_mint
        ):
            owner_address = base58.decode(ata_detail.owner_address)

            if try_finding_associated_token_account(
                owner_address,
                token_program,
                token_mint,
                token_account_address,
            ):
                return owner_address

    return None
