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
      python3
    ];
  shellHook = ''
    export VCPKG_ROOT=${pkgs.vcpkg}/share/vcpkg
    export EMSCRIPTEN_ROOT=${pkgs.emscripten}/share/emscripten
  '';
}
