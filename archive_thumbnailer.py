#!/usr/bin/env python3
import sys
import os
import zipfile
import rarfile
from PIL import Image
from io import BytesIO

SUPPORTED_FILETYPES = ('.png', '.jpg', '.jpeg', '.webp')


def get_first_image_from_zip(zip_path) -> Image.Image:
    """Extract the first image found inside a ZIP archive."""
    with zipfile.ZipFile(zip_path, 'r') as z:
        images = [f for f in z.namelist() if f.lower().endswith(SUPPORTED_FILETYPES)]
        if images:
            with z.open(images[0]) as img_file:
                return Image.open(BytesIO(img_file.read()))
    return None


def get_first_image_from_rar(rar_path) -> Image.Image:
    """Extract the first image found inside a RAR archive."""
    with rarfile.RarFile(rar_path, 'r') as r:
        images = [f for f in r.namelist() if f.lower().endswith(SUPPORTED_FILETYPES)]
        if images:
            with r.open(images[0]) as img_file:
                return Image.open(BytesIO(img_file.read()))
    return None


def generate_thumbnail(input_file, output_file, size=(256, 256)):
    """Generate a thumbnail from the first image inside ZIP or RAR."""
    try:
        if input_file.lower().endswith(".zip"):
            img = get_first_image_from_zip(input_file)
        elif input_file.lower().endswith(".rar"):
            img = get_first_image_from_rar(input_file)
        else:
            print("Unsupported format", file=sys.stderr)
            return 1

        if img:
            img.thumbnail(size)
            img.save(output_file, "PNG")
            return 0  # Success
        else:
            print("No images found in archive", file=sys.stderr)
            return 1  # Failure
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1  # Failure


if __name__ == "__main__":
    input_path = sys.argv[1]
    output_path = sys.argv[2]
    sys.exit(generate_thumbnail(input_path, output_path))
