from typing import TYPE_CHECKING

import storage.device
from trezor import wire
from trezor.messages import Success

if TYPE_CHECKING:
    from trezor.messages import BackupDevice


async def backup_device(ctx: wire.Context, msg: BackupDevice) -> Success:
    if not storage.device.is_initialized():
        raise wire.NotInitialized("Device is not initialized")
    raise wire.ProcessError("Seed already backed up")
