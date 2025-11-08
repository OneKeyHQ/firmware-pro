import math

from .common import lv


def create_preview_container(
    parent,
    *,
    width: int,
    height: int,
    top_offset: int = 0,
    align=lv.ALIGN.TOP_MID,
    style=None,
    bg_color=None,
    bg_opa=None,
) -> lv.obj:
    container = lv.obj(parent)
    container.set_size(width, height)
    container.align(align, 0, top_offset)

    if style is not None:
        container.add_style(style, 0)

    container.clear_flag(lv.obj.FLAG.CLICKABLE)
    container.add_flag(lv.obj.FLAG.EVENT_BUBBLE)
    container.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
    container.clear_flag(lv.obj.FLAG.SCROLLABLE)
    container.set_style_radius(0, 0)
    container.set_style_clip_corner(False, 0)

    if bg_color is not None:
        container.set_style_bg_color(bg_color, 0)
    if bg_opa is not None:
        container.set_style_bg_opa(bg_opa, 0)

    return container


def create_top_mask(
    container: lv.obj,
    *,
    height: int = 2,
    color=None,
    opa=None,
) -> lv.obj:
    mask = lv.obj(container)
    mask.remove_style_all()
    mask.set_size(container.get_width(), height)
    mask.align(lv.ALIGN.TOP_MID, 0, 0)
    mask.add_flag(lv.obj.FLAG.IGNORE_LAYOUT)
    mask.clear_flag(lv.obj.FLAG.CLICKABLE)

    if color is None:
        color = lv.color_hex(0x000000)
    if opa is None:
        opa = lv.OPA.COVER

    mask.set_style_bg_color(color, 0)
    mask.set_style_bg_opa(opa, 0)
    mask.move_foreground()

    return mask


def create_preview_image(
    container: lv.obj,
    src=None,
    *,
    base_size=(480, 800),
    target_size=None,
    align=lv.ALIGN.CENTER,
    antialias: bool = True,
) -> lv.img:
    target_width = container.get_width() if not target_size else target_size[0]
    target_height = container.get_height() if not target_size else target_size[1]

    base_width, base_height = base_size

    image = lv.img(container)
    image.set_size(lv.SIZE.CONTENT, lv.SIZE.CONTENT)
    image.clear_flag(lv.obj.FLAG.SCROLLABLE)

    zoom_x = math.ceil(target_width / base_width * 256)
    zoom_y = math.ceil(target_height / base_height * 256)
    zoom = max(int(zoom_x), int(zoom_y))
    image.set_zoom(zoom)
    image.set_antialias(antialias)
    image.align(align, 0, 0)

    if src is not None:
        image.set_src(src)

    return image
