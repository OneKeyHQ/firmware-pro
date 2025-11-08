import gc
from micropython import const
from typing import TYPE_CHECKING, Optional

from storage import device
from trezor import io, wire
from trezor.crypto.hashlib import blake2s
from trezor.enums import ResourceType
from trezor.messages import (
    BlurRequest,
    ResourceAck,
    ResourceRequest,
    Success,
    ZoomRequest,
)

import ujson as json
import ure as re  # type: ignore[could not be resolved]

if TYPE_CHECKING:
    from trezor.messages import ResourceUpload
SUPPORTED_EXTS = (("jpg", "png", "jpeg"), ("jpg", "jpeg", "png", "mp4"))

SUPPORTED_MAX_RESOURCE_SIZE = {
    "jpg": const(1024 * 1024),
    "jpeg": const(1024 * 1024),
    "png": const(1024 * 1024),
    "mp4": const(10 * 1024 * 1024),
}
NFT_METADATA_ALLOWED_KEYS = ("header", "subheader", "network", "owner")
REQUEST_CHUNK_SIZE = const(8 * 1024)
MIN_CHUNK_SIZE = const(2 * 1024)

MAX_WP_COUNTER = const(5)
MAX_NFT_COUNTER = const(24)

PATTERN = re.compile(r"^(nft|wp)-[0-9a-f]+-\d+$")


def _ensure_file_removed(path: str) -> None:
    try:
        io.fatfs.stat(path)
    except BaseException:
        return
    io.fatfs.unlink(path)


def _cleanup_partial_files(*paths: Optional[str]) -> None:
    for path in paths:
        if path:
            _ensure_file_removed(path)


def _verify_file_size(path: str, expected_size: int) -> None:
    if expected_size <= 0:
        return
    try:
        actual_size, _, _ = io.fatfs.stat(path)
    except BaseException as err:
        _ensure_file_removed(path)
        raise wire.FirmwareError(f"Failed to verify file {path}: {err}")
    if actual_size != expected_size:
        _ensure_file_removed(path)
        raise wire.FirmwareError(
            f"File size mismatch for {path}. Expected {expected_size}, got {actual_size}"
        )


async def upload_res(ctx: wire.Context, msg: ResourceUpload) -> Success:
    res_type = msg.res_type
    res_ext = msg.extension
    res_size = msg.data_length
    res_zoom_size = msg.zoom_data_length
    res_blur_size = msg.blur_data_length or 0

    from trezorui import Display

    display = Display()
    if hasattr(display, "cover_background_hide"):
        display.cover_background_hide()
    if hasattr(display, "cover_background_set_visible"):
        display.cover_background_set_visible(False)

    import trezorio
    from trezor import loop

    if hasattr(trezorio, "jpeg_decoder_is_busy"):
        max_wait_iterations = 100
        for _ in range(max_wait_iterations):
            is_busy = trezorio.jpeg_decoder_is_busy()  # type: ignore[is not a known member of module]
            has_error = (
                trezorio.jpeg_decoder_has_error()  # type: ignore[is not a known member of module]
                if hasattr(trezorio, "jpeg_decoder_has_error")
                else False
            )

            if has_error:
                break

            if not is_busy:
                break

            await loop.sleep(10)
    else:
        await loop.sleep(500)

    if hasattr(trezorio, "jpeg_save_decoder_state"):
        trezorio.jpeg_save_decoder_state()  # type: ignore[is not a known member of module]

    from trezor.lvglui.scrs.common import lv

    lv.img.cache_invalidate_src(None)

    from trezor.lvglui.scrs import homescreen

    if hasattr(homescreen, "_cached_styles"):
        homescreen._cached_styles.clear()
    if hasattr(homescreen, "_last_jpeg_loaded"):
        homescreen._last_jpeg_loaded = None

    for _ in range(5):
        gc.collect()

    mem_after = gc.mem_free()  # type: ignore[is not a known member of module]

    MIN_REQUIRED_MEMORY = 50 * 1024
    if mem_after < MIN_REQUIRED_MEMORY:
        for _ in range(3):
            gc.collect()

    if hasattr(trezorio, "jpeg_decoder_is_busy"):
        for _ in range(20):
            if not trezorio.jpeg_decoder_is_busy():  # type: ignore[is not a known member of module]
                break
            await loop.sleep(10)

    await loop.sleep(50)

    if res_ext not in SUPPORTED_EXTS[res_type]:
        raise wire.DataError("Not supported resource extension")
    elif res_size >= SUPPORTED_MAX_RESOURCE_SIZE[res_ext]:
        raise wire.DataError("Data size overflow")
    if msg.file_name_no_ext:
        if PATTERN.match(msg.file_name_no_ext) is None:
            raise wire.DataError(
                "File name should follow the pattern (^(nft|wp)-[0-9a-f]{8}-\\d{13,}$)"
            )
    else:
        raise wire.DataError("File name required")
    metadata_len = 0
    if res_type == ResourceType.Nft:
        if msg.nft_meta_data is None:
            raise wire.DataError("NFT metadata required")
        elif len(msg.nft_meta_data) >= 2048:
            raise wire.DataError("NFT metadata must be less than 2K")
        try:
            metadata = json.loads(msg.nft_meta_data.decode("utf-8"))
        except BaseException as e:
            raise wire.DataError(f"Invalid metadata {e}")
        metadata_len = len(msg.nft_meta_data)
        if any(key not in metadata.keys() for key in NFT_METADATA_ALLOWED_KEYS):
            raise wire.DataError("Invalid metadata")
    replace = False
    name_list = []
    try:
        file_counter = 0
        if res_type == ResourceType.WallPaper:
            for size, _attrs, name in io.fatfs.listdir("1:/res/wallpapers"):
                if size > 0 and name[:4] == "zoom":
                    file_counter += 1
                    name_list.append(name)
                if file_counter >= MAX_WP_COUNTER:
                    replace = True
                    break
        else:
            for size, _attrs, name in io.fatfs.listdir("1:/res/nfts/zooms"):
                if size > 0:
                    file_counter += 1
                    name_list.append(name)
                if file_counter >= MAX_NFT_COUNTER:
                    replace = True
                    break
    except BaseException as e:
        raise wire.FirmwareError(f"File system error {e}")
    file_name = msg.file_name_no_ext
    for name in name_list:
        if file_name[: file_name.rindex("-")] == name[5 : name.rindex("-")]:
            if res_type == ResourceType.WallPaper:
                return Success(message="Success")
            else:
                raise wire.DataError("File already exists")

    if res_type == ResourceType.WallPaper:
        for size, _attrs, name in io.fatfs.listdir("1:/res/wallpapers"):
            if size <= 0 or not name.startswith("zoom-"):
                continue
            dot_idx = name.rfind(".")
            if dot_idx <= 0:
                continue
            ext = name[dot_idx + 1 :]
            if ext != res_ext:
                continue

            orig_name = name[len("zoom-") :]
            zoom_path = f"1:/res/wallpapers/{name}"
            orig_path = f"1:/res/wallpapers/{orig_name}"

            try:
                zoom_size, _, _ = io.fatfs.stat(zoom_path)
                orig_size, _, _ = io.fatfs.stat(orig_path)
            except BaseException:
                continue

            if zoom_size == res_zoom_size and orig_size == res_size:
                return Success(message="Success")
    config_path = ""
    blur_path = ""
    if res_type == ResourceType.WallPaper:
        file_full_path = f"1:/res/wallpapers/{file_name}.{res_ext}"
        zoom_path = f"1:/res/wallpapers/zoom-{file_name}.{res_ext}"
        if res_blur_size > 0:
            blur_path = f"1:/res/wallpapers/{file_name}-blur.{res_ext}"
    else:
        file_full_path = f"1:/res/nfts/imgs/{file_name}.{res_ext}"
        zoom_path = f"1:/res/nfts/zooms/zoom-{file_name}.{res_ext}"
        config_path = f"1:/res/nfts/desc/{file_name}.json"
        if res_blur_size > 0:
            blur_path = f"1:/res/nfts/imgs/{file_name}-blur.{res_ext}"

    try:
        with io.fatfs.open(file_full_path, "w") as f:
            data_left = res_size
            offset = 0
            current_limit = REQUEST_CHUNK_SIZE
            while data_left > 0:
                requested = min(current_limit, data_left)
                while True:
                    try:
                        request = ResourceRequest(data_length=requested, offset=offset)
                        ack: ResourceAck = await ctx.call(request, ResourceAck)
                        break
                    except Exception as e:
                        if _is_codec_too_large(e) and requested > MIN_CHUNK_SIZE:
                            new_limit = max(requested // 2, MIN_CHUNK_SIZE)
                            _heavy_gc(5)
                            current_limit = new_limit
                            requested = min(new_limit, data_left)
                            continue
                        _heavy_gc(3)
                        try:
                            request = ResourceRequest(
                                data_length=requested, offset=offset
                            )
                            ack = await ctx.call(request, ResourceAck)
                            break
                        except Exception:
                            raise

                data = ack.data_chunk
                actual_len = len(data) if data else 0
                if actual_len == 0:
                    raise wire.DataError("Received empty chunk")
                if actual_len != requested:
                    current_limit = min(current_limit, actual_len)
                digest = blake2s(data).digest()
                if digest != ack.hash:
                    raise wire.DataError("Date digest is inconsistent")
                f.write(data)
                offset += actual_len
                data_left -= actual_len
            f.sync()
        _verify_file_size(file_full_path, res_size)

        with io.fatfs.open(zoom_path, "w") as f:
            data_left = res_zoom_size
            offset = 0
            current_limit = REQUEST_CHUNK_SIZE
            while data_left > 0:
                requested = min(current_limit, data_left)
                while True:
                    try:
                        request = ZoomRequest(data_length=requested, offset=offset)
                        ack: ResourceAck = await ctx.call(request, ResourceAck)
                        break
                    except Exception as e:
                        if _is_codec_too_large(e) and requested > MIN_CHUNK_SIZE:
                            new_limit = max(requested // 2, MIN_CHUNK_SIZE)
                            _heavy_gc(5)
                            current_limit = new_limit
                            requested = min(new_limit, data_left)
                            continue
                        _heavy_gc(3)
                        try:
                            request = ZoomRequest(data_length=requested, offset=offset)
                            ack = await ctx.call(request, ResourceAck)
                            break
                        except Exception:
                            raise

                data = ack.data_chunk
                actual_len = len(data) if data else 0
                if actual_len == 0:
                    raise wire.DataError("Received empty zoom chunk")
                if actual_len != requested:
                    current_limit = min(current_limit, actual_len)
                digest = blake2s(data).digest()
                if digest != ack.hash:
                    raise wire.DataError("Date digest is inconsistent")
                f.write(data)
                offset += actual_len
                data_left -= actual_len
            f.sync()
        _verify_file_size(zoom_path, res_zoom_size)

        if (
            res_type in (ResourceType.WallPaper, ResourceType.Nft)
            and blur_path
            and res_blur_size > 0
        ):
            for _ in range(5):
                gc.collect()

            await loop.sleep(50)

            with io.fatfs.open(blur_path, "w") as f:
                data_left = res_blur_size
                offset = 0
                current_limit = REQUEST_CHUNK_SIZE
                while data_left > 0:
                    requested = min(current_limit, data_left)
                    while True:
                        try:
                            request = BlurRequest(data_length=requested, offset=offset)
                            ack: ResourceAck = await ctx.call(request, ResourceAck)
                            break
                        except Exception as e:
                            if _is_codec_too_large(e) and requested > MIN_CHUNK_SIZE:
                                new_limit = max(requested // 2, MIN_CHUNK_SIZE)
                                _heavy_gc(5)
                                current_limit = new_limit
                                requested = min(new_limit, data_left)
                                continue
                            _heavy_gc(3)
                            try:
                                request = BlurRequest(
                                    data_length=requested, offset=offset
                                )
                                ack = await ctx.call(request, ResourceAck)
                                break
                            except Exception:
                                raise

                    data = ack.data_chunk
                    actual_len = len(data) if data else 0
                    if actual_len == 0:
                        raise wire.DataError("Received empty blur chunk")
                    if actual_len != requested:
                        current_limit = min(current_limit, actual_len)
                    digest = blake2s(data).digest()
                    if digest != ack.hash:
                        raise wire.DataError("Date digest is inconsistent")
                    f.write(data)
                    offset += actual_len
                    data_left -= actual_len
                f.sync()
            _verify_file_size(blur_path, res_blur_size)

        if res_type == ResourceType.Nft and config_path:
            with io.fatfs.open(config_path, "w") as f:
                assert msg.nft_meta_data
                f.write(msg.nft_meta_data)
                f.sync()
            _verify_file_size(config_path, metadata_len)

        gc.collect()

        if replace:
            lockscreen_wallpaper = device.get_homescreen()
            mainscreen_wallpaper = device.get_appdrawer_background()

            wallpapers_in_use = set()

            if lockscreen_wallpaper:
                if "/" in lockscreen_wallpaper:
                    lockscreen_name = lockscreen_wallpaper.split("/")[-1]
                else:
                    lockscreen_name = lockscreen_wallpaper

                if lockscreen_name.startswith("wp-"):
                    if "-blur." in lockscreen_name:
                        lockscreen_name = lockscreen_name.replace("-blur.", ".")
                    wallpapers_in_use.add(lockscreen_name)

            if mainscreen_wallpaper:
                if "/" in mainscreen_wallpaper:
                    mainscreen_name = mainscreen_wallpaper.split("/")[-1]
                else:
                    mainscreen_name = mainscreen_wallpaper

                if mainscreen_name.startswith("wp-"):
                    if "-blur." in mainscreen_name:
                        mainscreen_name = mainscreen_name.replace("-blur.", ".")
                    wallpapers_in_use.add(mainscreen_name)

            def safe_extract_timestamp(name):
                try:
                    parts = name[len("zoom-") :].split("-")
                    if len(parts) >= 2:
                        timestamp_part = parts[-2] if "-blur" in name else parts[-1]
                        if "." in timestamp_part:
                            timestamp_part = timestamp_part.split(".")[0]
                        return int(timestamp_part)
                    return 0
                except (ValueError, IndexError):
                    return 0

            name_list.sort(key=safe_extract_timestamp)

            zoom_file = None
            file_name = None
            for zoom_candidate in name_list:
                orig_candidate = zoom_candidate[len("zoom-") :]
                if orig_candidate not in wallpapers_in_use:
                    zoom_file = zoom_candidate
                    file_name = orig_candidate
                    break

            if zoom_file is None:
                replace = False

            if (
                replace
                and zoom_file
                and file_name
                and res_type == ResourceType.WallPaper
            ):
                zoom_to_delete = f"1:/res/wallpapers/{zoom_file}"
                orig_to_delete = f"1:/res/wallpapers/{file_name}"

                _ensure_file_removed(zoom_to_delete)
                _ensure_file_removed(orig_to_delete)

                blur_file_name = file_name[: -(len(res_ext) + 1)] + f"-blur.{res_ext}"
                blur_to_delete = f"1:/res/wallpapers/{blur_file_name}"
                _ensure_file_removed(blur_to_delete)
            elif replace and zoom_file and file_name and res_type == ResourceType.Nft:
                zoom_to_delete = f"1:/res/nfts/zooms/{zoom_file}"
                img_to_delete = f"1:/res/nfts/imgs/{file_name}"
                config_name = file_name[: -(len(res_ext) + 1)]
                config_to_delete = f"1:/res/nfts/desc/{config_name}.json"
                blur_file_name = f"{config_name}-blur.{res_ext}"
                blur_to_delete = f"1:/res/nfts/imgs/{blur_file_name}"

                _ensure_file_removed(zoom_to_delete)
                _ensure_file_removed(img_to_delete)
                _ensure_file_removed(config_to_delete)
                _ensure_file_removed(blur_to_delete)
        elif res_type == ResourceType.WallPaper:
            device.increase_wp_cnts()

    except BaseException as e:
        for _ in range(5):
            gc.collect()

        _cleanup_partial_files(file_full_path, zoom_path, blur_path, config_path)

        if hasattr(trezorio, "jpeg_restore_decoder_state"):
            trezorio.jpeg_restore_decoder_state()  # type: ignore[is not a known member of module]

        raise wire.FirmwareError(f"Failed to write file with error code {e}")

    for _ in range(5):
        gc.collect()

    if hasattr(trezorio, "jpeg_restore_decoder_state"):
        trezorio.jpeg_restore_decoder_state()  # type: ignore[is not a known member of module]

    if res_type == ResourceType.WallPaper:
        wallpaper_files = []
        for size, _attrs, name in io.fatfs.listdir("1:/res/wallpapers"):
            if (
                size > 0
                and name.startswith("wp-")
                and not name.endswith("-blur.jpeg")
                and not name.endswith("-blur.jpg")
            ):
                wallpaper_files.append(name)

        if len(wallpaper_files) > 5:

            def extract_timestamp(filename):
                try:
                    parts = filename.rsplit("-", 1)
                    if len(parts) == 2:
                        timestamp_str = parts[1].split(".")[0]
                        return int(timestamp_str)
                except (ValueError, IndexError):
                    return 0
                return 0

            wallpaper_files.sort(key=extract_timestamp, reverse=True)

            lockscreen_wallpaper = device.get_homescreen()
            lockscreen_wallpaper_name = None
            if lockscreen_wallpaper:
                if "/" in lockscreen_wallpaper:
                    lockscreen_wallpaper_name = lockscreen_wallpaper.split("/")[-1]
                else:
                    lockscreen_wallpaper_name = lockscreen_wallpaper

                if lockscreen_wallpaper_name.startswith("wp-"):
                    if "-blur." in lockscreen_wallpaper_name:
                        lockscreen_wallpaper_name = lockscreen_wallpaper_name.replace(
                            "-blur.", "."
                        )
                else:
                    lockscreen_wallpaper_name = None

            mainscreen_wallpaper = device.get_appdrawer_background()
            mainscreen_wallpaper_name = None
            if mainscreen_wallpaper:
                if "/" in mainscreen_wallpaper:
                    mainscreen_wallpaper_name = mainscreen_wallpaper.split("/")[-1]
                else:
                    mainscreen_wallpaper_name = mainscreen_wallpaper

                if mainscreen_wallpaper_name.startswith("wp-"):
                    if "-blur." in mainscreen_wallpaper_name:
                        mainscreen_wallpaper_name = mainscreen_wallpaper_name.replace(
                            "-blur.", "."
                        )
                else:
                    mainscreen_wallpaper_name = None

            wallpapers_in_use = set()
            if lockscreen_wallpaper_name:
                wallpapers_in_use.add(lockscreen_wallpaper_name)
            if mainscreen_wallpaper_name:
                wallpapers_in_use.add(mainscreen_wallpaper_name)

            slots_available = max(5 - len(wallpapers_in_use), 0)

            files_to_keep = wallpapers_in_use.copy()
            for wallpaper_name in wallpaper_files:
                if wallpaper_name not in wallpapers_in_use:
                    if slots_available > 0:
                        files_to_keep.add(wallpaper_name)
                        slots_available -= 1
                    else:
                        break

            for wallpaper_name in wallpaper_files:
                if wallpaper_name not in files_to_keep:
                    orig_path = f"1:/res/wallpapers/{wallpaper_name}"
                    _ensure_file_removed(orig_path)

                    zoom_path = f"1:/res/wallpapers/zoom-{wallpaper_name}"
                    _ensure_file_removed(zoom_path)

                    dot_idx = wallpaper_name.rfind(".")
                    if dot_idx > 0:
                        base_name = wallpaper_name[:dot_idx]
                        wallpaper_ext = wallpaper_name[dot_idx + 1 :]
                        blur_path = (
                            f"1:/res/wallpapers/{base_name}-blur.{wallpaper_ext}"
                        )
                        _ensure_file_removed(blur_path)

    return Success(message="Success")


def _heavy_gc(passes: int = 3) -> tuple[int, int]:
    for _ in range(passes):
        gc.collect()
    return gc.mem_free(), gc.mem_alloc()  # type: ignore[is not a known member of module]


def _is_codec_too_large(err: Exception) -> bool:
    name = type(err).__name__
    msg = getattr(err, "args", ())
    if msg:
        msg = msg[0]
    return (
        name in ("CodecError", "DataError")
        and isinstance(msg, str)
        and "Message too large" in msg
    )
