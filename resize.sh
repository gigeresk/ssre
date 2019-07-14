#!/bin/bash

echo "--- Cleaning Low Res Folder"
rm -f target/*.jpg
rm -f target/*.jpeg
rm -f target/*.png

if [ -z "$(ls -A input/)" ]; then
   echo "--- Aborting! --- Input folder is empty"
   exit 1
fi

echo "--- Downscaling target images"
cp -p input/* target/

for f in target/*; do
    case ${f: -4} in
        .png)
            extension='.png'
            ;;
        .jpg)
            extension='.jpg'
            ;;
        .jpeg)
            extension='.jpg'
            ;;
        *)
            continue
            ;;
    esac

    filename=$(echo "$(basename "$f")" | cut -f 1 -d '.')
    magick convert target/$filename$extension -resize 50% target/rsz_$filename.png
    if [ $? -ne 0 ]; then
        echo "--- Aborting! --- Error with downscaling images"
        exit 1
    fi
    rm -f target/$filename$extension
done

echo "--- Successfully downscaled target images"