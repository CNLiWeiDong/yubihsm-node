{
  "targets": [
    {
      "target_name": "yubihsm",
      "sources": [ 
        "src/main.cpp",
      ],
      "include_dirs": [
        "<(module_root_dir)/yubihsm2-sdk/include",
        "<!@(node -p \"require('node-addon-api').include\")",
        "src",
      ],
      "link_settings": {
        "libraries": [
          "-lyubihsm",
        ],
        "library_dirs": [
            "<(module_root_dir)/yubihsm2-sdk/lib",
        ],
      },
      'dependencies': [
        "<!(node -p \"require('node-addon-api').gyp\")",
        # "fetch_sdk",
      ],
      'cflags!': [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],
      'cflags_cc': [
        "-std=c++17",
      ],
      "cflags": [
        "-std=c++17"
      ],
      'xcode_settings': {
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
        'CLANG_CXX_LIBRARY': 'libc++',
        'MACOSX_DEPLOYMENT_TARGET': '10.14',
        'CLANG_CXX_LANGUAGE_STANDARD': 'c++17',
      },
      'msvs_settings': {
        'VCCLCompilerTool': { 'ExceptionHandling': 1 },
      },
      'conditions': [
        ['OS=="mac"', {
            'cflags+': ['-fvisibility=hidden'],
            'xcode_settings': {
              'GCC_SYMBOLS_PRIVATE_EXTERN': 'YES', # -fvisibility=hidden
            },
            # amd还是arm重新设置 link_settings中的library_dirs
          }
        ],
      ],
    },
  ],
}
