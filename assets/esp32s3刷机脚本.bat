@echo off
chcp 65001 > nul
setlocal enabledelayedexpansion

echo 请在设备管理器中查找esp32s3的端口号，请将esp32s3启动到刷机模式里
set /p portNumber=请输入想要刷入端口号（不用加COM）: 


set "cmd=esptool.exe --chip esp32s3 --baud 921600 --port COM%portNumber% --before default_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x0000 bootloader.bin 0x8000 partitions.bin 0xe000 boot_app0.bin 0x10000 firmware.bin"


echo 最终命令: %cmd%

%cmd%

endlocal

pause