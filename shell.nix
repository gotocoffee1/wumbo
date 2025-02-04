{
  pkgs ? import <nixpkgs> { },
}:

pkgs.mkShell {
  packages = with pkgs; [
    cmake
    ninja
    ccache
    gcc
    vcpkg
    pkg-config
  ];
  shellHook = ''
    export VCPKG_ROOT=${pkgs.vcpkg}/share/vcpkg
  '';
}
