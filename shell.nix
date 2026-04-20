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
      emscripten
      python3
      nodejs_24
      lua5_3
      luajit_2_0
      just
    ];
  env = {
    VCPKG_ROOT = "${pkgs.vcpkg}/share/vcpkg";
    EMSDK = "${pkgs.emscripten}";
    EMSCRIPTEN_ROOT = "${pkgs.emscripten}/share/emscripten";
  };
}
