from typing import TYPE_CHECKING

from trezor import loop, messages, utils, wire
from trezor.crypto import base58, bech32
from trezor.crypto.base58 import sha256d_32
from trezor.enums import InputScriptType, OutputScriptType, RequestType

from apps.ur_registry.chains import MismatchError
from apps.ur_registry.ur_py.ur.ur_encoder import UREncoder

from .crypto_psbt import CryptoPSBT
from .psbt.key import ExtendedPubKey
from .psbt.psbt import PSBT
from .psbt.script import is_p2pkh, is_p2sh, is_p2wsh, is_witness
from .psbt.tx import CTxOut

if TYPE_CHECKING:
    from typing import Tuple, Dict, Union, Optional, List
    from .psbt.key import KeyOriginInfo
    from .psbt.psbt import PartiallySignedInput, PartiallySignedOutput

    pass


ECDSA_SCRIPT_TYPES = [
    InputScriptType.SPENDADDRESS,
    InputScriptType.SPENDMULTISIG,
    InputScriptType.SPENDWITNESS,
    InputScriptType.SPENDP2SHWITNESS,
]
SCHNORR_SCRIPT_TYPES = [
    InputScriptType.SPENDTAPROOT,
]


class TxInputType:
    def __init__(
        self,
        *,
        prev_hash: "bytes",
        prev_index: "int",
        address_n: Optional[List["int"]] = None,
        script_sig: Optional["bytes"] = None,
        sequence: Optional["int"] = 4294967295,
        script_type: Optional["InputScriptType"] = InputScriptType.SPENDADDRESS,
        multisig: Optional[messages.MultisigRedeemScriptType] = None,
        amount: Optional["int"] = None,
        decred_tree: Optional["int"] = None,
        witness: Optional["bytes"] = None,
        ownership_proof: Optional["bytes"] = None,
        commitment_data: Optional["bytes"] = None,
        orig_hash: Optional["bytes"] = None,
        orig_index: Optional["int"] = None,
        script_pubkey: Optional["bytes"] = None,
    ) -> None:
        self.address_n = address_n if address_n is not None else []
        self.prev_hash = prev_hash
        self.prev_index = prev_index
        self.script_sig = script_sig
        self.sequence = sequence
        self.script_type = script_type
        self.multisig = multisig
        self.amount = amount
        self.decred_tree = decred_tree
        self.witness = witness
        self.ownership_proof = ownership_proof
        self.commitment_data = commitment_data
        self.orig_hash = orig_hash
        self.orig_index = orig_index
        self.script_pubkey = script_pubkey


class TxOutputType:
    def __init__(
        self,
        *,
        amount: "int",
        address_n: Optional[List["int"]] = None,
        address: Optional["str"] = None,
        script_type: Optional["OutputScriptType"] = OutputScriptType.PAYTOADDRESS,
        multisig: Optional[messages.MultisigRedeemScriptType] = None,
        op_return_data: Optional["bytes"] = None,
        orig_hash: Optional["bytes"] = None,
        orig_index: Optional["int"] = None,
    ) -> None:
        self.address_n = address_n if address_n is not None else []
        self.amount = amount
        self.address = address
        self.script_type = script_type
        self.multisig = multisig
        self.op_return_data = op_return_data
        self.orig_hash = orig_hash
        self.orig_index = orig_index


class TransactionType:
    def __init__(
        self,
        *,
        inputs: Optional[List["TxInputType"]] = None,
        bin_outputs: Optional[List["TxOutputBinType"]] = None,
        outputs: Optional[List["TxOutputType"]] = None,
        version: Optional["int"] = None,
        lock_time: Optional["int"] = None,
        inputs_cnt: Optional["int"] = None,
        outputs_cnt: Optional["int"] = None,
        extra_data: Optional["bytes"] = None,
        extra_data_len: Optional["int"] = None,
        expiry: Optional["int"] = None,
        overwintered: Optional["bool"] = None,
        version_group_id: Optional["int"] = None,
        timestamp: Optional["int"] = None,
        branch_id: Optional["int"] = None,
    ) -> None:
        self.inputs = inputs if inputs is not None else []
        self.bin_outputs = bin_outputs if bin_outputs is not None else []
        self.outputs = outputs if outputs is not None else []
        self.version = version
        self.lock_time = lock_time
        self.inputs_cnt = inputs_cnt
        self.outputs_cnt = outputs_cnt
        self.extra_data = extra_data
        self.extra_data_len = extra_data_len
        self.expiry = expiry
        self.overwintered = overwintered
        self.version_group_id = version_group_id
        self.timestamp = timestamp
        self.branch_id = branch_id


class TxOutputBinType:
    def __init__(
        self,
        *,
        amount: "int",
        script_pubkey: "bytes",
        decred_script_version: Optional["int"] = None,
    ) -> None:
        self.amount = amount
        self.script_pubkey = script_pubkey
        self.decred_script_version = decred_script_version


# Only handles up to 15 of 15
def parse_multisig(
    script: bytes,
    tx_xpubs: Dict[bytes, KeyOriginInfo],
    psbt_scope: Union[PartiallySignedInput, PartiallySignedOutput],
) -> Tuple[bool, messages.MultisigRedeemScriptType | None]:
    # at least OP_M pub OP_N OP_CHECKMULTISIG
    if len(script) < 37:
        return (False, None)
    # Get m
    m = script[0] - 80
    if m < 1 or m > 15:
        return (False, None)

    # Get pubkeys and build HDNodePathType
    pubkeys = []
    offset = 1
    while True:
        pubkey_len = script[offset]
        if pubkey_len != 33:
            break
        offset += 1
        key = script[offset : offset + 33]
        offset += 33

        hd_node = messages.HDNodeType(
            depth=0,
            fingerprint=0,
            child_num=0,
            chain_code=b"\x00" * 32,
            public_key=key,
        )
        pubkeys.append(messages.HDNodePathType(node=hd_node, address_n=[]))

    # Check things at the end
    n = script[offset] - 80
    if n != len(pubkeys):
        return (False, None)
    offset += 1
    op_cms = script[offset]
    if op_cms != 174:
        return (False, None)

    # check if we know corresponding xpubs from global scope
    for pub in pubkeys:
        if pub.node.public_key in psbt_scope.hd_keypaths:
            derivation = psbt_scope.hd_keypaths[pub.node.public_key]
            for xpub in tx_xpubs:
                hd = ExtendedPubKey.deserialize(base58.encode(xpub + sha256d_32(xpub)))
                origin = tx_xpubs[xpub]
                # check fingerprint and derivation
                if (origin.fingerprint == derivation.fingerprint) and (
                    origin.path == derivation.path[: len(origin.path)]
                ):
                    # all good - populate node and break
                    pub.address_n = list(derivation.path[len(origin.path) :])
                    pub.node = messages.HDNodeType(
                        depth=hd.depth,
                        fingerprint=int.from_bytes(hd.parent_fingerprint, "big"),
                        child_num=hd.child_num,
                        chain_code=hd.chaincode,
                        public_key=hd.pubkey,
                    )
                    break
    # Build MultisigRedeemScriptType and return it
    multisig = messages.MultisigRedeemScriptType(
        m=m, signatures=[b""] * n, pubkeys=pubkeys
    )
    return (True, multisig)


class SignPsbt:
    def __init__(self, req: CryptoPSBT):
        self.req = req
        self.qr = None
        self.tx = None
        self.inputs = []
        self.outputs = []
        self.signatures = []

    async def run(self):
        # if __debug__:
        #     utils.mem_trace(__name__, 0)
        psbt = PSBT()
        psbt.deserialize(self.req.get_psbt())
        # if __debug__:
        #     utils.mem_trace(__name__, 1)
        del self.req.psbt
        # if __debug__:
        #     utils.mem_trace(__name__, 2)
        self.tx = psbt
        from trezor.messages import GetPublicKey
        from apps.bitcoin import get_public_key as bitcoin_get_public_key
        from apps.common import passphrase

        if passphrase.is_enabled():
            wire.QR_CONTEXT.passphrase = None
        # pyright: off
        btc_pubkey_msg = GetPublicKey(address_n=[2147483692, 2147483708, 2147483648])
        resp = await bitcoin_get_public_key.get_public_key(
            wire.QR_CONTEXT, btc_pubkey_msg
        )
        master_fp = resp.root_fingerprint.to_bytes(4, "big")
        # if __debug__:
        #     master_fp = b'\x92\xbdh}'
        # pyright: on
        # Do multiple passes for multisig
        passes = 1
        p = 0
        is_TESTNET = False
        while p < passes:
            # Prepare inputs
            inputs = []
            to_ignore = (
                []
            )  # Note down which inputs whose signatures we're going to ignore
            for input_num, psbt_in in enumerate(psbt.inputs):
                assert psbt_in.prev_txid is not None
                assert psbt_in.prev_out is not None
                assert psbt_in.sequence is not None

                txinputtype = TxInputType(
                    prev_hash=bytes(reversed(psbt_in.prev_txid)),
                    prev_index=psbt_in.prev_out,
                    sequence=psbt_in.sequence,
                )

                # Determine spend type
                scriptcode = b""
                utxo = None
                if psbt_in.witness_utxo:
                    utxo = psbt_in.witness_utxo
                if psbt_in.non_witness_utxo:
                    if psbt_in.prev_txid != psbt_in.non_witness_utxo.hash:
                        raise Exception(
                            f"Input {input_num} has a non_witness_utxo with the wrong hash"
                        )
                    utxo = psbt_in.non_witness_utxo.vout[psbt_in.prev_out]
                if utxo is None:
                    continue
                scriptcode = utxo.scriptPubKey

                # Check if P2SH
                p2sh = False
                if is_p2sh(scriptcode):
                    # Look up redeems_cript
                    if len(psbt_in.redeem_script) == 0:
                        continue
                    scriptcode = psbt_in.redeem_script
                    p2sh = True

                # Check segwit
                is_wit, wit_ver, _ = is_witness(scriptcode)

                if is_wit:
                    if wit_ver == 0:
                        if p2sh:
                            txinputtype.script_type = InputScriptType.SPENDP2SHWITNESS
                        else:
                            txinputtype.script_type = InputScriptType.SPENDWITNESS
                    elif wit_ver == 1:
                        txinputtype.script_type = InputScriptType.SPENDTAPROOT
                else:
                    txinputtype.script_type = InputScriptType.SPENDADDRESS
                txinputtype.amount = utxo.nValue

                # Check if P2WSH
                p2wsh = False
                if is_p2wsh(scriptcode):
                    # Look up witness_script
                    if len(psbt_in.witness_script) == 0:
                        raise Exception("P2WSH script not found")
                    scriptcode = psbt_in.witness_script
                    p2wsh = True

                # Check for multisig
                is_ms, multisig = parse_multisig(scriptcode, psbt.xpub, psbt_in)
                if is_ms:
                    # Add to txinputtype
                    txinputtype.multisig = multisig
                    if not is_wit:
                        if utxo.is_p2sh():
                            txinputtype.script_type = InputScriptType.SPENDMULTISIG
                        else:
                            # Cannot sign bare multisig
                            raise Exception("Cannot sign bare multisig")
                elif not is_ms and not is_wit and not is_p2pkh(scriptcode):
                    # Cannot sign unknown spk
                    raise Exception("Cannot sign unknown scripts")
                elif not is_ms and is_wit and p2wsh:
                    # Cannot sign unknown witness script
                    raise Exception("Cannot sign unknown witness versions")

                # Find key to sign with
                found = False  # Whether we have found a key to sign with
                # found_in_sigs = (
                #     False  # Whether we have found one of our keys in the signatures
                # )
                our_keys = 0
                # path_last_ours = None  # The path of the last key that is ours. We will use this if we need to ignore this input because it is already signed.
                if txinputtype.script_type in ECDSA_SCRIPT_TYPES:
                    for key in psbt_in.hd_keypaths.keys():
                        keypath = psbt_in.hd_keypaths[key]
                        if keypath.fingerprint == master_fp:
                            # path_last_ours = keypath.path
                            if (
                                key in psbt_in.partial_sigs
                            ):  # This key already has a signature
                                # found_in_sigs = True
                                continue
                            if (
                                not found
                            ):  # This key does not have a signature and we don't have a key to sign with yet
                                if not is_TESTNET and (
                                    keypath.path[1] == (1 | 0x80000000)
                                ):
                                    is_TESTNET = True
                                txinputtype.address_n = keypath.path
                                found = True
                            our_keys += 1
                        else:
                            if __debug__:
                                print(
                                    f"Key fingerprint {keypath.fingerprint} does not match master key {master_fp}"
                                )
                            else:
                                raise MismatchError(
                                    "Key fingerprint does not match master key"
                                )
                elif txinputtype.script_type in SCHNORR_SCRIPT_TYPES:
                    # found_in_sigs = len(psbt_in.tap_key_sig) > 0
                    for key, (_, origin) in psbt_in.tap_bip32_paths.items():
                        # Assume key path signing
                        if (
                            key == psbt_in.tap_internal_key
                            and origin.fingerprint == master_fp
                        ):
                            # path_last_ours = origin.path
                            if not is_TESTNET and (origin.path[1] == (1 | 0x80000000)):
                                is_TESTNET = True
                            txinputtype.address_n = origin.path
                            found = True
                            our_keys += 1
                            break
                        else:
                            if __debug__:
                                print(
                                    f"Key fingerprint {origin.fingerprint} does not match master key {master_fp}"
                                )
                            else:
                                raise MismatchError(
                                    "Key fingerprint does not match master key"
                                )

                # Determine if we need to do more passes to sign everything
                if our_keys > passes:
                    passes = our_keys

                if not found:  # None of our keys were in hd_keypaths or in partial_sigs
                    # This input is not one of ours
                    raise Exception("Invalid input params")
                # elif not found and found_in_sigs:
                #     # All of our keys are in partial_sigs, pick the first key that is ours, sign with it,
                #     # and ignore whatever signature is produced for this input
                #     raise Exception("Invalid input params")

                # append to inputs
                inputs.append(txinputtype)
            self.inputs = inputs
            # address version byte
            if is_TESTNET:
                p2pkh_version = b"\x6f"
                p2sh_version = b"\xc4"
                bech32_hrp = "tb"
            else:
                p2pkh_version = b"\x00"
                p2sh_version = b"\x05"
                bech32_hrp = "bc"

            # prepare outputs
            outputs = []
            for psbt_out in psbt.outputs:
                out = psbt_out.get_txout()
                txoutput = TxOutputType(amount=out.nValue)
                txoutput.script_type = OutputScriptType.PAYTOADDRESS
                wit, ver, prog = out.is_witness()
                if wit:
                    txoutput.address = bech32.encode(bech32_hrp, ver, prog)
                elif out.is_p2pkh():
                    txoutput.address = base58.encode_check(
                        p2pkh_version + out.scriptPubKey[3:23], sha256d_32
                    )
                elif out.is_p2sh():
                    txoutput.address = base58.encode_check(
                        p2sh_version + out.scriptPubKey[2:22], sha256d_32
                    )
                elif out.is_opreturn():
                    txoutput.script_type = OutputScriptType.PAYTOOPRETURN
                    txoutput.op_return_data = out.scriptPubKey[2:]
                else:
                    raise Exception("Output is not an address")

                # Add the derivation path for change
                if not wit or (wit and ver == 0):
                    for _, keypath in psbt_out.hd_keypaths.items():
                        if keypath.fingerprint != master_fp:
                            if __debug__:
                                print(
                                    f"Key fingerprint {keypath.fingerprint} does not match master key {master_fp}"
                                )
                            else:
                                raise MismatchError(
                                    "Key fingerprint does not match master key"
                                )
                        wit, ver, prog = out.is_witness()
                        if out.is_p2pkh():
                            txoutput.address_n = keypath.path
                            txoutput.address = None
                        elif wit:
                            txoutput.script_type = OutputScriptType.PAYTOWITNESS
                            txoutput.address_n = keypath.path
                            txoutput.address = None
                        elif out.is_p2sh() and psbt_out.redeem_script:
                            wit, ver, prog = CTxOut(
                                0, psbt_out.redeem_script
                            ).is_witness()
                            if wit and len(prog) in [20, 32]:
                                txoutput.script_type = OutputScriptType.PAYTOP2SHWITNESS
                                txoutput.address_n = keypath.path
                                txoutput.address = None
                elif wit and ver == 1:
                    for key, (_, origin) in psbt_out.tap_bip32_paths.items():
                        # Assume key path change
                        if (
                            key == psbt_out.tap_internal_key
                            and origin.fingerprint == master_fp
                        ):
                            txoutput.address_n = origin.path
                            txoutput.script_type = OutputScriptType.PAYTOTAPROOT
                            txoutput.address = None
                            break

                # add multisig info
                if psbt_out.witness_script or psbt_out.redeem_script:
                    is_ms, multisig = parse_multisig(
                        psbt_out.witness_script or psbt_out.redeem_script,
                        psbt.xpub,
                        psbt_out,
                    )
                    if is_ms:
                        txoutput.multisig = multisig
                        if not wit:
                            txoutput.script_type = OutputScriptType.PAYTOMULTISIG

                # append to outputs
                outputs.append(txoutput)
            self.outputs = outputs

            self.signatures: List[bytes | None] = [None] * len(inputs)
            # Sign the transaction
            assert psbt.tx_version is not None
            # if __debug__:
            #     utils.mem_trace(__name__, 3)
            mods = utils.unimport_begin()
            from apps.bitcoin.sign_tx import sign_tx as bitcoin_sign_tx
            from trezor.messages import SignTx

            # if __debug__:
            #     utils.mem_trace(__name__, 4)
            loop.spawn(self.interact())
            # pyright: off
            res = await bitcoin_sign_tx(
                wire.QR_CONTEXT,
                SignTx(
                    coin_name="Bitcoin" if not is_TESTNET else "Testnet",
                    inputs_count=len(inputs),
                    outputs_count=len(outputs),
                    version=psbt.tx_version,
                    lock_time=psbt.compute_lock_time(),
                    serialize=False,
                ),
            )
            utils.unimport_end(mods)
            # pyright: on
            await wire.QR_CONTEXT.interact_stop()  # signal finshed
            assert messages.TxRequest.is_type_of(res)
            assert res.request_type == RequestType.TXFINISHED
            # if __debug__:
            #     utils.mem_trace(__name__, 5)
            self._retrieval_signatures(res)
            for input_num, (psbt_in, sig) in enumerate(
                list(zip(psbt.inputs, self.signatures))
            ):
                if input_num in to_ignore:
                    if __debug__:
                        print(f"input {input_num} signature ignored")
                    continue
                for pubkey in psbt_in.hd_keypaths.keys():
                    fp = psbt_in.hd_keypaths[pubkey].fingerprint
                    if fp == master_fp and pubkey not in psbt_in.partial_sigs:
                        assert sig is not None, "signature should not be None"
                        psbt_in.partial_sigs[pubkey] = sig + b"\x01"
                        if __debug__:
                            import binascii

                            print(
                                f"adding signature {binascii.hexlify(sig).decode()} for pubkey {pubkey} in input {input_num}"
                            )
                        break
                    if __debug__:
                        print(
                            f"input {input_num} signature missing for pubkey {pubkey} with fingerprint {fp}"
                        )
                if len(psbt_in.tap_internal_key) > 0 and len(psbt_in.tap_key_sig) == 0:
                    # Assume key path sig
                    assert sig is not None, "signature should not be None"
                    psbt_in.tap_key_sig = sig
                    if __debug__:
                        import binascii

                        print(
                            f"adding taproot signature {binascii.hexlify(sig).decode()} for input {input_num}"
                        )
            p += 1

        crypto_psbt = CryptoPSBT(psbt.serialize())
        del psbt
        del self.tx
        ur = crypto_psbt.to_ur()
        del crypto_psbt
        encoder = UREncoder(ur)
        del ur
        if encoder.is_single_part():
            self.qr = encoder.next_part()
            self.encoder = None
        else:
            self.encoder = encoder
            self.qr = None

    async def interact(self):

        assert self.tx is not None, "transaction should not be None"
        current_tx = TransactionType(
            inputs=self.inputs,
            outputs=self.outputs,
            inputs_cnt=len(self.inputs),
            outputs_cnt=len(self.outputs),
            version=self.tx.tx_version,
            lock_time=self.tx.compute_lock_time(),
        )

        while True:
            res = await wire.QR_CONTEXT.qr_receive()
            # if __debug__:
            #     utils.mem_trace(__name__, 6)
            if res is None:
                if __debug__:
                    print("btc sign psbt interaction finished")
                break
            if messages.TxRequest.is_type_of(res):
                self._retrieval_signatures(res)
                if __debug__:
                    print(f"tx request type: {res.request_type}")

                assert res.details is not None, "device did not provide details"

                if res.request_type in (RequestType.TXINPUT,):
                    assert res.details.request_index is not None
                    # pyright: off
                    msg = messages.TxAckInput(
                        tx=messages.TxAckInputWrapper(
                            input=messages.TxInput(
                                address_n=current_tx.inputs[
                                    res.details.request_index
                                ].address_n,
                                prev_hash=current_tx.inputs[
                                    res.details.request_index
                                ].prev_hash,
                                prev_index=current_tx.inputs[
                                    res.details.request_index
                                ].prev_index,
                                script_sig=current_tx.inputs[
                                    res.details.request_index
                                ].script_sig,
                                sequence=current_tx.inputs[
                                    res.details.request_index
                                ].sequence,
                                script_type=current_tx.inputs[
                                    res.details.request_index
                                ].script_type,
                                multisig=current_tx.inputs[
                                    res.details.request_index
                                ].multisig,
                                amount=current_tx.inputs[
                                    res.details.request_index
                                ].amount,
                                witness=current_tx.inputs[
                                    res.details.request_index
                                ].witness,
                                script_pubkey=current_tx.inputs[
                                    res.details.request_index
                                ].script_pubkey,
                            )
                        )
                    )
                    # pyright: on
                elif res.request_type == RequestType.TXOUTPUT:
                    assert res.details.request_index is not None
                    msg = messages.TxAckOutput(
                        tx=messages.TxAckOutputWrapper(
                            output=messages.TxOutput(
                                amount=current_tx.outputs[
                                    res.details.request_index
                                ].amount,
                                address_n=current_tx.outputs[
                                    res.details.request_index
                                ].address_n,
                                address=current_tx.outputs[
                                    res.details.request_index
                                ].address,
                                script_type=current_tx.outputs[
                                    res.details.request_index
                                ].script_type,
                                multisig=current_tx.outputs[
                                    res.details.request_index
                                ].multisig,
                                op_return_data=current_tx.outputs[
                                    res.details.request_index
                                ].op_return_data,
                            )
                        )
                    )
                # elif res.request_type == RequestType.TXEXTRADATA:
                #     assert res.details.extra_data_offset is not None
                #     assert res.details.extra_data_len is not None
                #     assert current_tx.extra_data is not None
                #     o, l = res.details.extra_data_offset, res.details.extra_data_len
                #     msg.extra_data = current_tx.extra_data[o : o + l]
                else:
                    raise Exception(f"Unknown request type - {res.request_type}.")
                await wire.QR_CONTEXT.qr_send(msg)

    def _retrieval_signatures(self, res: messages.TxRequest) -> None:
        if res.serialized:
            if __debug__:
                print(f"got signature at index: {res.serialized.signature_index}")
            if res.serialized.signature_index is not None:
                assert (
                    res.serialized.signature is not None
                ), "signature should not be None"
                idx = res.serialized.signature_index
                sig = res.serialized.signature
                if self.signatures[idx] is not None:
                    raise ValueError(f"Signature for index {idx} already filled")
                self.signatures[idx] = sig
                if __debug__:
                    import binascii

                    print(
                        f"signature for index {idx} is: {binascii.hexlify(sig).decode()}"
                    )

    @staticmethod
    async def gen_request(ur) -> "SignPsbt":
        req = CryptoPSBT.from_cbor(ur.cbor)
        return SignPsbt(req)
