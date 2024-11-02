path=`pwd`
project=${path}/samples/uart

BOARD=esp32c3_devkitm

west build -p --build-dir ${path}/build \
    --board ${BOARD} ${project} --no-sysbuild \
    -DCONF_FILE:STRING="${project}/prj.conf;"

