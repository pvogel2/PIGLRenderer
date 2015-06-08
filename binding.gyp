{
  "targets":[
    {
      "target_name": "pigl",
      "sources": [ "pigl.cc" ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")",
        "/opt/vc/include/",
        "/opt/vc/include/interface/vcos/pthreads/",
        "/opt/vc/include/interface/vmcs_host/linux/"
      ],
      'library_dirs': ['/opt/vc/lib/'],
      'libraries': ['-lGLESv2','-lEGL','-lbcm_host']
    }
  ]
}
