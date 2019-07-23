#/bin/sh
./build/super_resolve  /home/fgjtombs/fydp/super_resolution/traced-script-module-witcher2x.pt input/resize_fallout.jpg
python3 util/psnr.py -o fallout.jpg -m output/final_result.jpg
