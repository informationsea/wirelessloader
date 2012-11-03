mkdir -p binaries

function startaddr() {
    case $1 in
        "atmega168p")
            echo '3800' ;;
        "atmega164p")
            echo '3800' ;;
        "atmega328p")
            echo '7800' ;;
        "atmega324p")
            echo '7800' ;;
        "atmega644p")
            echo 'f800' ;;
        *)
            echo "Unknown MCU"
            exit 1;;
    esac
}

function target() {
    echo serialloader-$1-$2-`echo $3|sed -e 's/000000//'`
}

#for mcu in atmega168p atmega164p atmega328p atmega324p atmega644p;do
#    for baud in 9600 38400 115200;do
#        for cpu in 8000000 12000000 20000000;do
for mcu in atmega328p  atmega644p;do
    for baud in 38400;do
        for cpu in 8000000 20000000;do
            echo `target $mcu $target`
            make TARGET=`target $mcu $baud $cpu` MCU=$mcu F_CPU=$cpu BOOTLOADER_ADDRESS=`startaddr $mcu` BAUDRATE=$baud
            mv `target $mcu $baud $cpu`.hex binaries
            make TARGET=`target $mcu $baud $cpu` clean
            #make clean
        done
    done
done
