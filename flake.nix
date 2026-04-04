{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };
  outputs =
    { nixpkgs, ... }:
    {
      devShells =
        let
          system = "x86_64-linux";
        in
        {
          ${system} = {
            default =
              let
                pkgs = import nixpkgs { inherit system; };
              in
              import ./shell.nix { inherit pkgs; };
          };
        };
    };
}
