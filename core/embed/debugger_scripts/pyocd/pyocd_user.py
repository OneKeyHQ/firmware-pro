import pyocd
from pyocd.core.memory_map import (MemoryMap, MemoryType, FlashRegion)
import pyocd.core.target as SoCTarget
import pyocd.board as Board

target:SoCTarget

# For OneKey Pro/Touch
def will_connect(board:Board):

    # remove all flash region
    # for region in target.memory_map.clone():
    #     if region.type == MemoryType.FLASH:
    #             target.memory_map.remove_region(target.memory_map[region.name])

    # QSPI Flash 8M
    qspiflash = FlashRegion(
                            name="QSPI_FLASH",
                            start=0x90000000,
                            length=0x800000, # 8M
                            blocksize=0x10000, # 64k
                            is_boot_memory=True, # to force the test
                            # flm="OnekeyH7_QSPI.FLM"
                            flm="ONEKEY_STM32H7x_QSPI_W25Q64.FLM"
                            )
    target.memory_map.add_region(qspiflash)

