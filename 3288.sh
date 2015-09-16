#!/bin/bash
make ARCH=arm rockchip_defconfig
make ARCH=arm rk3288-evb-act8846.img
