from typing import TYPE_CHECKING

from trezor.strings import format_amount
from trezor.utils import BufferReader

from apps.common import readers

from .helpers import neo_address_from_script_hash
from .tokens import NeoTokenInfo

if TYPE_CHECKING:
    from typing import Sequence
    from enum import IntEnum
else:
    IntEnum = object

TRANSFER_SCRIPT_SEQUENCE = (
    b"\x14\xC0\x1F\x0C\x08\x74\x72\x61\x6e\x73\x66\x65\x72\x0C\x14"
)
CONTRACT_SYSCALL_SEQUENCE = b"\x41\x62\x7d\x5b\x52"
VOTE_SCRIPT_SEQUENCE = b"\x12\xC0\x1F\x0C\x04\x76\x6F\x74\x65\x0C\x14"

_HASH160_SIZE = 20
_COMPRESSED_PUBLIC_KEY_SIZE = 33


class WitnessScope(IntEnum):
    NONE = 0x0
    CALLED_BY_ENTRY = 0x01
    CUSTOM_CONTRACTS = 0x10
    CUSTOM_GROUPS = 0x20
    WITNESS_RULES = 0x40
    GLOBAL = 0x80


class WitnessRuleAction(IntEnum):
    DENY = 0
    ALLOW = 1


class WitnessConditionType(IntEnum):

    BOOLEAN = 0x0
    NOT = 0x01
    AND = 0x2
    OR = 0x03
    SCRIPT_HASH = 0x18
    GROUP = 0x19
    CALLED_BY_ENTRY = 0x20
    CALLED_BY_CONTRACT = 0x28
    CALLED_BY_GROUP = 0x29


class WitnessCondition:

    MAX_SUB_ITEMS = 16
    MAX_NESTING_DEPTH = 2

    _type = WitnessConditionType.BOOLEAN

    def type(self) -> WitnessConditionType | int:
        return self._type

    def _deserialize_without_type(
        self, reader: BufferReader, max_nesting_depth: int
    ) -> None:
        pass

    @staticmethod
    def _deserialize_conditions(
        reader: BufferReader, max_nesting_depth: int
    ) -> list["WitnessCondition"]:
        return [
            WitnessCondition._deserialize_from(reader, max_nesting_depth)
            for _ in range(readers.read_compact_size(reader))
        ]

    @staticmethod
    def _deserialize_from(
        reader: BufferReader, max_nesting_depth: int
    ) -> "WitnessCondition":
        condition_type = reader.get()

        def find_condition(
            condition_cls: type[WitnessCondition],
        ) -> "WitnessCondition":
            for sub in condition_cls.__subclasses__():
                child = sub._serializable_init()
                if child.type() == condition_type:
                    child._deserialize_without_type(reader, max_nesting_depth)
                    return child
                if len(sub.__subclasses__()) > 0:
                    condition = find_condition(sub)
                    if condition is not None:
                        return condition
            raise ValueError(
                f"Deserialization error - unknown witness condition. Type: {condition_type}"
            )

        condition = find_condition(WitnessCondition)
        return condition

    @classmethod
    def _serializable_init(cls) -> "WitnessCondition":
        return cls()


class ConditionAnd(WitnessCondition):

    _type = WitnessConditionType.AND

    def __init__(self, expressions: list[WitnessCondition] | None = None):
        self.expressions = expressions if expressions else []

    def __eq__(self, other):
        if type(other) != type(self):
            return False
        return self.expressions == other.expressions

    def _deserialize_without_type(
        self, reader: BufferReader, max_nesting_depth: int
    ) -> None:
        if max_nesting_depth <= 0:
            raise ValueError("Max nesting depth cannot be negative")
        self.expressions = WitnessCondition._deserialize_conditions(
            reader, max_nesting_depth
        )
        if len(self.expressions) == 0:
            raise ValueError("Cannot have 0 expressions")


class ConditionBool(WitnessCondition):

    _type = WitnessConditionType.BOOLEAN

    def __init__(self, value: bool | None = None):
        self.value = value

    def __eq__(self, other):
        if type(other) != type(self):
            return False
        return self.value == other.value

    def _deserialize_without_type(
        self, reader: BufferReader, max_nesting_depth: int
    ) -> None:
        self.value = bool(reader.get())


class ConditionNot(WitnessCondition):

    _type = WitnessConditionType.NOT

    def __init__(self, expression: WitnessCondition | None = None):
        self.expression = expression if expression else None

    def __eq__(self, other):
        if type(other) != type(self):
            return False
        return self.expression == other.expression

    def _deserialize_without_type(
        self, reader: BufferReader, max_nesting_depth: int
    ) -> None:
        if max_nesting_depth <= 0:
            raise ValueError("Max nesting depth cannot be negative")
        self.expression = WitnessCondition._deserialize_from(
            reader, max_nesting_depth - 1
        )


class ConditionOr(WitnessCondition):

    _type = WitnessConditionType.OR

    def __init__(self, expressions: list[WitnessCondition] | None = None):
        self.expressions = expressions if expressions else []

    def __eq__(self, other):
        if type(other) != type(self):
            return False
        return self.expressions == other.expressions

    def _deserialize_without_type(
        self, reader: BufferReader, max_nesting_depth: int
    ) -> None:
        if max_nesting_depth <= 0:
            raise ValueError("Max nesting depth cannot be negative")
        self.expressions = WitnessCondition._deserialize_conditions(
            reader, max_nesting_depth
        )
        if len(self.expressions) == 0:
            raise ValueError("Cannot have 0 expressions")


class ConditionCalledByContract(WitnessCondition):

    _type = WitnessConditionType.CALLED_BY_CONTRACT

    def __init__(self, _hash: bytes | None = None):
        self._hash = _hash if _hash else b""

    def __eq__(self, other):
        if type(other) != type(self):
            return False
        return self._hash == other._hash

    def _deserialize_without_type(
        self, reader: BufferReader, max_nesting_depth: int
    ) -> None:
        self._hash = reader.read(_HASH160_SIZE)


class ConditionCalledByEntry(WitnessCondition):

    _type = WitnessConditionType.CALLED_BY_ENTRY

    def __eq__(self, other):
        if type(self) == type(other):
            return True
        return False


class ConditionCalledByGroup(WitnessCondition):

    _type = WitnessConditionType.CALLED_BY_GROUP

    def __init__(self, group: bytes):
        self.group = group

    def __eq__(self, other):
        if type(self) != type(other):
            return False

        return self.group == other.group

    def _deserialize_without_type(
        self, reader: BufferReader, max_nesting_depth: int
    ) -> None:
        self.group = reader.read(_COMPRESSED_PUBLIC_KEY_SIZE)


class ConditionGroup(ConditionCalledByGroup):

    _type = WitnessConditionType.GROUP


class ConditionScriptHash(ConditionCalledByContract):

    _type = WitnessConditionType.SCRIPT_HASH


class WitnessRule:
    def __init__(
        self,
        action: WitnessRuleAction | None = None,
        condition: WitnessCondition | None = None,
    ):
        self.action = action
        self.condition = condition

    def deserialize(self, reader: BufferReader) -> "WitnessRule":
        self.action = reader.get()
        self.condition = WitnessCondition._deserialize_from(
            reader, WitnessCondition.MAX_NESTING_DEPTH
        )
        return self


class Signer:

    # Max number of allowed_contracts or allowed_groups
    MAX_SUB_ITEMS = 16

    def __init__(
        self,
        account: bytes | None = None,
        scope: WitnessScope = WitnessScope.CALLED_BY_ENTRY,
        allowed_contracts: Sequence[bytes] | None = None,
        allowed_groups: Sequence[bytes] | None = None,
        rules: Sequence[bytes] | None = None,
    ):
        # The TX sender.
        self.account = account
        # WitnessScope: The configured validation scope.
        self.scope = scope
        # list[bytes20]: Whitelist of contract script hashes if used with `WitnessScope.CUSTOM_CONTRACTS`.
        self.allowed_contracts = allowed_contracts if allowed_contracts else []
        # list[bytes33]: Whitelist of public keys if used with `WitnessScope.CUSTOM_GROUPS`.
        self.allowed_groups = allowed_groups if allowed_groups else []
        # list[bytes]: List of rules that must pass for the current execution context when used with `WitnessScope.WITNESS_RULES`.
        self.rules = rules if rules else []

    def deserialize(self, reader: BufferReader) -> "Signer":
        self.account = reader.read(_HASH160_SIZE)
        self.scope = reader.get()

        if self.scope & WitnessScope.GLOBAL and self.scope != WitnessScope.GLOBAL:
            raise ValueError(
                "Deserialization error - invalid scope. GLOBAL scope not allowed with other scope types"
            )

        if self.scope & WitnessScope.CUSTOM_CONTRACTS:
            self.allowed_contracts = [
                reader.read(_HASH160_SIZE)
                for _ in range(readers.read_compact_size(reader))
            ]

        if self.scope & WitnessScope.CUSTOM_GROUPS:
            self.allowed_groups = [
                reader.read(_COMPRESSED_PUBLIC_KEY_SIZE)
                for _ in range(readers.read_compact_size(reader))
            ]

        if self.scope & WitnessScope.WITNESS_RULES:
            self.rules = [
                WitnessRule().deserialize(reader)
                for _ in range(readers.read_compact_size(reader))
            ]
        return self


class TransactionAttributeType(IntEnum):

    _INVALID = 0x9999
    HIGH_PRIORITY = 0x1
    ORACLE_RESPONSE = 0x11


class TransactionAttribute:
    def __init__(self):
        self._type: TransactionAttributeType = TransactionAttributeType._INVALID
        self.allow_multiple = False

    @staticmethod
    def deserialize_from(reader: BufferReader) -> "TransactionAttribute":
        attribute_type = reader.get()
        for sub in TransactionAttribute.__subclasses__():
            child = sub._serializable_init()
            if child._type == attribute_type:
                child._deserialize_without_type(reader)
                return child
        raise ValueError("Deserialization error - unknown transaction attribute type")

    def _deserialize_without_type(self, reader: BufferReader) -> None:
        pass

    @classmethod
    def _serializable_init(cls) -> "TransactionAttribute":
        return cls()


class HighPriorityAttribute(TransactionAttribute):
    def __init__(self):
        super(HighPriorityAttribute, self).__init__()
        self._type = TransactionAttributeType.HIGH_PRIORITY


class OracleResponseCode(IntEnum):
    SUCCESS = 0x00
    PROTOCOL_NOT_SUPPORTED = 0x10
    CONSENSUS_UNREACHABLE = 0x12
    NOT_FOUND = 0x14
    TIMEOUT = 0x16
    FORBIDDEN = 0x18
    RESPONSE_TOO_LARGE = 0x1A
    INSUFFICIENT_FUNDS = 0x1C
    CONTENT_TYPE_NOT_SUPPORTED = 0x1F
    ERROR = 0xFF


class OracleResponse(TransactionAttribute):

    _MAX_RESULT_SIZE = 0xFFFF

    def __init__(self, id: int, code: OracleResponseCode, result: bytes):
        super(OracleResponse, self).__init__()
        self._type = TransactionAttributeType.ORACLE_RESPONSE
        self.allow_multiple = False
        self.id = id
        self.code = code
        self.result = result

    def _deserialize_without_type(self, reader: BufferReader) -> None:
        self.id = readers.read_uint64_le(reader)
        self.code = reader.get()
        self.result = reader.read(readers.read_compact_size(reader))
        if self.code != OracleResponseCode.SUCCESS and len(self.result) > 0:
            raise ValueError(f"Deserialization error - oracle response: {self.code}")

    @classmethod
    def _serializable_init(cls) -> "OracleResponse":
        return cls(0, OracleResponseCode.ERROR, b"")


class RawTransaction:
    #: the max number of bytes allowed in a transaction
    MAX_TX_SIZE = 10240
    #: the max number of transaction attributes allowed in a transaction
    MAX_TX_ATTRIBUTES = 16

    HEADER_SIZE = (
        1
        + 4
        + 8
        + 8
        + 4  # Version  # Nonce  # System Fee  # Network Fee  # Valid Until Block
    )

    def __init__(self):
        self._version: int | None = None
        self._nonce: int | None = None
        self._system_fee: int = 0
        self._network_fee: int = 0
        self._valid_until_block: int | None = None
        self._signers: list[Signer] = []
        self._attributes: list[TransactionAttribute] = []
        self._script: bytes | None = None
        self._destination_script_hash: bytes | None = None
        self._source_script_hash: bytes | None = None
        self._contract_script_hash: bytes | None = None
        self._amount: int = 0
        self._is_remove_vote: bool = False
        self._vote_to: bytes | None = None
        self._is_vote: bool = False
        self._is_asset_transfer: bool = False
        self._token: NeoTokenInfo | None = None

    def deserialize(self, reader: BufferReader) -> None:
        assert reader.remaining_count() > self.HEADER_SIZE, "transaction is too short"
        self._version = reader.get()
        assert self._version == 0, "version must be 0"
        self._nonce = readers.read_uint32_le(reader)
        self._system_fee = readers.read_int64_le(reader)
        assert self._system_fee >= 0, "system_fee must be positive"
        self._network_fee = readers.read_int64_le(reader)
        assert self._network_fee >= 0, "network_fee must be positive"
        self._valid_until_block = readers.read_uint32_le(reader)

        signers_count = readers.read_compact_size(reader)
        assert signers_count > 0, "signers must be non-empty"
        self._signers = [Signer().deserialize(reader) for _ in range(signers_count)]
        assert signers_count == len(set(self._signers)), "signers must be unique"

        attributes_count = readers.read_compact_size(reader)
        assert (
            attributes_count <= self.MAX_TX_ATTRIBUTES
        ), f"attributes must be less than or equal to {self.MAX_TX_ATTRIBUTES}"
        self._attributes = [
            TransactionAttribute.deserialize_from(reader)
            for _ in range(attributes_count)
        ]
        assert attributes_count == len(
            set(self._attributes)
        ), "attributes must be unique"

        script_length = readers.read_compact_size(reader)
        assert script_length > 0, "script must be non-empty"
        self._script = reader.read(script_length)
        assert reader.remaining_count() == 0, "reader must be empty"

        if not self.parse_asset_transfer():
            self.parse_vote()

    def sender(self) -> str:
        account = self._signers[0].account
        assert account is not None, "signers must be non-empty"
        return neo_address_from_script_hash(account)

    def source(self) -> str:
        assert self._source_script_hash is not None, "Invalid transaction state"
        return neo_address_from_script_hash(self._source_script_hash)

    def destination(self) -> str:
        assert self._destination_script_hash is not None, "Invalid transaction state"
        return neo_address_from_script_hash(self._destination_script_hash)

    def vote_to(self) -> str:
        assert self._vote_to is not None, "Invalid transaction state"
        from .helpers import neo_address_from_pubkey

        return neo_address_from_pubkey(self._vote_to)

    def token(self) -> NeoTokenInfo:
        if self._token is None:
            from .tokens import token_by_contract_script_hash

            assert self._contract_script_hash is not None, "Invalid transaction state"
            self._token = token_by_contract_script_hash(self._contract_script_hash)
        return self._token

    def token_contract_hash(self) -> str:
        assert self._contract_script_hash is not None, "Invalid transaction state"
        from binascii import hexlify

        return hexlify(bytes(reversed(self._contract_script_hash))).decode()

    def is_unknown_token(self) -> bool:
        token = self.token()
        from .tokens import UNKNOWN_TOKEN

        return token == UNKNOWN_TOKEN

    def display_amount(self) -> str:
        assert self._is_asset_transfer, "Invalid transaction state"

        token = self.token()
        return f"{format_amount(self._amount, token.decimals)} {token.symbol}"

    def is_asset_transfer(self) -> bool:
        return self._is_asset_transfer

    def is_vote(self) -> bool:
        return self._is_vote

    def is_remove_vote(self) -> bool:
        return self._is_remove_vote

    def total_fee(self) -> str:
        return f"{format_amount(self._system_fee + self._network_fee, 8)} GAS"

    def parse_asset_transfer(self) -> bool:
        assert self._script, "Invalid transaction state"
        reader = BufferReader(self._script)
        op_code = reader.get()
        # first byte should be 0xb (OpCode.PUSHNULL), indicating no data for the Nep17.transfer() 'data' argument
        if op_code != 0xB:
            return False
        # OpCode.PUSH0 - OpCode.PUSH16
        op_code = reader.get()
        if 0x10 <= op_code <= 0x20:
            self._amount = op_code - 0x10
        elif op_code == 0x00:
            self._amount = readers.read_int8_le(reader)
        elif op_code == 0x01:
            self._amount = readers.read_int16_le(reader)
        elif op_code == 0x02:
            self._amount = readers.read_int32_le(reader)
        elif op_code == 0x03:
            self._amount = readers.read_int64_le(reader)
        else:
            # do not support INT128 and INT256 values
            return False
        # parse destination script hash
        op_code = reader.get()
        if op_code != 0x0C:
            return False
        script_length = reader.get()
        if script_length != 0x14:
            return False
        self._destination_script_hash = reader.read(_HASH160_SIZE)
        # check for source script hash
        op_code = reader.get()
        if op_code != 0x0C:
            return False
        script_length = reader.get()
        if script_length != 0x14:
            return False
        self._source_script_hash = reader.read(_HASH160_SIZE)
        # check for transfer script sequence
        script = reader.read(len(TRANSFER_SCRIPT_SEQUENCE))
        if script != TRANSFER_SCRIPT_SEQUENCE:
            return False
        # read contract script hash
        self._contract_script_hash = reader.read(_HASH160_SIZE)
        # make sure we end with a contract syscall
        ending_script = reader.read(len(CONTRACT_SYSCALL_SEQUENCE))
        if ending_script != CONTRACT_SYSCALL_SEQUENCE:
            return False
        assert (
            reader.remaining_count() == 0
        ), "extra code after the transfer script is not allowed"
        self._is_asset_transfer = True
        return True

    def parse_vote(self) -> bool:
        assert self._script, "Invalid transaction state"
        reader = BufferReader(self._script)
        op_code = reader.get()
        # first byte should be 0xb when removing a vote, or 0x0C when voting
        if op_code not in [0xB, 0x0C]:
            return False
        if op_code == 0xB:
            self._is_remove_vote = True
        else:
            op_code = reader.get()
            if op_code != 0x21:
                return False
            self._vote_to = reader.read(_COMPRESSED_PUBLIC_KEY_SIZE)
            assert self._vote_to[0] in [0x02, 0x03], "invalid compressed public key"
            self._is_remove_vote = False
        # check for source script hash
        op_code = reader.get()
        if op_code != 0x0C:
            return False
        script_length = reader.get()
        if script_length != 0x14:
            return False
        self._source_script_hash = reader.read(_HASH160_SIZE)
        # check for vote script sequence
        script = reader.read(len(VOTE_SCRIPT_SEQUENCE))
        if script != VOTE_SCRIPT_SEQUENCE:
            return False
        # read contract script hash
        contract_script_hash = reader.read(_HASH160_SIZE)
        from .tokens import NEO_SCRIPT_HASH

        if contract_script_hash != NEO_SCRIPT_HASH:
            return False
        # make sure we end with a contract syscall
        ending_script = reader.read(len(CONTRACT_SYSCALL_SEQUENCE))
        if ending_script != CONTRACT_SYSCALL_SEQUENCE:
            return False
        assert (
            reader.remaining_count() == 0
        ), "extra code after the vote script is not allowed"
        self._is_vote = True
        return True
