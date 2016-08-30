#!/bin/bash

gcc -std=gnu99 $@ -DDEBUG -Isrc -Isrc/actuator_control/ src/actuator_control/simple_control.c \
    -fPIC -shared -o plugins/actuator_control/SimpleControl.so

gcc -std=gnu99 $@ -DDEBUG -Isrc -Isrc/robot_control/ src/robot_control/anklebot.c \
    -fPIC -shared -o plugins/robot_control/AnkleBot.so

gcc -std=gnu99 $@ -DDEBUG -Isrc -Isrc/data_io/ src/klib/kson.c src/data_io/json_io.c \
    -fPIC -shared -o plugins/data_io/JSON.so

gcc -std=gnu99 $@ -DDEBUG -Isrc -Isrc/signal_io/ -Isrc/pci4e/ src/signal_io/pci4e/pci4e_helper.c \
    src/signal_io/pci4e/pci4e.c -fPIC -shared -o plugins/signal_io/PCI4E.so

gcc -std=gnu99 $@ -DDEBUG -Isrc -Isrc/signal_io/ src/signal_io/dummy_io.c \
    -fPIC -shared -o plugins/signal_io/Dummy.so

gcc -std=gnu99 $@ -DDEBUG -Isrc -Isrc/signal_io/ -Isrc/power_daq/ src/signal_io/power_daq/pd2_mfx.c \
    -fPIC -shared -o plugins/signal_io/PD2MFx.so -Llibs -lpowerdaq32

gcc -std=gnu99 $@ -DDEBUG -Isrc -Isrc/signal_io/ -Isrc/power_daq/ src/signal_io/power_daq/pd2_ao.c \
    -fPIC -shared -o plugins/signal_io/PD2AO.so -Llibs -lpowerdaq32
