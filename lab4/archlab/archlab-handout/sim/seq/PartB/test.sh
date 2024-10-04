cd ..
make VERSION=full
./ssim -t ../y86-code/asumi.yo
cd ../y86-code
make testssim
cd ../ptest
make SIM=../seq/ssim
make SIM=../seq/ssim TFLAGS=-i
