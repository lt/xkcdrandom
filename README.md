xkcdrandom
==========

This kernel module adds a `/dev/xkcdrandom` device, which can be read to return a number [chosen by a fair dice roll](https://xkcd.com/221/).

There are two optional parameters that can be used to tune the device.

`spam` is a boolean, and controls whether the device outputs a single value before reaching EOF, or continues to write out values forever! The default is to return a single value.

`value` is the character code of the value to return. The default is 52.

Example: `$ sudo insmod xkcdrandom.ko spam=N value=52`
