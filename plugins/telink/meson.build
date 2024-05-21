install_data([
  'telink.quirk',
  ],
  install_dir: join_paths(datadir, 'fwupd', 'quirks.d')
)

shared_module('fu_plugin_telink',
  fu_hash,
  sources : [
    'fu-plugin-telink.c',
    'fu-telink-config.c',
    'fu-telink-firmware.c',
    'fu-telink-archive.c',
  ],
  include_directories : [
    root_incdir,
    fwupd_incdir,
    fwupdplugin_incdir,
  ],
  install : true,
  install_dir: plugin_dir,
  dependencies : [
    plugin_deps,
  ],
  link_with : [
    fwupd,
    fwupdplugin,
  ],
)
