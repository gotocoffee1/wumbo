{
  pkgs ? import <nixpkgs> { },
}:
let
  unstable =
    import
      (fetchTarball "https://github.com/nixos/nixpkgs/tarball/0868bed2208dbf49976ba910a0c942cac372c47e")
      { };
in
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
      unstable.emscripten
      python3
      nodejs_23
      lua5_3
      luajit_2_0
    ];
  shellHook = ''
    export VCPKG_ROOT=${pkgs.vcpkg}/share/vcpkg
    export EMSDK=${unstable.emscripten}
    export EM_CACHE=~/.emscripten_cache
  '';
}
