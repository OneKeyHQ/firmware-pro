class Error(Exception):
    def __init__(self, message: str) -> None:
        super().__init__()
        self.message = message


class MismatchError(Error):
    def __init__(self, message: str) -> None:
        super().__init__(message)
