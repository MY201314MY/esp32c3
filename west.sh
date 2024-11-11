path=`pwd`
project=${path}/samples/uart

BOARD=esp32c3_devkitm

west build -p --build-dir ${path}/build \
    --board ${BOARD} ${project} --no-sysbuild \
    -DZEPHYR_EXTRA_MODULES:STRING="${path}" \
    -DCONF_FILE:STRING="${project}/prj.conf;"

