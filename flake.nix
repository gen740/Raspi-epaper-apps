{
  description = "Flake shell";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
    mynixpkgs = {
      url = "git+ssh://github.com/gen740/mynixpkgs";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    inputs@{ flake-parts, nixpkgs, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = nixpkgs.lib.platforms.all;

      perSystem =
        { pkgs, ... }:
        {
          devShells.default = pkgs.mkShell {
            packages = [

              pkgs.libgpiod_1
              pkgs.cmake
              pkgs.ninja
              pkgs.llvmPackages_20.clang-tools
              pkgs.cmake-format
              pkgs.cmake-language-server
            ];
          };

          packages.default = pkgs.stdenv.mkDerivation {
            name = "demo";
            src = ./.;
            buildInputs = [
              pkgs.libgpiod_1
              pkgs.cmake
              pkgs.ninja
            ];
          };
        };
    };
}
