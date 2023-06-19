#pragma once

#ifndef ADRIFT_UTIL_CONSTANT_H
#  define ADRIFT_UTIL_CONSTANT_H

const char *const ADRIFT_VERSION = "0.0.1";
const char *const ADRIFT_NAME = "adrift";

const char *DEFAULT_CONFIG_FILE = "./dnsrelay.txt";
const char *DEFAULT_DNS_SERVER = "192.168.0.1";
const i32 DEFAULT_DEBUG_LEVEL = 0;

#define BOLD "\033[1m"
#define UNDERLINE "\033[4m"
#define RESET "\033[0m"

#endif // ADRIFT_UTIL_CONSTANT_H
