MAX_MESSAGE_LENGTH = 1024 * 8


def validate_message(message: bytes) -> None:
    if len(message) > MAX_MESSAGE_LENGTH:
        raise ValueError("Message too long, max 8KB")


def validate_message_with_custom_limit(message: bytes, max_length: int) -> None:
    if max_length <= 0:
        raise ValueError("Max length must be positive")
    if len(message) > max_length:
        raise ValueError(f"Message too long, max {max_length // 1024}KB")
