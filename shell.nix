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
    ]
    ++ [
      gcc
      emscripten
    ];
  shellHook = ''
    export VCPKG_ROOT=${pkgs.vcpkg}/share/vcpkg
  '';
}
