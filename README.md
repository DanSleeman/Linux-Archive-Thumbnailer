# Archive Thumbnailer

A simple thumbnailer that sets thumbnails for zip and rar archives using the first image file found in the archive.

I couldn't find anything similar so I created this.

I've only tested this in Mint 6.1 running Cinnamon 6.4.6 so I make no guarantees for any other distros.

There are two versions. My initial python script for a proof of concept and a C version.

Tested successfully with .zip .tar .rar and .7z archive file types.

Theoretically can also work with .cbz .cbr and .c7 file formats, but other programs/thumbnailers support these file types.

Evince for instance handles .cb_ files, but specifically excludes .zip files.

# C

## Dependencies

* libpng
* libjpeg
* libarchive
* libwebp

## Installation

1. Ensure you have the required libraries.

```bash
sudo apt update
sudo apt install libarchive-dev libjpeg-dev libpng-dev libwebp-dev
```

2. Compile the c script:

```bash
gcc archive_thumbnailer.c -o archive_thumbnailer -larchive -lpng -ljpeg -lwebp
```

3. Move the file: (or just stash it anywhere and modify the .thumbnailer path reference)

```bash
sudo mv archive_thumbnailer /bin/
```

4. Download the `archive-thumbnailer.thumbnailer` file and move it:

```bash
mv archive-thumbnailer.thumbnailer ~/.local/share/thumbnailers/
```

# Python

## Requirements

Python packages:
* zipfile (Hopefully already packaged in your distro)
* rarfile
* pillow (Hopefully already packaged in your distro)

## Installation

1. Copy the archive_thumbnailer.py to your local machine.

2. Make it executable:

```bash
chmod +x archive_thumbnailer.py
```

3. Move it somewhere and record that path.

```bash
sudo mv archive_thumbnailer.py /bin/
```

4. Copy the archive-thumbnailer.thumbnailer file to your local machine.

    Place it in `~/.local/share/thumbnailers/`

5. Clear thumbnail cache if needed:

```bash
rm -Rf ~/.cache/thumbnails/*
```

6. Restart file browser.