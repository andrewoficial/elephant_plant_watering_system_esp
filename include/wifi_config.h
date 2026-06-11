// wifi_config.h
#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

// Список точек доступа: { "SSID", "password" }
const char* wifi_networks[][2] = {
  {"IGM", "GtWoW#_9A!L"},
  {"Foxy Wi-Fi", "NopswdPur008!"},
  {"NOT_ASUS", "r4e25y7hdy4-*f5"},
  {"NETGEAR", "zaqzaq234"}
};

const int wifi_networks_count = sizeof(wifi_networks) / sizeof(wifi_networks[0]);

#endif