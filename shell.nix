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
      lldb
      vscode-extensions.vadimcn.vscode-lldb
      emscripten
      python3
      nodejs_24
      lua5_3
      luajit_2_0
    ];
  env = {
    CODELLDB_PATH = "${pkgs.vscode-extensions.vadimcn.vscode-lldb}/share/vscode/extensions/vadimcn.vscode-lldb/adapter/codelldb";
    VCPKG_ROOT = "${pkgs.vcpkg}/share/vcpkg";
    EMSDK = "${pkgs.emscripten}";
    EMSCRIPTEN_ROOT = "${pkgs.emscripten}/share/emscripten";
  };
}
