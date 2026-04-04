{
  pkgs ? import <nixpkgs> { },
}:
pkgs.mkShell {
  packages =
    with pkgs;
    [
      cmake
      ninja
    ]
    ++ [
      ccache
      clang-tools
    ]
    ++ [
      vcpkg
      pkg-config
      readline
    ]
    ++ [
      gcc
      gdb
      emscripten
      python3
      nodejs_24
      lua5_3
      luajit_2_0
    ];
  env = {
    VCPKG_ROOT = "${pkgs.vcpkg}/share/vcpkg";
    EMSDK = "${pkgs.emscripten}";
  };
}
