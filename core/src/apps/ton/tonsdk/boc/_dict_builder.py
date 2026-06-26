from ._cell import Cell
from .dict import serialize_dict


class DictBuilder:
    def __init__(self, key_size: int):
        self.key_size = key_size
        self.items = {}
        self.ended = False

    def _ensure_not_ended(self):
        if self.ended:
            raise RuntimeError("Already ended")

    def store_cell(self, index, value: Cell):
        self._ensure_not_ended()
        if type(index) is bytes:
            index = int(index.hex(), 16)

        if type(index) is not int:
            raise TypeError("Invalid index type")
        if index in self.items:
            raise ValueError(f"Item {index} already exists")
        self.items[index] = value
        return self

    def store_ref(self, index, value: Cell):
        self._ensure_not_ended()

        cell = Cell()
        cell.refs.append(value)
        self.store_cell(index, cell)
        return self

    def end_dict(self) -> Cell:
        self._ensure_not_ended()
        self.ended = True
        if not self.items:
            return Cell()  # ?

        def default_serializer(src, dest):
            dest.write_cell(src)

        return serialize_dict(self.items, self.key_size, default_serializer)

    def end_cell(self) -> Cell:
        self._ensure_not_ended()
        if not self.items:
            raise ValueError("Dict is empty")
        return self.end_dict()


def begin_dict(key_size):
    return DictBuilder(key_size)
