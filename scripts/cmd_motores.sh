#!/bin/bash

while sleep 0.002; do
	i2cset -y 2 0x68 0xA2 $1
	i2cset -y 2 0x69 0xA2 $2
	i2cset -y 2 0x6A 0xA2 $3
	i2cset -y 2 0x6A 0xA2 $4
done