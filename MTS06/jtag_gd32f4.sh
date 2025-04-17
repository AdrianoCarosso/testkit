if [ "$1" != "" ] ; then
cp $1 code.gd32f4.bin
fi

sudo ./openocd -f SendOOCD.gd32f4.cfg $2

