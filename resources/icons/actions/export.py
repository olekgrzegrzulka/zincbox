import xml.etree.ElementTree as ET

SVG_INPUT_FILE = "icons.svg"
PNG_OUTPUT_SIZE = 16
STROKE_WIDTH = 2.5
RASTERIZER = "resvg"  # "resvg" or "cairo"

MAIN_COLOR_HEX = "#ffc2f7"
ACCENT_COLOR_HEX = "#ff8aef"

if RASTERIZER == "cairo":
    import cairosvg
elif RASTERIZER == "resvg":
    from resvg_py import resvg_py


def render_png(svg_bytes: bytes, output_path: str, size: int) -> None:
    if RASTERIZER == "cairo":
        cairosvg.svg2png(
            bytestring=svg_bytes,
            write_to=output_path,
            output_width=size,
            output_height=size,
        )
    elif RASTERIZER == "resvg":
        png_data = resvg_py.svg_to_bytes(
            svg_string=svg_bytes.decode("utf-8"),
            width=size,
            height=size,
        )
        with open(output_path, "wb") as f:
            f.write(png_data)


tree = ET.parse(SVG_INPUT_FILE)
root = tree.getroot()

for icon in root.findall("{http://www.w3.org/2000/svg}svg"):
    icon_id = icon.attrib.get("id")
    if not icon_id:
        continue

    icon.attrib.pop("x", None)
    icon.attrib.pop("y", None)
    icon.attrib["stroke"] = MAIN_COLOR_HEX
    icon.attrib["stroke-width"] = str(STROKE_WIDTH)

    style_elem = ET.Element("{http://www.w3.org/2000/svg}style")
    style_elem.text = f".accent {{ stroke: {ACCENT_COLOR_HEX} !important; }}"
    icon.insert(0, style_elem)

    svg_data = ET.tostring(icon, encoding="utf-8", xml_declaration=True)
    render_png(svg_data, f"{icon_id}.png", PNG_OUTPUT_SIZE)
