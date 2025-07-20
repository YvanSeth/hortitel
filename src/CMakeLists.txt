add_executable(receiver
    receiver.cpp
)
target_link_libraries(receiver
    MeloperoPerpetuo
    pico_stdlib
    hardware_adc
)
pico_enable_stdio_usb(receiver 1)
pico_enable_stdio_uart(receiver 0)
pico_add_extra_outputs(receiver)


add_executable(sender
    sender.cpp
)
target_link_libraries(sender
    MeloperoPerpetuo
    pico_stdlib
    hardware_adc
)
pico_enable_stdio_usb(sender 1)
pico_enable_stdio_uart(sender 0)
pico_add_extra_outputs(sender)
