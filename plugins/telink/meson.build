if gudev.found() and gusb.found()
cargs = ['-DG_LOG_DOMAIN="FuPluginTelink"']
plugins += {meson.current_source_dir().split('/')[-1]: true}
plugin_quirks += files('telink.quirk')
plugin_builtins += static_library('fu_plugin_telink',
  sources: [
    'fu-plugin-telink.c',
    'fu-telink-hid-dev.c',
    'fu-telink-firmware.c',
    'fu-telink-firmware-beta.c',
    'fu-telink-firmware-v1.c',
    'fu-telink-archive.c',
  ],
  include_directories: plugin_incdirs,
  link_with: plugin_libs,
  c_args: cargs,
  dependencies: plugin_deps,
)
endif

#if get_option('bluez')
cargs = ['-DG_LOG_DOMAIN="FuPluginTelinkBle"']
plugins += {meson.current_source_dir().split('/')[-1]: true}
plugin_quirks += files('telink.quirk')
plugin_builtins += static_library('fu_plugin_telink_ble',
  sources: [
    'fu-plugin-telink-ble.c',
    'fu-telink-ble-dev.c',
    'fu-telink-firmware.c',
    'fu-telink-firmware-beta.c',
    'fu-telink-firmware-v1.c',
    'fu-telink-archive.c',
  ],
  include_directories: plugin_incdirs,
  link_with: plugin_libs,
  c_args: cargs,
  dependencies: plugin_deps,
)
#endif
