{
  "targets": [
    {
      "target_name": "jsox",
      'win_delay_load_hook': 'false',      
      "sources": [ "jsoxParse.cc",
           "jsox.cc",
          ],
	'defines': [ "BUILD_NODE_ADDON",
          'TARGETNAME="jsox.node"'
        ],
    'conditions': [
          ['OS=="linux"', {
            'defines': [
              '__LINUX__', '__NO_ODBC__','__MANUAL_PRELOAD__'
            ],
            'cflags_cc': ['-Wno-misleading-indentation','-Wno-parentheses','-Wno-unused-result'
			,'-Wno-char-subscripts'
			,'-Wno-empty-body','-Wno-format', '-Wno-address'
			, '-Wno-strict-aliasing', '-Wno-switch', '-Wno-missing-field-initializers' 
			, '-Wno-unused-variable', '-Wno-unused-function', '-Wno-unused-but-set-variable', '-Wno-maybe-uninitialized'
			, '-Wno-sign-compare', '-Wno-unknown-warning', '-fexceptions'
			],
            'cflags': ['-Wno-implicit-fallthrough'
			],
            'include_dirs': [
              'include/linux',
            ]
          }],
	['node_shared_openssl=="false"', {
	      'include_dirs': [
	        '<(node_root_dir)/deps/openssl/openssl/include'
	      ],
		"conditions" : [
			["target_arch=='ia32'", {
			 "include_dirs": [ "<(node_root_dir)/deps/openssl/config/piii" ]
			}],
			["target_arch=='x64'", {
			 "include_dirs": [ "<(node_root_dir)/deps/openssl/config/k8" ]
			}],
			["target_arch=='arm'", {
			 "include_dirs": [ "<(node_root_dir)/deps/openssl/config/arm" ]
			}]
        	]	
	}],
	['OS=="mac"', {
            'defines': [
              '__LINUX__','__MAC__', '__NO_ODBC__',"__NO_OPTIONS__"
            ],
            'xcode_settings': {
                'OTHER_CFLAGS': [
                       '-Wno-self-assign', '-Wno-null-conversion', '-Wno-parentheses-equality', '-Wno-parentheses'
			,'-Wno-char-subscripts', '-Wno-null-conversion'
			,'-Wno-empty-body','-Wno-format', '-Wno-address'
			, '-Wno-strict-aliasing', '-Wno-switch', '-Wno-missing-field-initializers' 
			, '-Wno-unused-variable', '-Wno-unused-function'
			, '-Wno-sign-compare', '-Wno-null-dereference'
			, '-Wno-address-of-packed-member', '-Wno-unknown-warning-option'
			, '-Wno-unused-result', '-fexceptions', '-Wno-unknown-pragma'
                ],
             },
            'include_dirs': [
              'include/linux',
            ],
          }],
          ['OS=="win"', {
            'configurations': {
              'Debug': {
                'msvs_settings': {
                  'VCCLCompilerTool': {
                    'BufferSecurityCheck': 'false',
                    'RuntimeTypeInfo': 'true',
                    'MultiProcessorCompilation' : 'true',
                    'InlineFunctionExpansion': 2,
                    'OmitFramePointers': 'true',
                    'ExceptionHandling':2

                  }
                }
              },
              'Release': {                            
                'msvs_settings': {
                  'VCCLCompilerTool': {
                    'BufferSecurityCheck': 'false',
                    'RuntimeTypeInfo': 'true',
                    'MultiProcessorCompilation' : 'true',
                    'InlineFunctionExpansion': 2,
                    'OmitFramePointers': 'true',
                    'ExceptionHandling':2
                  }
                }
              }
            },
            'sources': [
              # windows-only; exclude on other platforms.
            ],
  	        'libraries':[ 'winmm' ]
          }, { # OS != "win",
            'defines': [
              '__LINUX__',
            ],
          }]
        ],
    }
  ],

  "target_defaults": {
  	'include_dirs': ['src/sack']
  }
  
}

